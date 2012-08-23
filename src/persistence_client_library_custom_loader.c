/******************************************************************************
 * Project         Persistency
 * (c) copyright   2012
 * Company         XS Embedded GmbH
 *****************************************************************************/
/******************************************************************************
   Permission is hereby granted, free of charge, to any person obtaining
   a copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included
   in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
   DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
   TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
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


PersistenceCustomLibs_e custom_libname_to_id(const char* lib_name)
{
   PersistenceCustomLibs_e libId = PersCustomLib_LastEntry;

   if(0 == strcmp(lib_name, "early") )
   {
      libId = PersCustomLib_early;
   }
   else if (0 == strcmp(lib_name, "secure") )
   {
      libId = PersCustomLib_secure;
   }
   else if (0 == strcmp(lib_name, "emergency") )
   {
      libId = PersCustomLib_emergency;
   }
   else if (0 == strcmp(lib_name, "hwinfo") )
   {
      libId = PersCustomLib_HWinfo;
   }
   else if (0 == strcmp(lib_name, "custom1") )
   {
      libId = PersCustomLib_Custom1;
   }
   else if (0 == strcmp(lib_name, "custom2") )
   {
      libId = PersCustomLib_Custom2;
   }
   else if (0 == strcmp(lib_name, "custom3") )
   {
      libId = PersCustomLib_Custom3;
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
            libId = custom_libname_to_id(token);
            gCustomLibIdArray[libId] = i;

            // get the library name
            token  = strtok (NULL, delimiters);
            strncpy(gCustomLibArray[i++], token, CustLibMaxLen);

            while( token != NULL )
            {
               // get the library identifier (early, secure, emergency, ...)
               token = strtok(NULL, delimiters);
               if(token != NULL)
               {
                  libId = custom_libname_to_id(token);
                  gCustomLibIdArray[libId] = i;
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
/*
            printf("get_custom_libraries - found [ %d ] libraries \n", gNumOfCustomLibraries);
            for(i=0; i< gNumOfCustomLibraries; i++)
               printf("get_custom_libraries - names: %s\n", gCustomLibArray[i]);

            for(i=0; i<PersCustomLib_LastEntry; i++)
               printf("get_custom_libraries - id: %d | pos: %d \n", i, gCustomLibIdArray[i]);
*/
            munmap(configFileMap, buffer.st_size);
            close(fd);
         }
         else
         {
            printf("load config file error - mapping of file failed");
         }
      }
      else
      {
         printf("load config file error - filename: %s | error: %s \n", filename, strerror(errno) );

         // load default librarys
         // TODO
      }
   }
   else
   {
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
      if(handle != NULL)
      {
         dlerror();    // reset error

         // plugin_close
         *(void **) (&customFuncts->custom_plugin_close) = dlsym(handle, "plugin_close");
         if ((error = dlerror()) != NULL)
         {
              printf("load_custom_library - error: %s\n", error);
              return -1;
         }
         // custom_plugin_delete_data
         *(void **) (&customFuncts->custom_plugin_delete_data) = dlsym(handle, "plugin_delete_data");
         if ((error = dlerror()) != NULL)
         {
              printf("load_custom_library - error: %s\n", error);
              return -1;
          }
         // custom_plugin_get_data
         *(void **) (&customFuncts->custom_plugin_get_data) = dlsym(handle, "plugin_get_data");
         if ((error = dlerror()) != NULL)
         {
              printf("load_custom_library - error: %s\n", error);
              return -1;
          }
         // custom_plugin_init
         *(void **) (&customFuncts->custom_plugin_init) = dlsym(handle, "plugin_init");
         if ((error = dlerror()) != NULL)
         {
              printf("load_custom_library - error: %s\n", error);
              return -1;
          }
         // custom_plugin_open
         *(void **) (&customFuncts->custom_plugin_open) = dlsym(handle, "plugin_open");
         if ((error = dlerror()) != NULL)
         {
              printf("load_custom_library - error: %s\n", error);
              return -1;
          }
         // custom_plugin_set_data
         *(void **) (&customFuncts->custom_plugin_set_data) = dlsym(handle, "plugin_set_data");
         if ((error = dlerror()) != NULL)
         {
              printf("load_custom_library - error: %s\n", error);
              return -1;
          }
         // custom_plugin_get_status_notification_clbk
         *(void **) (&customFuncts->custom_plugin_get_status_notification_clbk) = dlsym(handle, "plugin_get_status_notification_clbk");
         if ((error = dlerror()) != NULL)
         {
              printf("load_custom_library - error: %s\n", error);
              return -1;
          }
      }
      else
      {
         //printf("load_custom_library - error: %s\n", dlerror());
         rval = -1;
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
      if( load_custom_library(i, &gPersCustomFuncs[i] ) == -1)
      {
         // printf("load_all_custom_libraries - error loading library number [%d] \n", i);
         rval = -1;
         break;
      }
   }
   return rval;
}



int get_position_in_array(PersistenceCustomLibs_e customLibId)
{
   //printf("get_position_in_array - id: %d | position: %d \n", customLibId, gCustomLibIdArray[(int)customLibId]);
   return gCustomLibIdArray[(int)customLibId];
}


int get_num_custom_client_libs()
{
   return gNumOfCustomLibraries;
}
