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
 * @file           persistence_client_library_custom_loader.c
 * @ingroup        Persistence client library
 * @author         Ingo Huerner
 * @brief          Implementation of persistence custom loadedr
 * @see
 */

#include "persistence_client_library_custom_loader.h"
#include "persistence_client_library_data_organization.h"

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dlfcn.h>


/// type definition of persistence custom library information
typedef struct sPersCustomLibInfo
{
   char libname[CustLibMaxLen];
   int valid;
} PersCustomLibInfo;


/// array with custom client library names
static PersCustomLibInfo gCustomLibArray[PersCustomLib_LastEntry];


PersistenceCustomLibs_e custom_client_name_to_id(const char* lib_name, int substring)
{
   PersistenceCustomLibs_e libId = PersCustomLib_LastEntry;

   if(substring == 0)
   {
      if(0 == strncmp(lib_name, "early", PersCustomPathSize) )
      {
         libId = PersCustomLib_early;
      }
      else if (0 == strncmp(lib_name, "secure", PersCustomPathSize) )
      {
         libId = PersCustomLib_secure;
      }
      else if (0 == strncmp(lib_name, "emergency", PersCustomPathSize) )
      {
         libId = PersCustomLib_emergency;
      }
      else if (0 == strncmp(lib_name, "hwinfo", PersCustomPathSize) )
      {
         libId = PersCustomLib_HWinfo;
      }
      else if (0 == strncmp(lib_name, "custom1", PersCustomPathSize) )
      {
         libId = PersCustomLib_Custom1;
      }
      else if (0 == strncmp(lib_name, "custom2", PersCustomPathSize) )
      {
         libId = PersCustomLib_Custom2;
      }
      else if (0 == strncmp(lib_name, "custom3", PersCustomPathSize) )
      {
         libId = PersCustomLib_Custom3;
      }
      else
      {
         DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("custom_libname_to_id - error - id not found for lib:"), DLT_STRING(lib_name));
      }
   }
   else
   {
      if(NULL != strstr(lib_name, "early") )
      {
         libId = PersCustomLib_early;
      }
      else if (NULL != strstr(lib_name, "secure") )
      {
         libId = PersCustomLib_secure;
      }
      else if (NULL != strstr(lib_name, "emergency") )
      {
         libId = PersCustomLib_emergency;
      }
      else if (NULL != strstr(lib_name, "hwinfo") )
      {
         libId = PersCustomLib_HWinfo;
      }
      else if (NULL != strstr(lib_name, "custom1") )
      {
         libId = PersCustomLib_Custom1;
      }
      else if (NULL != strstr(lib_name, "custom2") )
      {
         libId = PersCustomLib_Custom2;
      }
      else if (NULL != strstr(lib_name, "custom3") )
      {
         libId = PersCustomLib_Custom3;
      }
      else
      {
         DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("custom_libname_to_id - error - id not found for lib:"), DLT_STRING(lib_name));
      }

   }
   return libId;
}



int get_custom_libraries()
{
   int rval = 0, fd = 0, j = 0;

   struct stat buffer;
   char* delimiters = " \n";   // search for blank and end of line
   char* configFileMap = 0;
   char* token = 0;
   const char *filename = getenv("PERS_CLIENT_LIB_CUSTOM_LOAD");

   if(filename == NULL)
   {
      filename = "/etc/pclCustomLibConfigFile.cfg";  // use default filename
   }

   for(j=0; j<PersCustomLib_LastEntry; j++)
   {
      // init pos to -1
      gCustomLibArray[j].valid = -1;
   }


   if(stat(filename, &buffer) != -1)
   {
      if(buffer.st_size > 20)  // file needs to be at least bigger then 20 bytes
      {
         fd = open(filename, O_RDONLY);
         if (fd != -1)
         {
            configFileMap = (char*)mmap(0, buffer.st_size, PROT_WRITE, MAP_PRIVATE, fd, 0);

            if(configFileMap != MAP_FAILED)
            {
               int libId = 0;

               // get the library identifier (early, secure, emergency, ...)
               token = strtok(configFileMap, delimiters);
               libId = custom_client_name_to_id(token, 0);


               if(libId < PersCustomLib_LastEntry)
               {
                  gCustomLibArray[libId].valid = 1;
               }
               else
               {
                  munmap(configFileMap, buffer.st_size); // @CB: Add
                  close(fd); // @CB: Add // close file descriptor before return
                   return EPERS_OUTOFBOUNDS; // out of array bounds
               }

               // get the library name
               token  = strtok (NULL, delimiters);
               strncpy(gCustomLibArray[libId].libname, token, CustLibMaxLen);
               gCustomLibArray[libId].libname[CustLibMaxLen-1] = '\0'; // Ensures 0-Termination

               while( token != NULL )
               {
                  // get the library identifier (early, secure, emergency, ...)
                  token = strtok(NULL, delimiters);
                  if(token != NULL)
                  {
                     libId = custom_client_name_to_id(token, 0);
                     if(libId < PersCustomLib_LastEntry)
                     {
                        gCustomLibArray[libId].valid = 1;
                     }
                     else
                     {
                        rval = EPERS_OUTOFBOUNDS;
                        break;
                     }
                  }
                  else
                  {
                     break;
                  }

                  // get the library name
                  token  = strtok (NULL, delimiters);
                  if(token != NULL)
                  {
                     strncpy(gCustomLibArray[libId].libname, token, CustLibMaxLen);
                     gCustomLibArray[libId].libname[CustLibMaxLen-1] = '\0'; // Ensures 0-Termination
                  }
                  else
                  {
                     break;
                  }
               }

               munmap(configFileMap, buffer.st_size);

               #if 0 // debuging
               for(j=0; j<PersCustomLib_LastEntry; j++)
               {
                  printf("Custom libraries => Name: %s | valid: %d \n", gCustomLibArray[j].libname, gCustomLibArray[j].valid);
               }
               #endif
            }
            else
            {
               rval = EPERS_CONFIGMAPFAILED;
               DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("load config file error - mapping of file failed"));
            }
            close(fd);
         }
         else
         {
            rval = EPERS_CONFIGNOTAVAILABLE;
            DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("load config file error - no file with plugins available:"), DLT_STRING(filename), DLT_STRING(strerror(errno)));
         }
      }
      else
      {
         DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("load config file error - invalid file size"), DLT_STRING(filename), DLT_STRING(strerror(errno)));
      }
   }
   else
   {
      rval = EPERS_CONFIGNOSTAT;
      DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("lload config file error - can't stat config file:"), DLT_STRING(filename), DLT_STRING(strerror(errno)));
   }
   return rval;
}



int load_custom_library(PersistenceCustomLibs_e customLib, Pers_custom_functs_s *customFuncts)
{
   int rval = 1;
   char *error = NULL;

   if(customLib < PersCustomLib_LastEntry)
   {
      void* handle = dlopen(gCustomLibArray[customLib].libname, RTLD_LAZY);
      customFuncts->handle = handle;

      if(handle != NULL)
      {
         dlerror();    // reset error

         // plugin_close
         *(void **) (&customFuncts->custom_plugin_handle_close) = dlsym(handle, "plugin_handle_close");
         if ((error = dlerror()) != NULL)
         {
              DLT_LOG(gPclDLTContext, DLT_LOG_WARN, DLT_STRING("load_custom_library - error:"), DLT_STRING(error));
         }
         // custom_plugin_delete_data
         *(void **) (&customFuncts->custom_plugin_delete_data) = dlsym(handle, "plugin_delete_data");
         if ((error = dlerror()) != NULL)
         {
              DLT_LOG(gPclDLTContext, DLT_LOG_WARN, DLT_STRING("load_custom_library - error:"), DLT_STRING(error));
          }
         // custom_plugin_get_data
         *(void **) (&customFuncts->custom_plugin_handle_get_data) = dlsym(handle, "plugin_handle_get_data");
         if ((error = dlerror()) != NULL)
         {
              DLT_LOG(gPclDLTContext, DLT_LOG_WARN, DLT_STRING("load_custom_library - error:"), DLT_STRING(error));
          }
         // custom_plugin_get_data
         *(void **) (&customFuncts->custom_plugin_get_data) = dlsym(handle, "plugin_get_data");
         if ((error = dlerror()) != NULL)
         {
              DLT_LOG(gPclDLTContext, DLT_LOG_WARN, DLT_STRING("load_custom_library - error:"), DLT_STRING(error));
          }
         // custom_plugin_init
         *(void **) (&customFuncts->custom_plugin_init) = dlsym(handle, "plugin_init");
         if ((error = dlerror()) != NULL)
         {
              DLT_LOG(gPclDLTContext, DLT_LOG_WARN, DLT_STRING("load_custom_library - error:"), DLT_STRING(error));
          }
         // custom_plugin_deinit
         *(void **) (&customFuncts->custom_plugin_deinit) = dlsym(handle, "plugin_deinit");
         if ((error = dlerror()) != NULL)
         {
              DLT_LOG(gPclDLTContext, DLT_LOG_WARN, DLT_STRING("load_custom_library - error:"), DLT_STRING(error));
          }
         // custom_plugin_open
         *(void **) (&customFuncts->custom_plugin_handle_open) = dlsym(handle, "plugin_handle_open");
         if ((error = dlerror()) != NULL)
         {
              DLT_LOG(gPclDLTContext, DLT_LOG_WARN, DLT_STRING("load_custom_library - error:"), DLT_STRING(error));
          }
         // custom_plugin_set_data
         *(void **) (&customFuncts->custom_plugin_handle_set_data) = dlsym(handle, "plugin_handle_set_data");
         if ((error = dlerror()) != NULL)
         {
              DLT_LOG(gPclDLTContext, DLT_LOG_WARN, DLT_STRING("load_custom_library - error:"), DLT_STRING(error));
          }
         // custom_plugin_set_data
         *(void **) (&customFuncts->custom_plugin_set_data) = dlsym(handle, "plugin_set_data");
         if ((error = dlerror()) != NULL)
         {
              DLT_LOG(gPclDLTContext, DLT_LOG_WARN, DLT_STRING("load_custom_library - error:"), DLT_STRING(error));
         }
         // custom_plugin_get_size_handle
         *(void **) (&customFuncts->custom_plugin_handle_get_size) = dlsym(handle, "plugin_handle_get_size");
         if ((error = dlerror()) != NULL)
         {
            DLT_LOG(gPclDLTContext, DLT_LOG_WARN, DLT_STRING("load_custom_library - error:"), DLT_STRING(error));
         }
         // custom_plugin_get_size
         *(void **) (&customFuncts->custom_plugin_get_size) = dlsym(handle, "plugin_get_size");
         if ((error = dlerror()) != NULL)
         {
            DLT_LOG(gPclDLTContext, DLT_LOG_WARN, DLT_STRING("load_custom_library - error:"), DLT_STRING(error));
         }
          // create backup
         *(void **) (&customFuncts->custom_plugin_create_backup) = dlsym(handle, "plugin_create_backup");
         if ((error = dlerror()) != NULL)
         {
            DLT_LOG(gPclDLTContext, DLT_LOG_WARN, DLT_STRING("load_custom_library - error:"), DLT_STRING(error));
         }
         // restore backup
         *(void **) (&customFuncts->custom_plugin_restore_backup) = dlsym(handle, "plugin_restore_backup");
         if ((error = dlerror()) != NULL)
         {
             DLT_LOG(gPclDLTContext, DLT_LOG_WARN, DLT_STRING("load_custom_library - error:"), DLT_STRING(error));
         }
         // restore backup
         *(void **) (&customFuncts->custom_plugin_get_backup) = dlsym(handle, "plugin_get_backup");
         if ((error = dlerror()) != NULL)
         {
             DLT_LOG(gPclDLTContext, DLT_LOG_WARN, DLT_STRING("load_custom_library - error:"), DLT_STRING(error));
         }

         // custom_plugin_get_status_notification_clbk
         *(void **) (&customFuncts->custom_plugin_get_status_notification_clbk) = dlsym(handle, "plugin_get_status_notification_clbk");
         if ((error = dlerror()) != NULL)
         {
              DLT_LOG(gPclDLTContext, DLT_LOG_WARN, DLT_STRING("load_custom_library - error:"), DLT_STRING(error));
          }

         // initialize plugin (non blocking)
         *(void **) (&customFuncts->custom_plugin_init_async) = dlsym(handle, "plugin_init_async");
         if ((error = dlerror()) != NULL)
         {
              DLT_LOG(gPclDLTContext, DLT_LOG_WARN, DLT_STRING("load_custom_library - error:"), DLT_STRING(error));
         }

         // clear all data
         *(void **) (&customFuncts->custom_plugin_clear_all_data) = dlsym(handle, "plugin_clear_all_data");
         if ((error = dlerror()) != NULL)
         {
              DLT_LOG(gPclDLTContext, DLT_LOG_WARN, DLT_STRING("load_custom_library - error:"), DLT_STRING(error));
         }

         // sync data
         *(void **) (&customFuncts->custom_plugin_sync) = dlsym(handle, "plugin_sync");
         if ((error = dlerror()) != NULL)
         {
              DLT_LOG(gPclDLTContext, DLT_LOG_WARN, DLT_STRING("load_custom_library - error:"), DLT_STRING(error));
         }
      }
      else
      {
         error = dlerror();
         DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("load_custom_library - error:"), DLT_STRING(error));
         rval = EPERS_DLOPENERROR;
      }
   }
   else
   {
      DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("load_custom_library - error: - customLib out of bounds"));
      rval = EPERS_DLOPENERROR;
   }

   return rval;
}



int load_all_custom_libraries()
{
   int rval = 0,
          i = 0;

   for(i=0; i<PersCustomLib_LastEntry; i++)
   {
      rval = load_custom_library(i, &gPersCustomFuncs[i]);
      if( rval < 0)
      {
         break;
      }
   }
   return rval;
}


char* get_custom_client_lib_name(int idx)
{
   if(idx < PersCustomLib_LastEntry)
   {
      return gCustomLibArray[idx].libname;
   }
   else
   {
      return NULL;
   }
}

//int get_custom_client_position_in_array(int customLibId)
int check_valid_idx(int idx)
{
   int rval = -1;

   if(idx < PersCustomLib_LastEntry)
   {
      rval = gCustomLibArray[idx].valid;
   }

   return rval;
}



void invalidate_custom_plugin(int idx)
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
