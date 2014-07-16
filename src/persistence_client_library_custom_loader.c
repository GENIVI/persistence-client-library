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
   char 						libname[CustLibMaxLen];
   int 						valid;
   PersInitType_e  		initFunction;
   PersLoadingType_e 	loadingType;
} PersCustomLibInfo;


/// array with custom client library names
static PersCustomLibInfo gCustomLibArray[PersCustomLib_LastEntry];
static char* gpCustomTokenArray[TOKENARRAYSIZE];

int(* gPlugin_callback_async_t)(int errcode);

static void fillCustomCharTokenArray(unsigned int customConfigFileSize, char* fileMap)
{
   unsigned int i=0;
   int   customTokenCounter = 0;
   int blankCount=0;
   char* tmpPointer = fileMap;

   // set the first pointer to the start of the file
   gpCustomTokenArray[blankCount] = tmpPointer;
   blankCount++;

   while(i < customConfigFileSize)
   {
      if(1 != gCharLookup[(int)*tmpPointer])
      {
         *tmpPointer = 0;

         // check if we are at the end of the token array
         if(blankCount >= TOKENARRAYSIZE)
         {
            break;
         }
         gpCustomTokenArray[blankCount] = tmpPointer+1;
         blankCount++;
         customTokenCounter++;

      }
      tmpPointer++;
      i++;
   }
}


static PersLoadingType_e getLoadingType(const char* type)
{
	PersLoadingType_e persLoadingType = LoadType_Undefined;

   if(0 == strcmp(type, "init") )
   {
   	persLoadingType = LoadType_PclInit;
   }
   else if(0 == strcmp(type, "od") )
   {
   	persLoadingType = LoadType_OnDemand;
   }

   return persLoadingType;
}


static PersInitType_e getInitType(const char* policy)
{
	PersInitType_e persInitType = Init_Undefined;

   if(0 == strcmp(policy, "sync"))
   {
   	persInitType = Init_Synchronous;
   }
   else if (0 == strcmp(policy, "async"))
   {
   	persInitType = Init_Asynchronous;
   }

   return persInitType;
}


PersLoadingType_e getCustomLoadingType(int i)
{
	return gCustomLibArray[i].loadingType;
}


PersInitType_e getCustomInitType(int i)
{
	return gCustomLibArray[i].initFunction;
}


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
   int rval = 0, j = 0;
   struct stat buffer;
   const char *filename = getenv("PERS_CLIENT_LIB_CUSTOM_LOAD");

	if(filename == NULL)
	{
		filename = "/etc/pclCustomLibConfigFile.cfg";  // use default filename
	}

   for(j=0; j<PersCustomLib_LastEntry; j++)
   {
      gCustomLibArray[j].valid = -1;	// init pos to -1
   }

   memset(&buffer, 0, sizeof(buffer));
   if(stat(filename, &buffer) != -1)
   {
		if(buffer.st_size > 0)	// check for empty file
		{
			char* customConfFileMap = NULL;
			int i = 0;
			int fd = open(filename, O_RDONLY);

			DLT_LOG(gPclDLTContext, DLT_LOG_INFO, DLT_STRING("load custom library config file ==> "), DLT_STRING(filename));

			if (fd == -1)
			{
				DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("load custom library config file error ==> Error file open: "),
						                                 DLT_STRING(filename), DLT_STRING("err msg: "), DLT_STRING(strerror(errno)) );
				return EPERS_COMMON;
			}

			// map the config file into memory
			customConfFileMap = (char*)mmap(0, buffer.st_size, PROT_WRITE, MAP_PRIVATE, fd, 0);

			if (customConfFileMap == MAP_FAILED)
			{
				close(fd);
				DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("load custom library config file error ==> Error mapping the file"));
				return EPERS_COMMON;
			}

			fillCustomCharTokenArray(buffer.st_size, customConfFileMap);

			while( i < TOKENARRAYSIZE )
			{
				if(gpCustomTokenArray[i] != 0 && gpCustomTokenArray[i+1] != 0 && gpCustomTokenArray[i+2] != 0 &&gpCustomTokenArray[i+3] != 0 )
				{
					int libId = custom_client_name_to_id(gpCustomTokenArray[i], 0);	// get the custom libID

					// assign the libraryname
					strncpy(gCustomLibArray[libId].libname, gpCustomTokenArray[i+1], CustLibMaxLen);
					gCustomLibArray[libId].libname[CustLibMaxLen-1] = '\0'; // Ensures 0-Termination

					gCustomLibArray[libId].loadingType  = getLoadingType(gpCustomTokenArray[i+2]);
					gCustomLibArray[libId].initFunction = getInitType(gpCustomTokenArray[i+3]);
					gCustomLibArray[libId].valid        = 1;	// marks as valid;
	#if 0
					// debug
					printf("     1. => %s => %d \n",   gpCustomTokenArray[i],   libId);
					printf("     2. => %s => %s \n",   gpCustomTokenArray[i+1], gCustomLibArray[libId].libname);
					printf("     3. => %s => %d \n",   gpCustomTokenArray[i+2], (int)gCustomLibArray[libId].initFunction);
					printf("     4. => %s => %d \n\n", gpCustomTokenArray[i+3], (int)gCustomLibArray[libId].loadingType);
	#endif
				}
				else
				{
					break;
				}
				i+=4;       // move to the next configuration file entry
			}
			close(fd);
		}
		else
		{
			DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("load custom library config file error ==> Error file size is 0"));
			rval = EPERS_COMMON;
		}
   }
   else
   {
   	DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("load custom library config file error ==> failed to stat() file"));
   	rval = EPERS_COMMON;
   }
   return rval;
}



int load_custom_library(PersistenceCustomLibs_e customLib, Pers_custom_functs_s *customFuncts)
{
   int rval = 1;
   char *error = NULL;

   if(customLib < PersCustomLib_LastEntry)
   {
   	PersInitType_e initType = getCustomInitType(customLib);
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

         //
         // initialize the library
         //
			if(initType == Init_Synchronous)
			{
				if( (gPersCustomFuncs[customLib].custom_plugin_init) != NULL)
				{
					DLT_LOG(gPclDLTContext, DLT_LOG_INFO, DLT_STRING("load_custom_library => (sync)  : "), DLT_STRING(get_custom_client_lib_name(customLib)));
					gPersCustomFuncs[customLib].custom_plugin_init();
				}
				else
				{
					DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("load_custom_library - error: could not load plugin functions: "),
																      DLT_STRING(get_custom_client_lib_name(customLib)));
					rval = EPERS_COMMON;
				}
			}
			else if(initType == Init_Asynchronous)
			{
				if( (gPersCustomFuncs[customLib].custom_plugin_init_async) != NULL)
				{
					DLT_LOG(gPclDLTContext, DLT_LOG_INFO, DLT_STRING("load_custom_library => (async) : "),
							                  DLT_STRING(get_custom_client_lib_name(customLib)));

					gPersCustomFuncs[customLib].custom_plugin_init_async(gPlugin_callback_async_t);
				}
				else
				{
					DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("load_custom_library => error: could not load plugin functions: "),
																	DLT_STRING(get_custom_client_lib_name(customLib)));
					rval = EPERS_COMMON;
				}
			}
			else
			{
				DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("load_custom_library - error: unknown init type "),
						                                 DLT_STRING(get_custom_client_lib_name(customLib)));
				rval = EPERS_COMMON;
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


int load_custom_plugins(plugin_callback_async_t pfInitCompletedCB)
{
	int rval = 0, i = 0;

	/// get custom library names to load
	if(get_custom_libraries() >= 0)
	{
		gPlugin_callback_async_t = pfInitCompletedCB;		// assign init callback

		// initialize custom library structure
		for(i = 0; i < PersCustomLib_LastEntry; i++)
		{
			invalidate_custom_plugin(i);
		}

		for(i=0; i < PersCustomLib_LastEntry; i++ )
		{
			if(check_valid_idx(i) != -1)
			{
				if(getCustomLoadingType(i) == LoadType_PclInit)	// check if the plugin must be loaded on plc init
				{
					if(load_custom_library(i, &gPersCustomFuncs[i] ) <= 0)
					{
						DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("load_custom_plugins => E r r o r could not load plugin: "),
													DLT_STRING(get_custom_client_lib_name(i)));
						rval = EPERS_COMMON;
					}
				}
			}
			else
			{
				continue;
			}
		}
	}
	else
	{
		DLT_LOG(gPclDLTContext, DLT_LOG_WARN, DLT_STRING("pclInit => Failed to load custom library config table"));
		rval = EPERS_COMMON;
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
