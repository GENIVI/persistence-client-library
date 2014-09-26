/******************************************************************************
 * Project         Persistency
 * (c) copyright   2012
 * Company         XS Embedded GmbH
 *****************************************************************************/
/******************************************************************************
 * This Source Code Form is subject to the terms of the
 * Mozilla Public License, v. 2.0. If a  copy of the MPL was not distributed
 * with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
******************************************************************************/
 /**
 * @file           persistence_client_library.c
 * @ingroup        Persistence client library
 * @author         Ingo Huerner
 * @brief          Implementation of the persistence client library.
 *                 Library provides an API to access persistent data
 * @see            
 */


#include "persistence_client_library_lc_interface.h"
#include "persistence_client_library_pas_interface.h"
#include "persistence_client_library_dbus_service.h"
#include "persistence_client_library_handle.h"
#include "persistence_client_library_custom_loader.h"
#include "persistence_client_library.h"
#include "persistence_client_library_backup_filelist.h"
#include "persistence_client_library_db_access.h"
#include "persistence_client_library_dbus_cmd.h"

#if USE_FILECACHE
   #include <persistence_file_cache.h>
#endif

#if USE_XSTRACE_PERS
   #include <xsm_user.h>
#endif

#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <dbus/dbus.h>




/// debug log and trace (DLT) setup
DLT_DECLARE_CONTEXT(gPclDLTContext);


/// global variable to store lifecycle shutdown mode
static int gShutdownMode = 0;
/// global shutdown cancel counter
static int gCancelCounter = 0;


int customAsyncInitClbk(int errcode)
{
	printf("Dummy async init Callback\n");

	return 1;
}


int pclInitLibrary(const char* appName, int shutdownMode)
{
   int rval = 1;

#if USE_XSTRACE_PERS
   xsm_send_user_event("%s - %d\n", __FUNCTION__, __LINE__);
#endif

   if(gPclInitialized == PCLnotInitialized)
   {
      gShutdownMode = shutdownMode;

      DLT_REGISTER_CONTEXT(gPclDLTContext,"PCL","Context for persistence client library logging");
      DLT_LOG(gPclDLTContext, DLT_LOG_INFO, DLT_STRING("pclInitLibrary => I N I T  Persistence Client Library - "), DLT_STRING(appName),
                              DLT_STRING("- init counter: "), DLT_INT(gPclInitialized) );

      char blacklistPath[DbPathMaxLen] = {0};

#if USE_FILECACHE
   DLT_LOG(gPclDLTContext, DLT_LOG_INFO, DLT_STRING("Using the filecache!!!"));
   pfcInitCache(appName);
#endif

      pthread_mutex_lock(&gDbusPendingRegMtx);   // block until pending received

      // Assemble backup blacklist path
      sprintf(blacklistPath, "%s%s/%s", CACHEPREFIX, appName, gBackupFilename);

      if(readBlacklistConfigFile(blacklistPath) == -1)
      {
         DLT_LOG(gPclDLTContext, DLT_LOG_WARN, DLT_STRING("pclInitLibrary - failed to access blacklist:"), DLT_STRING(blacklistPath));
      }

#if USE_XSTRACE_PERS
      xsm_send_user_event("%s - %d\n", __FUNCTION__, __LINE__);
#endif
      if(setup_dbus_mainloop() == -1)
      {
         DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("pclInitLibrary - Failed to setup main loop"));
         pthread_mutex_unlock(&gDbusPendingRegMtx);
         return EPERS_DBUS_MAINLOOP;
      }
#if USE_XSTRACE_PERS
      xsm_send_user_event("%s - %d\n", __FUNCTION__, __LINE__);
#endif


      if(gShutdownMode != PCL_SHUTDOWN_TYPE_NONE)
      {
         // register for lifecycle dbus messages
         if(register_lifecycle(shutdownMode) == -1)
         {
            DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("pclInitLibrary => Failed to register to lifecycle dbus interface"));
            pthread_mutex_unlock(&gDbusPendingRegMtx);
            return EPERS_REGISTER_LIFECYCLE;
         }
      }
#if USE_PASINTERFACE
      DLT_LOG(gPclDLTContext, DLT_LOG_INFO, DLT_STRING("PAS interface is enabled!!"));
      if(register_pers_admin_service() == -1)
      {
         DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("pclInitLibrary - Failed to register to pers admin dbus interface"));
         pthread_mutex_unlock(&gDbusPendingRegMtx);
         return EPERS_REGISTER_ADMIN;
      }
	  else
	  {
         DLT_LOG(gPclDLTContext, DLT_LOG_INFO,  DLT_STRING("pclInitLibrary - Successfully established IPC protocol for PCL."));
	  }
#else
      DLT_LOG(gPclDLTContext, DLT_LOG_WARN, DLT_STRING("PAS interface is not enabled, enable with \"./configure --enable-pasinterface\""));
#endif

      // load custom plugins
      if(load_custom_plugins(customAsyncInitClbk) < 0)
      {
      	DLT_LOG(gPclDLTContext, DLT_LOG_WARN, DLT_STRING("Failed to load custom plugins"));
      }

      // initialize keyHandle array
      init_key_handle_array();

      pers_unlock_access();

      // assign application name
      strncpy(gAppId, appName, MaxAppNameLen);
      gAppId[MaxAppNameLen-1] = '\0';

      gPclInitialized++;
   }
   else if(gPclInitialized >= PCLinitialized)
   {
      gPclInitialized++; // increment init counter
      DLT_LOG(gPclDLTContext, DLT_LOG_INFO, DLT_STRING("pclInitLibrary - I N I T  Persistence Client Library - "), DLT_STRING(gAppId),
                           DLT_STRING("- ONLY INCREMENT init counter: "), DLT_INT(gPclInitialized) );
   }

#if USE_XSTRACE_PERS
   xsm_send_user_event("%s - %d\n", __FUNCTION__, __LINE__);
#endif

   return rval;
}



int pclDeinitLibrary(void)
{
   int i = 0, rval = 1;

#if USE_XSTRACE_PERS
   xsm_send_user_event("%s - %d\n", __FUNCTION__, __LINE__);
#endif

   if(gPclInitialized == PCLinitialized)
   {
      int* retval;
   	MainLoopData_u data;
   	data.message.cmd = (uint32_t)CMD_QUIT;
   	data.message.string[0] = '\0'; 	// no string parameter, set to 0

      DLT_LOG(gPclDLTContext, DLT_LOG_INFO, DLT_STRING("pclDeinitLibrary - D E I N I T  client library - "), DLT_STRING(gAppId),
                                            DLT_STRING("- init counter: "), DLT_INT(gPclInitialized));

      // unregister for lifecycle dbus messages
      if(gShutdownMode != PCL_SHUTDOWN_TYPE_NONE)
         rval = unregister_lifecycle(gShutdownMode);

#if USE_PASINTERFACE == 1
      rval = unregister_pers_admin_service();
      if(0 != rval)
	  {
		  DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("pclDeinitLibrary - Failed to de-initialize IPC protocol for PCL."));
	  }
      else
      {
    	  DLT_LOG(gPclDLTContext, DLT_LOG_INFO,  DLT_STRING("pclDeinitLibrary - Successfully de-initialized IPC protocol for PCL."));
      }
#endif

      // unload custom client libraries
      for(i=0; i<PersCustomLib_LastEntry; i++)
      {
         if(gPersCustomFuncs[i].custom_plugin_deinit != NULL)
         {
            // deinitialize plugin
            gPersCustomFuncs[i].custom_plugin_deinit();
            // close library handle
            dlclose(gPersCustomFuncs[i].handle);

            invalidate_custom_plugin(i);
         }
      }

      process_prepare_shutdown(Shutdown_Full);	// close all db's and fd's and block access

      // send quit command to dbus mainloop
      deliverToMainloop_NM(&data);

      // wait until the dbus mainloop has ended
      pthread_join(gMainLoopThread, (void**)&retval);

      pthread_mutex_unlock(&gDbusPendingRegMtx);
      pthread_mutex_unlock(&gDbusInitializedMtx);

      gPclInitialized = PCLnotInitialized;

#if USE_FILECACHE
   pfcDeinitCache();
#endif

      DLT_UNREGISTER_CONTEXT(gPclDLTContext);
   }
   else if(gPclInitialized > PCLinitialized)
   {
      DLT_LOG(gPclDLTContext, DLT_LOG_INFO, DLT_STRING("pclDeinitLibrary - D E I N I T  client library - "), DLT_STRING(gAppId),
                                           DLT_STRING("- ONLY DECREMENT init counter: "), DLT_INT(gPclInitialized));
      gPclInitialized--;   // decrement init counter
   }
   else
   {
   	DLT_LOG(gPclDLTContext, DLT_LOG_WARN, DLT_STRING("pclDeinitLibrary - D E I N I T  client library - "), DLT_STRING(gAppId),
   	                                      DLT_STRING("- NOT INITIALIZED: "));
      rval = EPERS_NOT_INITIALIZED;
   }


#if USE_XSTRACE_PERS
   xsm_send_user_event("%s - %d\n", __FUNCTION__, __LINE__);
#endif

   return rval;
}



int pclLifecycleSet(int shutdown)
{
	int rval = 0;

	if(gShutdownMode == PCL_SHUTDOWN_TYPE_NONE)
	{
		if(shutdown == PCL_SHUTDOWN)
		{
			DLT_LOG(gPclDLTContext, DLT_LOG_INFO, DLT_STRING("pclLifecycleSet - PCL_SHUTDOWN -"), DLT_STRING(gAppId));
			process_prepare_shutdown(Shutdown_Partial);	// close all db's and fd's and block access
		}
		else if(shutdown == PCL_SHUTDOWN_CANCEL)
		{
			DLT_LOG(gPclDLTContext, DLT_LOG_INFO, DLT_STRING("pclLifecycleSet - PCL_SHUTDOWN_CANCEL -"), DLT_STRING(gAppId), DLT_STRING(" Cancel Counter - "), DLT_INT(gCancelCounter));
			if(gCancelCounter < Shutdown_MaxCount)
			{
				pers_unlock_access();
			}
			else
			{
				rval = EPERS_SHUTDOWN_MAX_CANCEL;
			}
		}
		else
		{
			rval = EPERS_COMMON;
		}
	}
	else
	{
		rval = EPERS_SHUTDOWN_NO_PERMIT;
	}

	return rval;
}



