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
#include "persistence_client_library.h"

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dlfcn.h>

// array containing the id of the custom arrays
static int gCustomLibIdArray[PersCustomLib_LastEntry];

/// array with custom client library names
static char gCustomLibArray[PersCustomLib_LastEntry][CustLibMaxLen];
// number of libraries loaded
static int gNumOfCustomLibraries = 0;


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
         printf("custom_libname_to_id - error - id not found for lib: %s \n", lib_name);
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
         printf("custom_libname_to_id - error - id not found for lib: %s \n", lib_name);
      }

   }
   return libId;
}



int get_custom_libraries()
{
   int rval = 0,
         fd = 0,
          i = 0;

   struct stat buffer;
   char* delimiters = " \n";   // search for blank and end of line
   char* configFileMap = 0;
   char* token = 0;
   const char *filename = getenv("PERS_CLIENT_LIB_CUSTOM_LOAD");

   if(filename == NULL)
   {
      filename = "customLibConfigFile.cfg";  // use default filename
   }

   if(stat(filename, &buffer) != -1)
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
               gCustomLibIdArray[libId] = i;
            }
            else
            {
                return EPERS_OUTOFBOUNDS; // out of array bounds
            }

            // get the library name
            token  = strtok (NULL, delimiters);
            strncpy(gCustomLibArray[i++], token, CustLibMaxLen);

            while( token != NULL )
            {
               // get the library identifier (early, secure, emergency, ...)
               token = strtok(NULL, delimiters);
               if(token != NULL)
               {
                  libId = custom_client_name_to_id(token, 0);
                  if(libId < PersCustomLib_LastEntry)
                  {
                     gCustomLibIdArray[libId] = i;
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
                  strncpy(gCustomLibArray[i++], token, CustLibMaxLen);
               }
               else
               {
                  break;
               }
            }
            gNumOfCustomLibraries = i;    // remember the number of loaded libraries

            munmap(configFileMap, buffer.st_size);
            close(fd);

            // debugging only
/*          printf("get_custom_libraries - found [ %d ] libraries \n", gNumOfCustomLibraries);
            for(i=0; i< gNumOfCustomLibraries; i++)
               printf("get_custom_libraries - names: %s\n", gCustomLibArray[i]);

            for(i=0; i<PersCustomLib_LastEntry; i++)
               printf("get_custom_libraries - id: %d | pos: %d \n", i, gCustomLibIdArray[i]); */
         }
         else
         {
            rval = EPERS_CONFIGMAPFAILED;
            printf("load config file error - mapping of file failed");
         }
      }
      else
      {
         rval = EPERS_CONFIGNOTAVAILABLE;
         printf("load config file error - no file with plugins available -> filename: %s | error: %s \n", filename, strerror(errno) );
      }
   }
   else
   {
      rval = EPERS_CONFIGNOSTAT;
      printf("load config file error - can't stat config file: %s | %s \n", filename, strerror(errno));
   }
   return rval;
}



int load_custom_library(PersistenceCustomLibs_e customLib, Pers_custom_functs_s *customFuncts)
{
   int rval = 0;
   char *error;

   if(customLib < PersCustomLib_LastEntry)
   {
      void* handle = dlopen(gCustomLibArray[customLib], RTLD_LAZY);
      customFuncts->handle = handle;
      if(handle != NULL)
      {
         dlerror();    // reset error

         // plugin_close
         *(void **) (&customFuncts->custom_plugin_handle_close) = dlsym(handle, "plugin_handle_close");
         if ((error = dlerror()) != NULL)
         {
              printf("load_custom_library - error: %s\n", error);
         }
         // custom_plugin_delete_data
         *(void **) (&customFuncts->custom_plugin_delete_data) = dlsym(handle, "plugin_delete_data");
         if ((error = dlerror()) != NULL)
         {
              printf("load_custom_library - error: %s\n", error);
          }
         // custom_plugin_get_data
         *(void **) (&customFuncts->custom_plugin_handle_get_data) = dlsym(handle, "plugin_handle_get_data");
         if ((error = dlerror()) != NULL)
         {
              printf("load_custom_library - error: %s\n", error);

          }
         // custom_plugin_get_data
         *(void **) (&customFuncts->custom_plugin_get_data) = dlsym(handle, "plugin_get_data");
         if ((error = dlerror()) != NULL)
         {
              printf("load_custom_library - error: %s\n", error);
          }
         // custom_plugin_init
         *(void **) (&customFuncts->custom_plugin_init) = dlsym(handle, "plugin_init");
         if ((error = dlerror()) != NULL)
         {
              printf("load_custom_library - error: %s\n", error);

          }
         // custom_plugin_deinit
         *(void **) (&customFuncts->custom_plugin_deinit) = dlsym(handle, "plugin_deinit");
         if ((error = dlerror()) != NULL)
         {
              printf("load_custom_library - error: %s\n", error);
          }
         // custom_plugin_open
         *(void **) (&customFuncts->custom_plugin_handle_open) = dlsym(handle, "plugin_handle_open");
         if ((error = dlerror()) != NULL)
         {
              printf("load_custom_library - error: %s\n", error);
          }
         // custom_plugin_set_data
         *(void **) (&customFuncts->custom_plugin_handle_set_data) = dlsym(handle, "plugin_handle_set_data");
         if ((error = dlerror()) != NULL)
         {
              printf("load_custom_library - error: %s\n", error);
          }
         // custom_plugin_set_data
         *(void **) (&customFuncts->custom_plugin_set_data) = dlsym(handle, "plugin_set_data");
         if ((error = dlerror()) != NULL)
         {
              printf("load_custom_library - error: %s\n", error);
         }
         // custom_plugin_get_size_handle
         *(void **) (&customFuncts->custom_plugin_handle_get_size) = dlsym(handle, "plugin_get_size_handle");
         if ((error = dlerror()) != NULL)
         {
            printf("load_custom_library - error: %s\n", error);
         }
         // custom_plugin_get_size
         *(void **) (&customFuncts->custom_plugin_get_size) = dlsym(handle, "plugin_get_size");
         if ((error = dlerror()) != NULL)
         {
            printf("load_custom_library - error: %s\n", error);
         }
          // create backup
         *(void **) (&customFuncts->custom_plugin_create_backup) = dlsym(handle, "plugin_create_backup");
         if ((error = dlerror()) != NULL)
         {
            printf("load_custom_library - error: %s\n", error);
         }
         // restore backup
         *(void **) (&customFuncts->custom_plugin_restore_backup) = dlsym(handle, "plugin_restore_backup");
         if ((error = dlerror()) != NULL)
         {
             printf("load_custom_library - error: %s\n", error);
         }
         // restore backup
         *(void **) (&customFuncts->custom_plugin_get_backup) = dlsym(handle, "plugin_get_backup");
         if ((error = dlerror()) != NULL)
         {
             printf("load_custom_library - error: %s\n", error);
         }

         // custom_plugin_get_status_notification_clbk
         *(void **) (&customFuncts->custom_plugin_get_status_notification_clbk) = dlsym(handle, "plugin_get_status_notification_clbk");
         if ((error = dlerror()) != NULL)
         {
              printf("load_custom_library - error: %s\n", error);
          }
      }
      else
      {
         printf("load_custom_library - error: %s\n", dlerror());
         rval = EPERS_DLOPENERROR;
      }
   }

   return rval;
}



int load_all_custom_libraries()
{
   int rval = 0,
          i = 0;

   for(i=0; i<gNumOfCustomLibraries; i++)
   {
      rval = load_custom_library(i, &gPersCustomFuncs[i]);
      if( rval < 0)
      {
         // printf("load_all_custom_libraries - error loading library number [%d] \n", i);
         break;
      }
   }
   return rval;
}


char* get_custom_client_lib_name(int idx)
{
   if(idx < PersCustomLib_LastEntry)
   {
      return gCustomLibArray[idx];
   }
   else
   {
      return NULL;
   }
}

int get_custom_client_position_in_array(PersistenceCustomLibs_e customLibId)
{
   //printf("get_position_in_array - id: %d | position: %d \n", customLibId, gCustomLibIdArray[(int)customLibId]);
   if(customLibId < PersCustomLib_LastEntry)
   {
      return gCustomLibIdArray[(int)customLibId];
   }
   else
   {
      return -1;
   }
}


int get_num_custom_client_libs()
{
   return gNumOfCustomLibraries;
}
