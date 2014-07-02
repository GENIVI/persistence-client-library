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
 * @file           persistence_client_library_db_access.c
 * @ingroup        Persistence client library
 * @author         Ingo Huerner
 * @brief          Implementation of persistence database access
 *                 Library provides an API to access persistent data
 * @see            
 */

#include "persistence_client_library_db_access.h"
#include "persistence_client_library_custom_loader.h"
#include "persistence_client_library_dbus_service.h"
#include "persistence_client_library_prct_access.h"

#include <persComErrors.h>
#include <persComDataOrg.h>
#include <persComDbAccess.h>

#include <dbus/dbus.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>



/// btree array
static int gHandlesDB[DbTableSize][PersistencePolicy_LastEntry];
static int gHandlesDBCreated[DbTableSize][PersistencePolicy_LastEntry] = { {0} };


// function prototype
int pers_send_Notification_Signal(const char* key, PersistenceDbContext_s* context, unsigned int reason);


char* pers_get_raw_key(char *key)
{
   char *temp = NULL;
   char *rawKey = key;

   temp = strrchr(key, (int)'/');
   
   if (NULL != temp)
   {
   	  rawKey = temp + 1;
   }

   return rawKey;
}


int pers_db_open_default(const char* dbPath, PersDefaultType_e DefaultType)
{
   int ret = 0;
   char path[DbPathMaxLen] = {0};

   if (PersDefaultType_Configurable == DefaultType)
   {
      snprintf(path, DbPathMaxLen, "%s%s", dbPath, gLocalConfigurableDefault);
   }
   else if (PersDefaultType_Factory== DefaultType)
   {
      snprintf(path, DbPathMaxLen, "%s%s", dbPath, gLocalFactoryDefault);
   }
   else
   {
      DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("pers_db_open_default ==> unknown DefaultType"));
      ret = EPERS_COMMON;
   }

   if (EPERS_COMMON != ret)
   {
      ret = persComDbOpen(path, 0);
      if (ret < 0)
      {
         ret = EPERS_COMMON;
         DLT_LOG(gPclDLTContext, DLT_LOG_WARN,
                              DLT_STRING("pers_db_open_default() -> persComDbOpen() -> problem open db: "),
                              DLT_STRING(path),
                              DLT_STRING(" Code: "),
                              DLT_INT(ret));
      }
   }

   return ret;
}


int pers_get_defaults(char* dbPath, char* key, unsigned char* buffer, unsigned int buffer_size, PersGetDefault_e job)
{
   PersDefaultType_e i = PersDefaultType_Configurable;
   int handleDefaultDB = -1;
   int read_size = EPERS_NOKEY;
   char dltMessage[DbPathMaxLen] = {0};

   // key = pers_get_raw_key(key); /* We need only the raw key without a prefixed '/node/' or '/user/1/seat/0' etc... */

   for(i=0; i<PersDefaultType_LastEntry; i++)
   {
      handleDefaultDB = pers_db_open_default(dbPath, i);
      if(handleDefaultDB >= 0)
      {
         if (PersGetDefault_Data == job)
         {
         	read_size = persComDbReadKey(handleDefaultDB, key, (char*)buffer, buffer_size);
         }
         else if (PersGetDefault_Size == job)
         {
            read_size = persComDbGetKeySize(handleDefaultDB, key);
         }
         else
         {
            DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("pers_get_defaults ==> unknown job"));
            break;
         }

         if (0 > persComDbClose(handleDefaultDB))
         {
            DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("pers_get_defaults ==> persComDbClose returned with error"));
         }

         if(read_size < 0) // check read_size
         {
            if(PERS_COM_ERR_NOT_FOUND == read_size)
            {
               read_size = EPERS_NOKEY;
            }
         }
         else /* read_size >= 0 --> default value found */
         {
            if (PersDefaultType_Configurable == i)
            {
               snprintf(dltMessage, DbPathMaxLen, "%s%s", dbPath, gLocalConfigurableDefault);
            }
            if (PersDefaultType_Factory == i)
            {
                snprintf(dltMessage, DbPathMaxLen, "%s%s", dbPath, gLocalFactoryDefault);
            }
            DLT_LOG(gPclDLTContext, DLT_LOG_INFO, DLT_STRING("Default data will be used for Key"),
                                               DLT_STRING(key),
                                               DLT_STRING("from"),
                                               DLT_STRING(dltMessage));
            break;
         }
      }
   }

   if (read_size < 0)
   {
       DLT_LOG(gPclDLTContext, DLT_LOG_INFO, DLT_STRING("Default data not available for Key"),
                                          DLT_STRING(key),
                                          DLT_STRING("Path:"),
                                          DLT_STRING(dbPath));
   }

   return read_size;
}


static int database_get(PersistenceInfo_s* info, const char* dbPath)
{
   int arrayIdx = 0;
   int handleDB = -1;

   // create array index: index is a combination of resource config table type and group
   arrayIdx = info->configKey.storage + info->context.ldbid ;

   //if(arrayIdx <= DbTableSize)
   if(arrayIdx < DbTableSize)
   {
      if(gHandlesDBCreated[arrayIdx][info->configKey.policy] == 0)
      {

         char path[DbPathMaxLen] = {0};

         if(PersistencePolicy_wt == info->configKey.policy)
         {
            snprintf(path, DbPathMaxLen, "%s%s", dbPath, gLocalWt);
         }
         else if(PersistencePolicy_wc == info->configKey.policy)
         {
            snprintf(path, DbPathMaxLen, "%s%s", dbPath, gLocalCached);
         }
         else
         {
            handleDB = -2;
         }

         if (handleDB == -1)
         {
            handleDB = persComDbOpen(path, 0x01);
            if(handleDB >= 0)
            {
               gHandlesDB[arrayIdx][info->configKey.policy] = handleDB ;
               gHandlesDBCreated[arrayIdx][info->configKey.policy] = 1;
            }
            else
            {
               DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("database_get ==> persComDbOpen() failed"));
            }
         }
         else
         {
            DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("database_get ==> wrong policy! Cannot extend dbPath wit database."));
         }
      }
      else
      {
         handleDB = gHandlesDB[arrayIdx][info->configKey.policy];
      }
   }
   else
   {
      DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("database_get ==> invalid storage type"), DLT_STRING(dbPath));
   }



   return handleDB;
}


void database_close(PersistenceInfo_s* info)
{
   int arrayIdx = info->configKey.storage + info->context.ldbid;

   if(info->configKey.storage <= PersistenceStorage_shared )
   {
      int iErrorCode = persComDbClose(gHandlesDB[arrayIdx][info->configKey.policy]) ;
      if (iErrorCode < 0)
      {
         DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("database_close ==> persComDbClose() failed"));
      }
      else
      {
        gHandlesDBCreated[arrayIdx][info->configKey.policy] = 0;
      }
   }
   else
   {
      DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("database_close ==> invalid storage type"), DLT_INT(info->context.ldbid ));
   }
}

void database_close_all()
{
   int i = 0;

   for(i=0; i<DbTableSize; i++)
   {
      // close write cached database
      if(gHandlesDBCreated[i][PersistencePolicy_wc] == 1)
      {
         int iErrorCode = persComDbClose(gHandlesDB[i][PersistencePolicy_wc]);
         if (iErrorCode < 0)
         {
            DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("database_close_all => failed to close db => persComDbClose"));
         }
         else
         {
             gHandlesDBCreated[i][PersistencePolicy_wc] = 0;
         }
      }

      // close write through database
      if(gHandlesDBCreated[i][PersistencePolicy_wt] == 1)
      {
         int iErrorCode = persComDbClose(gHandlesDB[i][PersistencePolicy_wt]);
         if (iErrorCode < 0)
         {
            DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("database_close_all => failed to close db => persComDbClose"));
         }
         else
         {
            gHandlesDBCreated[i][PersistencePolicy_wt] = 0;
         }
      }
   }
}



int persistence_get_data(char* dbPath, char* key, const char* resourceID, PersistenceInfo_s* info, unsigned char* buffer, unsigned int buffer_size)
{
   int read_size = -1;
   int ret_defaults = -1;

   if(   PersistenceStorage_shared == info->configKey.storage
      || PersistenceStorage_local == info->configKey.storage)
   {
      int handleDB = database_get(info, dbPath);
      if(handleDB >= 0)
      {
         read_size = persComDbReadKey(handleDB, key, (char*)buffer, buffer_size);
         if(read_size < 0)
         {
            read_size = pers_get_defaults(dbPath, (char*)resourceID, buffer, buffer_size, PersGetDefault_Data); /* 0 ==> Get data */
         }
      }
   }
   else if(PersistenceStorage_custom == info->configKey.storage)   // custom storage implementation via custom library
   {
      int idx =  custom_client_name_to_id(dbPath, 1);
      char workaroundPath[128];  // workaround, because /sys/ can not be accessed on host!!!!
      snprintf(workaroundPath, 128, "%s%s", "/Data", dbPath  );

      if(idx < PersCustomLib_LastEntry)
      {
      	int available = 0;
      	if(gPersCustomFuncs[idx].custom_plugin_get_size == NULL )
			{
				if(getCustomLoadingType(idx) == LoadType_OnDemand)
				{
					// plugin not loaded, try to load the requested plugin
					if(load_custom_library(idx, &gPersCustomFuncs[idx]) == 1)
					{
						// check again if the plugin function is now available
						if(gPersCustomFuncs[idx].custom_plugin_get_data != NULL)
						{
							available = 1;
						}
					}
				}
				else if(getCustomLoadingType(idx) == LoadType_PclInit)
				{
					if(gPersCustomFuncs[idx].custom_plugin_get_data != NULL)
					{
						available = 1;
					}
				}
				else
				{
					 DLT_LOG(gPclDLTContext, DLT_LOG_INFO, DLT_STRING("Plugin not available (getData), unknown loading type: "),
					                                       DLT_INT(getCustomLoadingType(idx)));
					 read_size = EPERS_COMMON;
				}
			}
      	else
      	{
      		available = 1;	// already loaded
      	}

      	if(available == 1)
      	{
				char pathKeyString[128] = {0};
				if(info->configKey.customID[0] == '\0')   // if we have not a customID we use the key
				{
					snprintf(pathKeyString, 128, "0x%08X/%s/%s", info->context.ldbid, info->configKey.custom_name, key);
				}
				else
				{
					snprintf(pathKeyString, 128, "0x%08X/%s", info->context.ldbid, info->configKey.customID);
				}
				read_size = gPersCustomFuncs[idx].custom_plugin_get_data(pathKeyString, (char*)buffer, buffer_size);
      	}
      	else
      	{
      		read_size = EPERS_NOPLUGINFUNCT;
      	}
      }
      else
      {
         read_size = EPERS_NOPLUGINFUNCT;
      }

      if (1 > read_size) /* Try to get default values */
      {
         info->configKey.policy = PersistencePolicy_wc;			/* Set the policy */
         info->configKey.type   = PersistenceResourceType_key;  /* Set the type */
         (void)get_db_path_and_key(info, key, NULL, dbPath);
         DLT_LOG(gPclDLTContext, DLT_LOG_INFO, DLT_STRING("Plugin data not available. Try to get default data of key:"),
                                            DLT_STRING(key));
         ret_defaults = pers_get_defaults(dbPath, key, buffer, buffer_size, PersGetDefault_Data);
         if (0 < ret_defaults)
         {
        	 read_size = ret_defaults;
         }
      }
   }
   return read_size;
}



int persistence_set_data(char* dbPath, char* key, PersistenceInfo_s* info, unsigned char* buffer, unsigned int buffer_size)
{
   int write_size = -1;

   if(   PersistenceStorage_local == info->configKey.storage
      || PersistenceStorage_shared == info->configKey.storage )
   {
      int handleDB = -1 ;


      handleDB = database_get(info, dbPath);
      if(handleDB >= 0)
      {
         write_size = persComDbWriteKey(handleDB, key, (char*)buffer, buffer_size) ;
         if(write_size < 0)
         {
            DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("persistence_set_data ==> persComDbWriteKey() failure"));
         }
         else
         {
            if(PersistenceStorage_shared == info->configKey.storage)
            {
               int rval = pers_send_Notification_Signal(key, &info->context, pclNotifyStatus_changed);
               if(rval <= 0)
               {
                  DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("persistence_set_data ==> failed to send notification signal"));
                  write_size = rval;
               }
            }
         }

      }
      else
      {
         DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("persistence_set_data ==> no resource config table"), DLT_STRING(dbPath), DLT_STRING(key));
         write_size = EPERS_NOPRCTABLE;
      }
   }
   else if(PersistenceStorage_custom == info->configKey.storage)   // custom storage implementation via custom library
   {
   	int available = 0;
      int idx = custom_client_name_to_id(dbPath, 1);
      if(idx < PersCustomLib_LastEntry )
      {
      	if(gPersCustomFuncs[idx].custom_plugin_set_data != NULL)
      	{

				if (getCustomLoadingType(idx) == LoadType_OnDemand)
				{
					// plugin not loaded, try to load the requested plugin
					if(load_custom_library(idx, &gPersCustomFuncs[idx]) == 1)
					{
						// check again if the plugin function is now available
						if(gPersCustomFuncs[idx].custom_plugin_set_data != NULL)
						{
							available = 1;
						}
					}
				}
				else if(getCustomLoadingType(idx) == LoadType_PclInit)
				{
					if(gPersCustomFuncs[idx].custom_plugin_set_data != NULL)
					{
						available = 1;
					}
				}
				else
				{
					 DLT_LOG(gPclDLTContext, DLT_LOG_INFO, DLT_STRING("Plugin not available (setData), unknown loading type: "),
					                                       DLT_INT(getCustomLoadingType(idx)));
					 write_size = EPERS_COMMON;
				}
			}
      	else
      	{
      		available = 1;	// already loaded
      	}

      	if(available == 1)
      	{
				char pathKeyString[128] = {0};
				if(info->configKey.customID[0] == '\0')   // if we have not a customID we use the key
				{
					snprintf(pathKeyString, 128, "0x%08X/%s/%s", info->context.ldbid, info->configKey.custom_name, key);
				}
				else
				{
					snprintf(pathKeyString, 128, "0x%08X/%s", info->context.ldbid, info->configKey.customID);
				}
				write_size = gPersCustomFuncs[idx].custom_plugin_set_data(pathKeyString, (char*)buffer, buffer_size);

				if ((0 < write_size) && ((unsigned int)write_size == buffer_size)) /* Check return value and send notification if OK */
				{
					int rval = pers_send_Notification_Signal(key, &info->context, pclNotifyStatus_changed);
					if(rval <= 0)
					{
						DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("persistence_set_data ==> failed to send notification signal"));
						write_size = rval;
					}
				}
      	}
      	else
      	{
      		write_size = EPERS_NOPLUGINFUNCT;
      	}
      }
      else
      {
         write_size = EPERS_NOPLUGINFUNCT;
      }
   }
   return write_size;
}



int persistence_get_data_size(char* dbPath, char* key, PersistenceInfo_s* info)
{
   int read_size = -1;
   int ret_defaults = -1;

   if(   PersistenceStorage_shared == info->configKey.storage
      || PersistenceStorage_local == info->configKey.storage)
   {
      int handleDB = database_get(info, dbPath);
      if(handleDB >= 0)
      {
         read_size = persComDbGetKeySize(handleDB, key);
         if(read_size < 0)
         {
            read_size = pers_get_defaults(dbPath, key, NULL, 0, PersGetDefault_Size);
         }
      }
   }
   else if(PersistenceStorage_custom == info->configKey.storage)   // custom storage implementation via custom library
   {
   	int available = 0;
      int idx = custom_client_name_to_id(dbPath, 1);
      if(idx < PersCustomLib_LastEntry )
      {
      	if(gPersCustomFuncs[idx].custom_plugin_get_size == NULL )
      	{
      		if (getCustomLoadingType(idx) == LoadType_OnDemand)
      		{
					// plugin not loaded, try to load the requested plugin
					if(load_custom_library(idx, &gPersCustomFuncs[idx]) == 1)
					{
						// check again if the plugin function is now available
						if(gPersCustomFuncs[idx].custom_plugin_get_size != NULL)
						{
							available = 1;
						}
					}
      		}
      		else if(getCustomLoadingType(idx) == LoadType_PclInit)
      		{
   				if(gPersCustomFuncs[idx].custom_plugin_get_size != NULL)
					{
   					available = 1;
					}
      		}
				else
				{
					 DLT_LOG(gPclDLTContext, DLT_LOG_INFO, DLT_STRING("Plugin not available (getDataSize), unknown loading type: "),
					                                       DLT_INT(getCustomLoadingType(idx)));
					 read_size = EPERS_COMMON;
				}
      	}
      	else
      	{
      		available = 1;	// already loaded
      	}

      	if(available == 1)
      	{
      		char pathKeyString[128] = {0};
				if(info->configKey.customID[0] == '\0')   // if we have not a customID we use the key
				{
					snprintf(pathKeyString, 128, "0x%08X/%s/%s", info->context.ldbid, info->configKey.custom_name, key);
				}
				else
				{
					snprintf(pathKeyString, 128, "0x%08X/%s", info->context.ldbid, info->configKey.customID);
				}
				read_size = gPersCustomFuncs[idx].custom_plugin_get_size(pathKeyString);
      	}
      	else
      	{
      		read_size = EPERS_NOPLUGINFUNCT;
      	}
      }
      else
      {
         read_size = EPERS_NOPLUGINFUNCT;
      }

      if (1 > read_size)
      {
         info->configKey.policy = PersistencePolicy_wc;			/* Set the policy */
         info->configKey.type   = PersistenceResourceType_key;  /* Set the type */
         (void)get_db_path_and_key(info, key, NULL, dbPath);
         DLT_LOG(gPclDLTContext, DLT_LOG_INFO, DLT_STRING("Plugin data not available. Try to get size of default data for key:"),
                                            DLT_STRING(key));
         ret_defaults = pers_get_defaults(dbPath, key, NULL, 0, PersGetDefault_Size);
         if (0 < ret_defaults)
         {
        	 read_size = ret_defaults;
         }
      }
   }
   return read_size;
}



int persistence_delete_data(char* dbPath, char* key, PersistenceInfo_s* info)
{
   int ret = 0;
   if(PersistenceStorage_custom != info->configKey.storage)
   {
      int handleDB = database_get(info, dbPath);
      if(handleDB >= 0)
      {
         ret = persComDbDeleteKey(handleDB, key) ;
         if(ret < 0)
         {
            DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("persistence_delete_data => persComDbDeleteKey failed: "), DLT_STRING(key));
            if(PERS_COM_ERR_NOT_FOUND == ret)
            {
                ret = EPERS_NOKEY ;
            }
            else
            {
                ret = EPERS_DB_ERROR_INTERNAL ;
            }
         }

         if(PersistenceStorage_shared == info->configKey.storage)
         {
            pers_send_Notification_Signal(key, &info->context, pclNotifyStatus_deleted);
         }
      }
      else
      {
         DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("persistence_delete_data ==> no resource config table"), DLT_STRING(dbPath), DLT_STRING(key));
         ret = EPERS_NOPRCTABLE;
      }
   }
   else   // custom storage implementation via custom library
   {
   	int available = 0;
      int idx = custom_client_name_to_id(dbPath, 1);
      if(idx < PersCustomLib_LastEntry)
      {
      	if(gPersCustomFuncs[idx].custom_plugin_get_size == NULL )
			{
				if (getCustomLoadingType(idx) == LoadType_OnDemand)
				{
					// plugin not loaded, try to load the requested plugin
					if(load_custom_library(idx, &gPersCustomFuncs[idx]) == 1)
					{
						// check again if the plugin function is now available
						if(gPersCustomFuncs[idx].custom_plugin_delete_data != NULL)
						{
							available = 1;
						}
					}
				}
				else if(getCustomLoadingType(idx) == LoadType_PclInit)
				{
					if(gPersCustomFuncs[idx].custom_plugin_delete_data != NULL)
					{
						available = 1;
					}
				}
				else
				{
					 DLT_LOG(gPclDLTContext, DLT_LOG_INFO, DLT_STRING("Plugin not available (deleteData), unknown loading type: "),
					                                       DLT_INT(getCustomLoadingType(idx)));
					 ret = EPERS_COMMON;
				}
			}
      	else
      	{
      		available = 1;	// already loaded
      	}

      	if(available == 1)
      	{
				char pathKeyString[128] = {0};
				if(info->configKey.customID[0] == '\0')   // if we have not a customID we use the key
				{
					snprintf(pathKeyString, 128, "0x%08X/%s/%s", info->context.ldbid, info->configKey.custom_name, key);
				}
				else
				{
					snprintf(pathKeyString, 128, "0x%08X/%s", info->context.ldbid, info->configKey.customID);
				}
				ret = gPersCustomFuncs[idx].custom_plugin_delete_data(pathKeyString);

				if(0 <= ret) /* Check return value and send notification if OK */
				{
					pers_send_Notification_Signal(key, &info->context, pclNotifyStatus_deleted);
				}
      	}
      	else
      	{
      		ret = EPERS_NOPLUGINFUNCT;
      	}
      }
      else
      {
         ret = EPERS_NOPLUGINFUNCT;
      }
   }
   return ret;
}


int persistence_notify_on_change(const char* key, unsigned int ldbid, unsigned int user_no, unsigned int seat_no,
                                 pclChangeNotifyCallback_t callback, PersNotifyRegPolicy_e regPolicy)
{
   int rval = 0;

   if(regPolicy < Notify_lastEntry)
   {
   	MainLoopData_u data;

   	data.message.cmd = (uint32_t)CMD_REG_NOTIFY_SIGNAL;
   	data.message.params[0] = ldbid;
   	data.message.params[1] = user_no;
   	data.message.params[2] = seat_no;
   	data.message.params[3] = regPolicy;

   	snprintf(data.message.string, DbKeyMaxLen, "%s", key);

      if(regPolicy == Notify_register)
      {
         // assign callback
         gChangeNotifyCallback = callback;
      }
      else if(regPolicy == Notify_unregister)
      {
         // remove callback
         gChangeNotifyCallback = NULL;
      }

      if(-1 == deliverToMainloop(&data))
      {
         DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("persistence_notify_on_change => failed to write to pipe"), DLT_INT(errno));
         rval = -1;
      }
   }
   else
   {
      rval = -1;
   }

   return rval;
}






int pers_send_Notification_Signal(const char* key, PersistenceDbContext_s* context, pclNotifyStatus_e reason)
{
   int rval = 1;
   if(reason < pclNotifyStatus_lastEntry)
   {
   	MainLoopData_u data;

   	data.message.cmd = (uint32_t)CMD_SEND_NOTIFY_SIGNAL;
   	data.message.params[0] = context->ldbid;
   	data.message.params[1] = context->user_no;
   	data.message.params[2] = context->seat_no;
   	data.message.params[3] = reason;

   	snprintf(data.message.string, DbKeyMaxLen, "%s", key);

      if(-1 == deliverToMainloop(&data) )
      {
         DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("pers_send_Notification_Signal => failed to write to pipe"), DLT_INT(errno));
         rval = EPERS_NOTIFY_SIG;
      }
   }
   else
   {
      rval = EPERS_NOTIFY_SIG;
   }

   return rval;
}


void pers_rct_close_all()
{
   int i = 0;

   // close all open persistence resource configuration tables
   for(i=0; i< PrctDbTableSize; i++)
   {
   	if(get_resource_cfg_table_by_idx(i) != -1)
   	{
			if(persComRctClose(get_resource_cfg_table_by_idx(i)) != 0)
			{
				DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("process_prepare_shutdown => failed to close db => index:"), DLT_INT(i));
			}

			invalidate_resource_cfg_table(i);
   	}
   }
}

