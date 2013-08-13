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

#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <dlfcn.h>

#include <dlt/dlt.h>
#include <dlt/dlt_common.h>

#include <dbus/dbus.h>

#define USE_DBUS 1

/// debug log and trace (DLT) setup
DLT_DECLARE_CONTEXT(gDLTContext);

static int gShutdownMode = 0;

/// loical function declaration
void invalidateCustomPlugin(int idx);

int pclInitLibrary(const char* appName, int shutdownMode)
{
   int status = 0;
   int i = 0, rval = 1;

   if(gPclInitialized == PCLnotInitialized)
   {
      gPclInitialized++;

      gShutdownMode = shutdownMode;

      DLT_REGISTER_CONTEXT(gDLTContext,"pers","Context for persistence client library logging");
      DLT_LOG(gDLTContext, DLT_LOG_INFO, DLT_STRING("pclInitLibrary => I N I T  Persistence Client Library - "), DLT_STRING(gAppId),
                           DLT_STRING("- init counter: "), DLT_INT(gPclInitialized) );

      /// environment variable for on demand loading of custom libraries
      const char *pOnDemandLoad = getenv("PERS_CUSTOM_LIB_LOAD_ON_DEMAND");

      /// environment variable for max key value data
      const char *pDataSize = getenv("PERS_MAX_KEY_VAL_DATA_SIZE");

      if(pDataSize != NULL)
      {
         gMaxKeyValDataSize = atoi(pDataSize);
      }

      if( setup_dbus_mainloop() == -1)
      {
         DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("pclInitLibrary => Failed to setup main loop"));
         return -1;
      }

#if USE_DBUS
      // register for lifecycle and persistence admin service dbus messages
      if(register_lifecycle(shutdownMode) == -1)
      {
         DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("pclInitLibrary => Failed to register to lifecycle dbus interface"));
         return -1;
      }

      rval = register_pers_admin_service();
      {
         DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("pclInitLibrary => Failed to register to pers admin dbus interface"));
         return -1;
      }
#endif

      /// get custom library names to load
      status = get_custom_libraries();
      if(status >= 0)
      {
         // initialize custom library structure
         for(i = 0; i < PersCustomLib_LastEntry; i++)
         {
            invalidateCustomPlugin(i);
         }

         if(pOnDemandLoad == NULL)  // load all available libraries now
         {
            for(i=0; i < PersCustomLib_LastEntry; i++ )
            {
               if(check_valid_idx(i) != -1)
               {
                  if(load_custom_library(i, &gPersCustomFuncs[i] ) == 1)
                  {
                     if( (gPersCustomFuncs[i].custom_plugin_init) != NULL)
                     {
                        DLT_LOG(gDLTContext, DLT_LOG_INFO, DLT_STRING("pclInitLibrary => Loaded plugin: "),
                                                           DLT_STRING(get_custom_client_lib_name(i)));
                        gPersCustomFuncs[i].custom_plugin_init();
                     }
                     else
                     {
                        DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("pclInitLibrary => E r r o r could not load plugin functions: "),
                                                            DLT_STRING(get_custom_client_lib_name(i)));
                     }
                  }
                  else
                  {
                     DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("pclInitLibrary => E r r o r could not load plugin: "),
                                          DLT_STRING(get_custom_client_lib_name(i)));
                  }
               }
               else
               {
                  continue;
               }
            }
         }
      }
      else
      {
         DLT_LOG(gDLTContext, DLT_LOG_WARN, DLT_STRING("pclInit => Failed to load custom library config table => error number:"), DLT_INT(status));
      }


      // assign application name
      strncpy(gAppId, appName, MaxAppNameLen);
      gAppId[MaxAppNameLen-1] = '\0';

      // destory mutex
      pthread_mutex_destroy(&gDbusInitializedMtx);
      pthread_cond_destroy(&gDbusInitializedCond);

      rval = 1;
   }
   else if(gPclInitialized >= PCLinitialized)
   {
      gPclInitialized++; // increment init counter
      DLT_LOG(gDLTContext, DLT_LOG_INFO, DLT_STRING("pclInitLibrary => I N I T  Persistence Client Library - "), DLT_STRING(gAppId),
                           DLT_STRING("- ONLY INCREMENT init counter: "), DLT_INT(gPclInitialized) );
   }

   return rval;
}



int pclDeinitLibrary(void)
{
   int i = 0, rval = 1;

   if(gPclInitialized == PCLinitialized)
   {
      DLT_LOG(gDLTContext, DLT_LOG_INFO, DLT_STRING("pclDeinitLibrary -> D E I N I T  client library - "), DLT_STRING(gAppId),
                                           DLT_STRING("- init counter: "), DLT_INT(gPclInitialized));

      // unregister for lifecycle and persistence admin service dbus messages
   #if USE_DBUS
      rval = unregister_lifecycle(gShutdownMode);
      rval = unregister_pers_admin_service();
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

            invalidateCustomPlugin(i);
         }
      }

      gPclInitialized = PCLnotInitialized;

      DLT_UNREGISTER_CONTEXT(gDLTContext);
   }
   else if(gPclInitialized > PCLinitialized)
   {
      DLT_LOG(gDLTContext, DLT_LOG_INFO, DLT_STRING("pclDeinitLibrary -> D E I N I T  client library - "), DLT_STRING(gAppId),
                                           DLT_STRING("- ONLY DECREMENT init counter: "), DLT_INT(gPclInitialized));
      gPclInitialized--;   // decrement init counter
   }

   return rval;
}


void invalidateCustomPlugin(int idx)
{
   gPersCustomFuncs[idx].handle  = NULL;
   gPersCustomFuncs[idx].custom_plugin_init = NULL;
   gPersCustomFuncs[idx].custom_plugin_deinit = NULL;
   gPersCustomFuncs[idx].custom_plugin_handle_open = NULL;
   gPersCustomFuncs[idx].custom_plugin_handle_close = NULL;
   gPersCustomFuncs[idx].custom_plugin_handle_get_data = NULL;
   gPersCustomFuncs[idx].custom_plugin_handle_set_data  = NULL;
   gPersCustomFuncs[idx].custom_plugin_get_data = NULL;
   gPersCustomFuncs[idx].custom_plugin_set_data = NULL;
   gPersCustomFuncs[idx].custom_plugin_delete_data = NULL;
   gPersCustomFuncs[idx].custom_plugin_get_status_notification_clbk = NULL;
   gPersCustomFuncs[idx].custom_plugin_handle_get_size = NULL;
   gPersCustomFuncs[idx].custom_plugin_get_size = NULL;
   gPersCustomFuncs[idx].custom_plugin_create_backup = NULL;
   gPersCustomFuncs[idx].custom_plugin_get_backup = NULL;
   gPersCustomFuncs[idx].custom_plugin_restore_backup = NULL;
}





