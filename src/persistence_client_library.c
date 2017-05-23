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
#include <dlt.h>
#include <dirent.h>
#include <ctype.h>


/// debug log and trace (DLT) setup
DLT_DECLARE_CONTEXT(gPclDLTContext);


/// global variable to store lifecycle shutdown mode
static int gShutdownMode = 0;
/// global shutdown cancel counter
static int gCancelCounter = 0;

static pthread_mutex_t gInitMutex = PTHREAD_MUTEX_INITIALIZER;

/// name of the backup blacklist file (contains all the files which are excluded from backup creation)
static const char* gBackupFilename = "BackupFileList.info";
static const char* gNsmAppId = "NodeStateManager";
static const char* gShmWtNameTemplate = "_Data_mnt_c_%s";
static const char* gShmCNameTemplate  = "_Data_mnt_wt_%s";

static char gAppFolder[PERS_ORG_MAX_LENGTH_PATH_FILENAME] = {0};


#if USE_APPCHECK
/// global flag
static int gAppCheckFlag = -1;

static char gRctFilename[PERS_ORG_MAX_LENGTH_PATH_FILENAME] = {0};

#endif


int customAsyncInitClbk(int errcode)
{
  //printf("Dummy async init Callback: %d\n", errcode);
   (void)errcode;

  return 1;
}

// forward declaration
static int private_pclInitLibrary(const char* appName, int shutdownMode);
static int private_pclDeinitLibrary(void);


/* security check for valid application:
   if the RCT table exists, the application is proven to be valid (trusted),
   otherwise return EPERS_NOPRCTABLE  */
#if USE_APPCHECK
static void doInitAppcheck(const char* appName)
{
   // no need for NULL ptr check for appName, already done in calling function

   snprintf(gRctFilename, PERS_ORG_MAX_LENGTH_PATH_FILENAME, getLocalWtPathKey(), appName, plugin_gResTableCfg);

   if(access(gRctFilename, F_OK) == 0)
   {
      gAppCheckFlag = 1;   // "trusted" application
      DLT_LOG(gPclDLTContext, DLT_LOG_INFO, DLT_STRING("initLibrary - app check: "), DLT_STRING(appName), DLT_STRING("trusted app"));
   }
   else
   {
      gAppCheckFlag = 0;   // currently not a "trusted" application
      DLT_LOG(gPclDLTContext, DLT_LOG_INFO, DLT_STRING("initLibrary - app check: "), DLT_STRING(appName), DLT_STRING("NOT trusted app"));
   }
}
#endif


#if USE_APPCHECK
int doAppcheck(void)
{
   int trusted = 1;

   if(gAppCheckFlag != 1)
   {
      if(access(gRctFilename, F_OK) == 0)
      {
         gAppCheckFlag = 1;   // "trusted" application
      }
      else
      {
         gAppCheckFlag = 0;   // not a "trusted" application
         trusted = 0;
      }
   }
   return trusted;
}
#endif


#define FILE_DIR_NOT_SELF_OR_PARENT(s) ((s)[0]!='.'&&(((s)[1]!='.'||(s)[2]!='\0')||(s)[1]=='\0'))


char* makeShmName(const char* path)
{
   size_t pathLen = strlen(path);
   char* result = (char*)malloc(pathLen+1);   //free happens in checkLocalArtefacts
   int i =0;

   if(result != NULL)
   {
      memset(result, 0, pathLen+1);
      for(i = 0; i < pathLen; i++)
      {
         if(!isalnum(path[i]))
         {
            result[i] = '_';
         }
         else
         {
            result[i] = path[i];
         }
      }
      result[i] = '\0';
   }
   else
   {
      result = NULL;
   }
   return result;
}



void checkLocalArtefacts(const char* thePath, const char* appName)
{
   struct dirent *dirent = NULL;

   if(thePath != NULL && appName != NULL)
   {
      char* name = makeShmName(appName);

      if(name != NULL)
      {
         DIR *dir = opendir(thePath);
         if(NULL != dir)
         {
            for(dirent = readdir(dir); NULL != dirent; dirent = readdir(dir))
            {
               if(FILE_DIR_NOT_SELF_OR_PARENT(dirent->d_name))
               {
                  char shmWtBuffer[128] = {0};
                  char shmCBuffer[128] = {0};

                  memset(shmWtBuffer, 0, 128);
                  memset(shmCBuffer, 0, 128);

                  snprintf(shmWtBuffer, 128, gShmWtNameTemplate, name);
                  snprintf(shmCBuffer,  128, gShmCNameTemplate,  name);

                  if(   strstr(dirent->d_name, shmWtBuffer)
                     || strstr(dirent->d_name, shmCBuffer) )
                  {
                     size_t len = strlen(thePath) + strlen(dirent->d_name)+1;
                     char* fileName = malloc(len);

                     if(fileName != NULL)
                     {
                        snprintf(fileName, len, "%s%s", thePath, dirent->d_name);
                        remove(fileName);

                        DLT_LOG(gPclDLTContext, DLT_LOG_WARN, DLT_STRING("pclInitLibrary => remove sem + shmem:"), DLT_STRING(fileName));
                        free(fileName);
                     }
                  }
               }
            }
            closedir(dir);
         }
         free(name);
      }
   }
}



int pclInitLibrary(const char* appName, int shutdownMode)
{
   int rval = 1;

   int lock = pthread_mutex_lock(&gInitMutex);
   if(lock == 0)
   {
      if(appName != NULL && strlen(appName) > 0 && strlen(appName) < 256)
      {
         if(gPclInitCounter == 0)
         {
            DLT_REGISTER_CONTEXT(gPclDLTContext,"PCL","Ctx for PCL Logging");

            DLT_LOG(gPclDLTContext, DLT_LOG_INFO, DLT_STRING("pclInitLibrary => App:"), DLT_STRING(appName),
                                    DLT_STRING("- init counter: "), DLT_UINT(gPclInitCounter) );

            // do check if there are remaining shared memory and semaphores for local app
            // (only when PCO key-value-store database backend is beeing used)
            checkLocalArtefacts("/dev/shm/", appName);
            //checkGroupArtefacts("/dev/shm", "group_");

            rval = private_pclInitLibrary(appName, shutdownMode);
            if(rval >= 0)
            {
               gPclInitCounter++;     // increment after private init, otherwise atomic access is too early
            }
         }
         else
         {
            DLT_LOG(gPclDLTContext, DLT_LOG_INFO, DLT_STRING("pclInitLibrary - App:"), DLT_STRING(gAppId),
                                                  DLT_STRING("- ONLY INCREMENT init counter: "), DLT_UINT(gPclInitCounter) );

            gPclInitCounter++;     // increment after private init, otherwise atomic access is too early
         }
      }
      else
      {
         DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("pclInitLibrary - appName invalid"));
         rval = EPERS_COMMON;
      }

      pthread_mutex_unlock(&gInitMutex);
   }
   else
   {
     DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("pclInitLibrary - mutex lock failed:"), DLT_INT(lock));
   }

   return rval;
}

static int private_pclInitLibrary(const char* appName, int shutdownMode)
{
   // no need for NULL ptr check for appName, already done in calling function

   int rval = 1, pasRegStatus = -1;

   char blacklistPath[PERS_ORG_MAX_LENGTH_PATH_FILENAME] = {0};

   gShutdownMode = shutdownMode;

#if USE_FSYNC
   DLT_LOG(gPclDLTContext, DLT_LOG_INFO, DLT_STRING("Using fsync version"));
#else
   DLT_LOG(gPclDLTContext, DLT_LOG_INFO, DLT_STRING("Using datasync version (DEFAULT)"));
#endif

#if USE_FILECACHE
   DLT_LOG(gPclDLTContext, DLT_LOG_INFO, DLT_STRING("Using the filecache"));
   pfcInitCache(appName);
#endif

   if(gDbusMainloopRunning == 0) // check if dbus has been already initialized
   {
      if(setup_dbus_mainloop() == -1)
      {
        DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("initLibrary - Failed to setup main loop"));
        return EPERS_DBUS_MAINLOOP;
      }
      gDbusMainloopRunning = 1;
   }

#if USE_PASINTERFACE
   DLT_LOG(gPclDLTContext, DLT_LOG_INFO, DLT_STRING("PAS interface is enabled!!"));

   pasRegStatus = register_pers_admin_service();

   if(pasRegStatus == -1)
   {
     DLT_LOG(gPclDLTContext, DLT_LOG_WARN, DLT_STRING("initLibrary - Failed reg to PAS dbus interface"));
   }
   else if(pasRegStatus < -1)
   {
      DLT_LOG(gPclDLTContext, DLT_LOG_INFO,  DLT_STRING("initLibrary - registration to PAS currently not possible."));
      return EPERS_NO_REG_TO_PAS;
   }
   else
   {
     DLT_LOG(gPclDLTContext, DLT_LOG_INFO,  DLT_STRING("initLibrary - Successfully established IPC protocol for PCL."));
     gPasRegistered = 1;   // remember registration to PAS
   }
#else
   DLT_LOG(gPclDLTContext, DLT_LOG_INFO, DLT_STRING("PAS interface not enabled, enable with \"./configure --enable-pasinterface\""));
#endif

   strncpy(gAppId, appName, PERS_RCT_MAX_LENGTH_RESPONSIBLE);  // assign application name
   gAppId[PERS_RCT_MAX_LENGTH_RESPONSIBLE-1] = '\0';

   if(strcmp(appName, gNsmAppId)  != 0)   // check for NodeStateManager
   {
      // get and fd to the app folder, needed to call syncfs when cmd CMD_LC_PREPARE_SHUTDOWN is called
      // (commit buffer cache to disk)
      // only if not NSM ==> if NSM ha an handle to the folder, it may interfere with PAS installation sequence
      memset(gAppFolder, 0, PERS_ORG_MAX_LENGTH_PATH_FILENAME-1);
      snprintf(gAppFolder, PERS_ORG_MAX_LENGTH_PATH_FILENAME, "/Data/mnt-c/%s/", appName);
      gAppFolder[PERS_ORG_MAX_LENGTH_PATH_FILENAME-1] = '\0';

      gSyncFd = open(gAppFolder, O_RDONLY);
      if(gSyncFd == -1)
      {
         DLT_LOG(gPclDLTContext, DLT_LOG_WARN, DLT_STRING("Failed to open syncfd:"), DLT_STRING(strerror(errno)));
      }
   }
   else
   {
      gIsNodeStateManager = 1;
   }

   // Assemble backup blacklist path
   snprintf(blacklistPath, PERS_ORG_MAX_LENGTH_PATH_FILENAME, "%s%s/%s", CACHEPREFIX, appName, gBackupFilename);

   if(readBlacklistConfigFile(blacklistPath) == -1)
   {
     DLT_LOG(gPclDLTContext, DLT_LOG_INFO, DLT_STRING("initLibrary - Err access blacklist:"), DLT_STRING(blacklistPath));
   }


   if(gShutdownMode != PCL_SHUTDOWN_TYPE_NONE)
   {
     if(register_lifecycle(shutdownMode) == -1) // register for lifecycle dbus messages
     {
       DLT_LOG(gPclDLTContext, DLT_LOG_WARN, DLT_STRING("initLibrary => Failed reg to LC dbus interface"));
     }
   }

   if((rval = load_custom_plugins(customAsyncInitClbk)) < 0)      // load custom plugins
   {
     DLT_LOG(gPclDLTContext, DLT_LOG_WARN, DLT_STRING("Failed to load custom plugins"));
     return rval;
   }

   init_key_handle_array();

#if USE_APPCHECK
   doInitAppcheck(appName);      // check if we have a trusted application
#endif

   pers_unlock_access();

   return rval;
}



int pclDeinitLibrary(void)
{
   int rval = 1;

   int lock = pthread_mutex_lock(&gInitMutex);

   if(lock == 0)
   {
      if(gPclInitCounter == 1)
      {
         DLT_LOG(gPclDLTContext, DLT_LOG_INFO, DLT_STRING("pclDeinitLibrary - DEINIT  client lib - "), DLT_STRING(gAppId),
                                               DLT_STRING("- init counter: "), DLT_UINT(gPclInitCounter));
         rval = private_pclDeinitLibrary();

         gDbusMainloopRunning = 0;
         gPclInitCounter--;         // decrement init counter
         DLT_UNREGISTER_CONTEXT(gPclDLTContext);
      }
      else if(gPclInitCounter > 1)
      {
         DLT_LOG(gPclDLTContext, DLT_LOG_INFO, DLT_STRING("pclDeinitLibrary - DEINIT client lib - "), DLT_STRING(gAppId),
                                              DLT_STRING("- ONLY DECREMENT init counter: "), DLT_UINT(gPclInitCounter));
         gPclInitCounter--;   // decrement init counter
      }
      else
      {
       DLT_LOG(gPclDLTContext, DLT_LOG_WARN, DLT_STRING("pclDeinitLibrary - DEINIT client lib - "), DLT_STRING(gAppId),
                                             DLT_STRING("- NOT INITIALIZED: "));
         rval = EPERS_NOT_INITIALIZED;
      }

      pthread_mutex_unlock(&gInitMutex);
   }

   return rval;
}

static int private_pclDeinitLibrary(void)
{
   int rval = 1;
   int* retval;

   MainLoopData_u data;

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
      gPasRegistered = 0;
   }
#endif

   memset(&data, 0, sizeof(MainLoopData_u));
   data.cmd = (uint32_t)CMD_LC_PREPARE_SHUTDOWN;
   data.params[0] = Shutdown_Full;        // shutdown full
   data.params[1] = 0;                    // internal prepare shutdown
   data.string[0] = '\0';                 // no string parameter, set to 0
   deliverToMainloop_NM(&data);           // send quit command to dbus mainloop


   memset(&data, 0, sizeof(MainLoopData_u));
   data.cmd = (uint32_t)CMD_QUIT;
   data.string[0] = '\0';           // no string parameter, set to 0

   deliverToMainloop_NM(&data);                       // send quit command to dbus mainloop

   pthread_join(gMainLoopThread, (void**)&retval);    // wait until the dbus mainloop has ended

   deleteHandleTrees();                               // delete allocated trees
   deleteBackupTree();
   deleteNotifyTree();

#if USE_FILECACHE
   pfcDeinitCache();
#endif

   if(gIsNodeStateManager == 0)
      close(gSyncFd);

   return rval;
}



int pclLifecycleSet(int shutdown)
{
   int rval = 0;

   if(gShutdownMode == PCL_SHUTDOWN_TYPE_NONE)
   {
      if(shutdown == PCL_SHUTDOWN)
      {
         MainLoopData_u data;

         DLT_LOG(gPclDLTContext, DLT_LOG_INFO, DLT_STRING("lifecycleSet - PCL_SHUTDOWN -"), DLT_STRING(gAppId));

         memset(&data, 0, sizeof(MainLoopData_u));
         data.cmd = (uint32_t)CMD_LC_PREPARE_SHUTDOWN;
         data.params[0] = Shutdown_Partial;     // shutdown partial
         data.params[1] = 0;                    // internal prepare shutdown
         data.string[0] = '\0';                 // no string parameter, set to 0
         deliverToMainloop_NM(&data);           // send quit command to dbus mainloop

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
      DLT_LOG(gPclDLTContext, DLT_LOG_WARN, DLT_STRING("lifecycleSet - not allowed, type not PCL_SHUTDOWN_TYPE_NONE"));
      rval = EPERS_SHUTDOWN_NO_PERMIT;
   }

   return rval;
}


#if 0
void pcl_test_send_shutdown_command()
{
   const char* command = {"snmpset -v1 -c public 134.86.58.225 iso.3.6.1.4.1.1909.22.1.1.1.5.1 i 1"};

   if(system(command) == -1)
   {
      printf("Failed to send shutdown command!!!!\n");
   }
}
#endif

