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
#include "persistence_client_library_tree_helper.h"
#include "crc32.h"

#include <persComErrors.h>

#include <errno.h>




/// btree array
static int gHandlesDB[DbTableSize][PersistenceDB_LastEntry];
static int gHandlesDBCreated[DbTableSize][PersistenceDB_LastEntry] = { {0} };

/// tree to store notification information
static jsw_rbtree_t *gNotificationTree = NULL;


void deleteNotifyTree(void)
{
   if(gNotificationTree != NULL)
   {
      jsw_rbdelete(gNotificationTree);
      gNotificationTree = NULL;
   }
}


static int database_get(PersistenceInfo_s* info, const char* dbPath, int dbType)
{
   int arrayIdx = 0, handleDB = -1;

   // create array index: index is a combination of resource configuration table type and group
   arrayIdx = info->configKey.storage + info->context.ldbid ;

   if(arrayIdx < DbTableSize)
   {
      int openFlags = 0x01;   // by default create file if not existing

      if(gHandlesDBCreated[arrayIdx][dbType] == 0)
      {
         char path[PERS_ORG_MAX_LENGTH_PATH_FILENAME] = {0};

         if(PersistencePolicy_wt == dbType)				/// write through database
         {
            /// 0x02 ==> open database in write through mode,keep bit 1 set in order to create db if not existing
            openFlags |= 0x02;
            snprintf(path, PERS_ORG_MAX_LENGTH_PATH_FILENAME, "%s%s", dbPath, plugin_gLocalWt);
         }
         else if(PersistencePolicy_wc == dbType)		// cached database
         {
            snprintf(path, PERS_ORG_MAX_LENGTH_PATH_FILENAME, "%s%s", dbPath, plugin_gLocalCached);
         }
         else if(PersistenceDB_confdefault == dbType)	// configurable default database
			{
			  snprintf(path, PERS_ORG_MAX_LENGTH_PATH_FILENAME, "%s%s", dbPath, plugin_gLocalConfigurableDefault);
			}
			else if(PersistenceDB_default == dbType)		// default database
			{
			  snprintf(path, PERS_ORG_MAX_LENGTH_PATH_FILENAME, "%s%s", dbPath, plugin_gLocalFactoryDefault);
			}
         else
         {
            handleDB = -2;
         }

         if (handleDB == -1)
         {
            handleDB = plugin_persComDbOpen(path, openFlags);
            if(handleDB >= 0)
            {
               gHandlesDB[arrayIdx][dbType] = handleDB ;
               gHandlesDBCreated[arrayIdx][dbType] = 1;
            }
            else
            {
               DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("dbGet - persComDbOpen() failed"));
            }
         }
         else
         {
            DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("dbGet - wrong policy! Cannot extend dbPath wit db."));
         }
      }
      else
      {
         handleDB = gHandlesDB[arrayIdx][dbType];
      }
   }
   else
   {
      DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("dbGet - inval storage type"), DLT_STRING(dbPath));
   }
   return handleDB;
}


int pers_get_defaults(char* dbPath, char* key, PersistenceInfo_s* info, unsigned char* buffer, unsigned int buffer_size, PersGetDefault_e job)
{
   PersDefaultType_e i = PersDefaultType_Configurable;
   int handleDefaultDB = -1, read_size = EPERS_NOKEY;
   char dltMessage[PERS_ORG_MAX_LENGTH_PATH_FILENAME] = {0};

   for(i=(int)PersistenceDB_confdefault; i<(int)PersistenceDB_LastEntry; i++)
   {
   	handleDefaultDB = database_get(info, dbPath, i);
      if(handleDefaultDB >= 0)
      {
         if (PersGetDefault_Data == job)
         {
         	read_size = plugin_persComDbReadKey(handleDefaultDB, key, (char*)buffer, buffer_size);
         }
         else if (PersGetDefault_Size == job)
         {
            read_size = plugin_persComDbGetKeySize(handleDefaultDB, key);
         }
         else
         {
            DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("getDefaults - unknown job"));
            break;
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
               snprintf(dltMessage, PERS_ORG_MAX_LENGTH_PATH_FILENAME, "%s%s", dbPath, plugin_gLocalConfigurableDefault);
            }
            if (PersDefaultType_Factory == i)
            {
                snprintf(dltMessage, PERS_ORG_MAX_LENGTH_PATH_FILENAME, "%s%s", dbPath, plugin_gLocalFactoryDefault);
            }
            DLT_LOG(gPclDLTContext, DLT_LOG_DEBUG, DLT_STRING("getDefaults - default data will be used for Key"), DLT_STRING(key),
                                                  DLT_STRING("from"), DLT_STRING(dltMessage));
            break;
         }
      }
   }

   if (read_size < 0)
   {
       DLT_LOG(gPclDLTContext, DLT_LOG_DEBUG, DLT_STRING("getDefaults - default data not available for Key"), DLT_STRING(key),
                                             DLT_STRING("Path:"), DLT_STRING(dbPath));
   }

   return read_size;
}



void database_close_all()
{
   int i = 0, j = 0;

   for(i=0; i<DbTableSize; i++)
   {
   	for(j=0; j < PersistenceDB_LastEntry; j++)
   	{
			if(gHandlesDBCreated[i][j] == 1)
			{
				int iErrorCode = plugin_persComDbClose(gHandlesDB[i][j]);
				if (iErrorCode < 0)
				{
					DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("dbCloseAll - Err close db"));
				}
				else
				{
					 gHandlesDBCreated[i][j] = 0;
				}
			}
   	}
   }
}



int persistence_get_data(char* dbPath, char* key, const char* resourceID, PersistenceInfo_s* info, unsigned char* buffer, unsigned int buffer_size)
{
   int read_size = -1, ret_defaults = -1;

   if(   PersistenceStorage_shared == info->configKey.storage
      || PersistenceStorage_local == info->configKey.storage)
   {
      int handleDB = database_get(info, dbPath, info->configKey.policy);
      if(handleDB >= 0)
      {
         read_size = plugin_persComDbReadKey(handleDB, key, (char*)buffer, buffer_size);
         if(read_size < 0)
         {
            read_size = pers_get_defaults(dbPath, (char*)resourceID, info, buffer, buffer_size, PersGetDefault_Data); /* 0 ==> Get data */
         }
      }
   }
   else if(PersistenceStorage_custom == info->configKey.storage)   // custom storage implementation via custom library
   {
      int idx =  custom_client_name_to_id(dbPath, 1);
      char workaroundPath[128];  								         // workaround, because /sys/ can not be accessed on host!!!!
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
					 DLT_LOG(gPclDLTContext, DLT_LOG_DEBUG, DLT_STRING("getData - Plugin not avail, unknown loading type: "),
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

      if (1 > read_size) // Try to get default values
      {
         info->configKey.policy = PersistencePolicy_wc;			 // Set the policy
         info->configKey.type   = PersistenceResourceType_key;  // Set the type

         (void)get_db_path_and_key(info, key, NULL, dbPath);

         DLT_LOG(gPclDLTContext, DLT_LOG_DEBUG, DLT_STRING("getData - Plugin data not available - get default data of key:"), DLT_STRING(key));
         ret_defaults = pers_get_defaults(dbPath, (char*)resourceID, info, buffer, buffer_size, PersGetDefault_Data);
         if (0 < ret_defaults)
         {
        	   read_size = ret_defaults;
         }
      }
   }
   return read_size;
}



int persistence_set_data(char* dbPath, char* key, const char* resource_id, PersistenceInfo_s* info, unsigned char* buffer, unsigned int buffer_size)
{
   int write_size = -1;

   if(   PersistenceStorage_local == info->configKey.storage
      || PersistenceStorage_shared == info->configKey.storage )
   {
      int handleDB = -1 ;
      int dbType = info->configKey.policy;      // assign default policy
      const char* dbInput = key;                // assign default key

      if(info->context.user_no ==  (int)PCL_USER_DEFAULTDATA)
      {
         dbType = PersistenceDB_confdefault;    // change policy when writing configurable default data
         dbInput = resource_id;                 // change database key when writing configurable default data
      }

      handleDB = database_get(info, dbPath, dbType);

      if(handleDB >= 0)
      {
         write_size = plugin_persComDbWriteKey(handleDB, dbInput, (char*)buffer, buffer_size) ;
         if(write_size < 0)
         {
            DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("setData - persComDbWriteKey() failure"));
         }
         else
         {
            if(PersistenceStorage_shared == info->configKey.storage)
            {
               int rval = pers_send_Notification_Signal(resource_id, &info->context, pclNotifyStatus_changed);
               if(rval <= 0)
               {
                  DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("setData - Err to send noty sig"));
                  write_size = rval;
               }
            }
         }
      }
      else
      {
         DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("setData - no RCT"), DLT_STRING(dbPath), DLT_STRING(key));
         write_size = EPERS_NOPRCTABLE;
      }
   }
   else if(PersistenceStorage_custom == info->configKey.storage)   // custom storage implementation via custom library
   {
   	int available = 0, idx = custom_client_name_to_id(dbPath, 1);

      if(idx < PersCustomLib_LastEntry )
      {
      	if(gPersCustomFuncs[idx].custom_plugin_set_data == NULL)
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
					 DLT_LOG(gPclDLTContext, DLT_LOG_DEBUG, DLT_STRING("setData - Plugin not avail, unknown loading type: "),
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
					int rval = pers_send_Notification_Signal(resource_id, &info->context, pclNotifyStatus_changed);
					if(rval <= 0)
					{
						DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("setData - Err send noty sig"));
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



int persistence_get_data_size(char* dbPath, char* key, const char* resourceID, PersistenceInfo_s* info)
{
   int read_size = -1, ret_defaults = -1;

   if(   PersistenceStorage_shared == info->configKey.storage
      || PersistenceStorage_local == info->configKey.storage)
   {
      int handleDB = database_get(info, dbPath, info->configKey.policy);
      if(handleDB >= 0)
      {

         read_size = plugin_persComDbGetKeySize(handleDB, key);
         if(read_size < 0)
         {
            read_size = pers_get_defaults( dbPath, (char*)resourceID, info, NULL, 0, PersGetDefault_Size);
         }
      }
   }
   else if(PersistenceStorage_custom == info->configKey.storage)   // custom storage implementation via custom library
   {
   	int available = 0, idx = custom_client_name_to_id(dbPath, 1);

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
					 DLT_LOG(gPclDLTContext, DLT_LOG_DEBUG, DLT_STRING("getDataSize - Plugin not avail, unknown loading type: "),
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
         DLT_LOG(gPclDLTContext, DLT_LOG_DEBUG, DLT_STRING("getDataSize - Plugin data not avail,  get size of default data for key:"),
                                            DLT_STRING(key));
         ret_defaults = pers_get_defaults(dbPath, (char*)resourceID, info, NULL, 0, PersGetDefault_Size);
         if (0 < ret_defaults)
         {
        	 read_size = ret_defaults;
         }
      }
   }
   return read_size;
}



int persistence_delete_data(char* dbPath, char* key, const char* resource_id, PersistenceInfo_s* info)
{
   int ret = 0;
   if(PersistenceStorage_custom != info->configKey.storage)
   {
      int handleDB = database_get(info, dbPath, info->configKey.policy);
      if(handleDB >= 0)
      {
         ret = plugin_persComDbDeleteKey(handleDB, key) ;
         if(ret < 0)
         {
            DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("deleteData - failed: "), DLT_STRING(key));
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
            pers_send_Notification_Signal(resource_id, &info->context, pclNotifyStatus_deleted);
         }
      }
      else
      {
         DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("deleteData - no RCT"), DLT_STRING(dbPath), DLT_STRING(key));
         ret = EPERS_NOPRCTABLE;
      }
   }
   else   // custom storage implementation via custom library
   {
   	int available = 0, idx = custom_client_name_to_id(dbPath, 1);

      if(idx < PersCustomLib_LastEntry)
      {
      	if(gPersCustomFuncs[idx].custom_plugin_delete_data == NULL )
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
					 DLT_LOG(gPclDLTContext, DLT_LOG_DEBUG, DLT_STRING("deleteData - Plugin not available, unknown loading type: "),
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
					pers_send_Notification_Signal(resource_id, &info->context, pclNotifyStatus_deleted);
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



int persistence_notify_on_change(const char* resource_id, const char* dbKey, unsigned int ldbid, unsigned int user_no, unsigned int seat_no,
                                 pclChangeNotifyCallback_t callback, PersNotifyRegPolicy_e regPolicy)
{
   int rval = 0;

   if(regPolicy < Notify_lastEntry)
   {
   	MainLoopData_u data;
      key_value_s* foundItem = NULL;
      key_value_s* searchItem = NULL;
      unsigned int hashKey = pclCrc32(0, (unsigned char*)dbKey, strlen(dbKey));

   	data.message.cmd = (uint32_t)CMD_REG_NOTIFY_SIGNAL;
   	data.message.params[0] = ldbid;
   	data.message.params[1] = user_no;
   	data.message.params[2] = seat_no;
   	data.message.params[3] = regPolicy;

   	snprintf(data.message.string, PERS_DB_MAX_LENGTH_KEY_NAME, "%s", resource_id);

      // check if the tree has already been created
   	if(gNotificationTree == NULL)
   	{
   	   gNotificationTree = jsw_rbnew(key_val_cmp, key_val_dup, key_val_rel);
   	}

   	// search if item is already stored in the tree
   	searchItem = malloc(sizeof(key_value_s));
      searchItem->key = hashKey;
      foundItem = (key_value_s*)jsw_rbfind(gNotificationTree, searchItem);

      if(regPolicy == Notify_register)
      {
         if(foundItem == NULL)   // item not found add it, else already added so nothing to do
         {
            key_value_s* item = malloc(sizeof(key_value_s));    // assign key and value to the rbtree item
            if(item != NULL)
            {
               item->key = hashKey;
               // we don't need the path name here, we just need to know that this key is available in the tree
               item->value = "";
               (void)jsw_rbinsert(gNotificationTree, item);
               free(item);

               gChangeNotifyCallback = callback;      // assign callback
            }
            else
            {
               rval = -1;
            }
         }
         free(foundItem);
      }
      else if(regPolicy == Notify_unregister)
      {
         if(foundItem != NULL)   // item already in the tree remove it, if not found nothing to do
         {
            // remove from tree
            jsw_rberase(gNotificationTree, foundItem);

            if(jsw_rbsize(gNotificationTree) == 0)  // if no other notification is stored in the tree, remove callback
            {
               gChangeNotifyCallback = NULL;          // remove callback
            }
         }
         else
         {
            free(foundItem);
         }
      }

      if(-1 == deliverToMainloop(&data))
      {
         DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("notifyOnChange - Err to write to pipe"), DLT_INT(errno));
         rval = -1;
      }

      free(searchItem);
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

   	snprintf(data.message.string, PERS_DB_MAX_LENGTH_KEY_NAME, "%s", key);

      if(-1 == deliverToMainloop(&data) )
      {
         DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("sendNotifySig - Err write to pipe"), DLT_INT(errno));
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

   for(i=0; i< PrctDbTableSize; i++)      // close all open persistence resource configuration tables
   {
   	if(get_resource_cfg_table_by_idx(i) != -1)
   	{
			if(plugin_persComRctClose(get_resource_cfg_table_by_idx(i)) != 0)
			{
				DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("prepShtdwn - Err close db => index:"), DLT_INT(i));
			}
			invalidate_resource_cfg_table(i);
   	}
   }
}

