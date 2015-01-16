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

#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <dbus/dbus.h>
#include <pthread.h>


/// debug log and trace (DLT) setup
DLT_DECLARE_CONTEXT(gPclDLTContext);


/// global variable to store lifecycle shutdown mode
static int gShutdownMode = 0;
/// global shutdown cancel counter
static int gCancelCounter = 0;

static pthread_mutex_t gInitMutex = PTHREAD_MUTEX_INITIALIZER;

#if USE_APPCHECK
/// global flag
static int gAppCheckFlag = -1;
#endif

int customAsyncInitClbk(int errcode)
{
   (void)errcode;
  printf("Dummy async init Callback\n");

  return 1;
}

// forward declaration
static int private_pclInitLibrary(const char* appName, int shutdownMode);
static int private_pclDeinitLibrary(void);


/* security check for valid application:
   if the RCT table exists, the application is proven to be valid (trusted),
   otherwise return EPERS_NOPRCTABLE  */
void doInitAppcheck(const char* appName)
{
#if USE_APPCHECK
   char rctFilename[DbPathMaxLen] = {0};
   snprintf(rctFilename, DbPathMaxLen, gLocalWtPathKey, appName, plugin_gResTableCfg);

   if(access(rctFilename, F_OK) == 0)
   {
      gAppCheckFlag = 1;   // "trusted" application
      DLT_LOG(gPclDLTContext, DLT_LOG_INFO, DLT_STRING("initLibrary - app check: "), DLT_STRING(appName), DLT_STRING("trusted app"));
   }
   else
   {
      gAppCheckFlag = 0;   // currently not a "trusted" application
      DLT_LOG(gPclDLTContext, DLT_LOG_INFO, DLT_STRING("initLibrary - app check: "), DLT_STRING(appName), DLT_STRING("NOT trusted app"));
   }
#endif
}



int doAppcheck(void)
{
   int trusted = 1;
#if USE_APPCHECK
   if(gAppCheckFlag != 1)
   {
      char rctFilename[DbPathMaxLen] = {0};
      snprintf(rctFilename, DbPathMaxLen, gLocalWtPathKey, gAppId, plugin_gResTableCfg);
      if(access(rctFilename, F_OK) == 0)
      {
         gAppCheckFlag = 1;   // "trusted" application
      }
      else
      {
         gAppCheckFlag = 0;   // not a "trusted" application
         trusted = 0;
      }
   }
#endif
   return trusted;
}



int pclInitLibrary(const char* appName, int shutdownMode)
{
   int rval = 1;

   pthread_mutex_lock(&gInitMutex);
   if(gPclInitCounter == 0)
   {
      DLT_REGISTER_CONTEXT(gPclDLTContext,"PCL","Context for persistence client library logging");
      DLT_LOG(gPclDLTContext, DLT_LOG_INFO, DLT_STRING("pclInitLibrary => INIT  PCL - "), DLT_STRING(appName),
                              DLT_STRING("- init counter: "), DLT_INT(gPclInitCounter) );

      rval = private_pclInitLibrary(appName, shutdownMode);
   }
   else
   {
      DLT_LOG(gPclDLTContext, DLT_LOG_INFO, DLT_STRING("pclInitLibrary - INIT  PCL - "), DLT_STRING(gAppId),
                                            DLT_STRING("- ONLY INCREMENT init counter: "), DLT_INT(gPclInitCounter) );
   }

   gPclInitCounter++;     // increment after private init, otherwise atomic access is too early
   pthread_mutex_unlock(&gInitMutex);

   return rval;
}

static int private_pclInitLibrary(const char* appName, int shutdownMode)
{
   int rval = 1;

   gShutdownMode = shutdownMode;

   char blacklistPath[DbPathMaxLen] = {0};

   doInitAppcheck(appName);      // check if we have a trusted application

#if USE_FILECACHE
 DLT_LOG(gPclDLTContext, DLT_LOG_INFO, DLT_STRING("Using the filecache!!!"));
 pfcInitCache(appName);
#endif

   pthread_mutex_lock(&gDbusPendingRegMtx);   // block until pending received

   // Assemble backup blacklist path
   snprintf(blacklistPath, DbPathMaxLen, "%s%s/%s", CACHEPREFIX, appName, gBackupFilename);

   if(readBlacklistConfigFile(blacklistPath) == -1)
   {
     DLT_LOG(gPclDLTContext, DLT_LOG_DEBUG, DLT_STRING("initLibrary - Err access blacklist:"), DLT_STRING(blacklistPath));
   }

   if(setup_dbus_mainloop() == -1)
   {
     DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("initLibrary - Failed to setup main loop"));
     pthread_mutex_unlock(&gDbusPendingRegMtx);
     return EPERS_DBUS_MAINLOOP;
   }

   if(gShutdownMode != PCL_SHUTDOWN_TYPE_NONE)
   {
     if(register_lifecycle(shutdownMode) == -1) // register for lifecycle dbus messages
     {
       DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("initLibrary => Failed reg to LC dbus interface"));
       pthread_mutex_unlock(&gDbusPendingRegMtx);
       return EPERS_REGISTER_LIFECYCLE;
     }
   }
#if USE_PASINTERFACE
   DLT_LOG(gPclDLTContext, DLT_LOG_INFO, DLT_STRING("PAS interface is enabled!!"));
   if(register_pers_admin_service() == -1)
   {
     DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("initLibrary - Failed reg to PAS dbus interface"));
     pthread_mutex_unlock(&gDbusPendingRegMtx);
     return EPERS_REGISTER_ADMIN;
   }
   else
   {
     DLT_LOG(gPclDLTContext, DLT_LOG_INFO,  DLT_STRING("initLibrary - Successfully established IPC protocol for PCL."));
   }
#else
   DLT_LOG(gPclDLTContext, DLT_LOG_DEBUG, DLT_STRING("PAS interface not enabled, enable with \"./configure --enable-pasinterface\""));
#endif

   if(load_custom_plugins(customAsyncInitClbk) < 0)      // load custom plugins
   {
     DLT_LOG(gPclDLTContext, DLT_LOG_WARN, DLT_STRING("Failed to load custom plugins"));
   }

   init_key_handle_array();

   pers_unlock_access();

   strncpy(gAppId, appName, MaxAppNameLen);  // assign application name
   gAppId[MaxAppNameLen-1] = '\0';

   return rval;
}



int pclDeinitLibrary(void)
{
   int rval = 1;

   pthread_mutex_lock(&gInitMutex);

   if(gPclInitCounter == 1)
   {
      DLT_LOG(gPclDLTContext, DLT_LOG_INFO, DLT_STRING("pclDeinitLibrary - DEINIT  client lib - "), DLT_STRING(gAppId),
                                            DLT_STRING("- init counter: "), DLT_INT(gPclInitCounter));
      rval = private_pclDeinitLibrary();

      gPclInitCounter--;   // decrement init counter
      DLT_UNREGISTER_CONTEXT(gPclDLTContext);
   }
   else if(gPclInitCounter > 1)
   {
      DLT_LOG(gPclDLTContext, DLT_LOG_INFO, DLT_STRING("pclDeinitLibrary - DEINIT client lib - "), DLT_STRING(gAppId),
                                           DLT_STRING("- ONLY DECREMENT init counter: "), DLT_INT(gPclInitCounter));
      gPclInitCounter--;   // decrement init counter
   }
   else
   {
    DLT_LOG(gPclDLTContext, DLT_LOG_WARN, DLT_STRING("pclDeinitLibrary - DEINIT client lib - "), DLT_STRING(gAppId),
                                          DLT_STRING("- NOT INITIALIZED: "));
      rval = EPERS_NOT_INITIALIZED;
   }

   pthread_mutex_unlock(&gInitMutex);

   return rval;
}

static int private_pclDeinitLibrary(void)
{
   int rval = 1;
   int* retval;

   MainLoopData_u data;
   data.message.cmd = (uint32_t)CMD_QUIT;
   data.message.string[0] = '\0';  // no string parameter, set to 0

   if(gShutdownMode != PCL_SHUTDOWN_TYPE_NONE)  // unregister for lifecycle dbus messages
   {
      rval = unregister_lifecycle(gShutdownMode);
   }

#if USE_PASINTERFACE == 1
   rval = unregister_pers_admin_service();
   if(0 != rval)
 {
   DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("pclDeinitLibrary - Err to de-initialize IPC protocol for PCL."));
 }
   else
   {
     DLT_LOG(gPclDLTContext, DLT_LOG_INFO,  DLT_STRING("pclDeinitLibrary - Succ de-initialized IPC protocol for PCL."));
   }
#endif

   process_prepare_shutdown(Shutdown_Full);           // close all db's and fd's and block access

   deliverToMainloop_NM(&data);                       // send quit command to dbus mainloop

   pthread_join(gMainLoopThread, (void**)&retval);    // wait until the dbus mainloop has ended

   pthread_mutex_unlock(&gDbusPendingRegMtx);

#if USE_FILECACHE
   pfcDeinitCache();
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
         DLT_LOG(gPclDLTContext, DLT_LOG_INFO, DLT_STRING("lifecycleSet - PCL_SHUTDOWN -"), DLT_STRING(gAppId));
         process_prepare_shutdown(Shutdown_Partial);
         gCancelCounter++;
      }
      else if(shutdown == PCL_SHUTDOWN_CANCEL)
      {
         DLT_LOG(gPclDLTContext, DLT_LOG_INFO, DLT_STRING("lifecycleSet - PCL_SHUTDOWN_CANCEL -"), DLT_STRING(gAppId), DLT_STRING(" Cancel Counter - "), DLT_INT(gCancelCounter));
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



void pcl_test_send_shutdown_command()
{
   const char* command = {"snmpset -v1 -c public 134.86.58.225 iso.3.6.1.4.1.1909.22.1.1.1.5.1 i 1"};

   if(system(command) == -1)
   {
      printf("Failed to send shutdown command!!!!\n");
   }
}


