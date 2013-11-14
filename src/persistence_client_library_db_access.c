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
 * @file           persistence_client_library_data_access.c
 * @ingroup        Persistence client library
 * @author         Ingo Huerner
 * @brief          Implementation of persistence database access
 *                 Library provides an API to access persistent data
 * @see            
 */

#include "../include_protected/persistence_client_library_db_access.h"
#include "../include_protected/persistence_client_library_rc_table.h"
#include "persistence_client_library_custom_loader.h"
#include "persistence_client_library_itzam_errors.h"

#include "persistence_client_library_dbus_service.h"

#include <dbus/dbus.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>


/// definition of a key-value pair stored in the database
typedef struct _KeyValuePair_s
{
    char m_key[DbKeySize];    /// the key
    char m_data[DbValueSize];    /// the data
    unsigned int m_data_size;   /// the size of the data
}
KeyValuePair_s;


// definition of a cursor entry
typedef struct _CursorEntry_s
{
   itzam_btree_cursor m_cursor;
   itzam_btree        m_btree;
   int                m_empty;
}
CursorEntry_s;

// cursor array handle
CursorEntry_s gCursorArray[MaxPersHandle];

/// handle index
static int gHandleIdx = 1;

/// free handle array
int gFreeCursorHandleArray[MaxPersHandle];
// free head index
int gFreeCursorHandleIdxHead = 0;

// mutex to controll access to the cursor array
pthread_mutex_t gMtx = PTHREAD_MUTEX_INITIALIZER;


/// btree array
static itzam_btree gBtree[DbTableSize][PersistencePolicy_LastEntry];
static int gBtreeCreated[DbTableSize][PersistencePolicy_LastEntry] = { {0} };


// function prototype
int pers_send_Notification_Signal(const char* key, PersistenceDbContext_s* context, unsigned int reason);
int pers_get_default_data(char* dbPath, char* key, char* buffer, unsigned int buffer_size);




int pers_db_open_default(itzam_btree* btree, const char* dbPath, int configDefault)
{
   itzam_state  state = ITZAM_FAILED;
   char path[DbPathMaxLen] = {0};

   if(1 == configDefault)
   {
      snprintf(path, DbPathMaxLen, "%s%s", dbPath, gConfigDefault);
   }
   else if(0 == configDefault)
   {
      snprintf(path, DbPathMaxLen, "%s%s", dbPath, gDefault);
   }
   else
   {
      return -1;  // invalid
   }

   state = itzam_btree_open(btree, path, itzam_comparator_string, error_handler, 0/*recover*/, 0/*read_only*/);
   if (state != ITZAM_OKAY)
   {
      DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("pers_db_open_default ==> itzam_btree_open => Itzam problem"), DLT_STRING(STATE_MESSAGES[state]));
   }

   return 1;
}


itzam_btree* pers_db_open(PersistenceInfo_s* info, const char* dbPath)
{
   int arrayIdx = 0;
   itzam_btree* btree = NULL;

   // create array index: index is a combination of resource config table type and group
   arrayIdx = info->configKey.storage + info->context.ldbid;

   //if(arrayIdx <= DbTableSize)
   if(arrayIdx < DbTableSize)
   {
      if(gBtreeCreated[arrayIdx][info->configKey.policy] == 0)
      {
         itzam_state  state = ITZAM_FAILED;
         char path[DbPathMaxLen] = {0};

         if(PersistencePolicy_wt == info->configKey.policy)
         {
            snprintf(path, DbPathMaxLen, "%s%s", dbPath, gWt);
         }
         else if(PersistencePolicy_wc == info->configKey.policy)
         {
            snprintf(path, DbPathMaxLen, "%s%s", dbPath, gCached);
         }
         else
         {
            return btree;
         }

         state = itzam_btree_open(&gBtree[arrayIdx][info->configKey.policy], path,
                                  itzam_comparator_string, error_handler, 0/*recover*/, 0/*read_only*/);
         if (state != ITZAM_OKAY)
         {
            DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("pers_db_open ==> itzam_btree_open => Itzam problem"), DLT_STRING(STATE_MESSAGES[state]));
         }
         gBtreeCreated[arrayIdx][info->configKey.policy] = 1;
      }
      // assign database
      btree = &gBtree[arrayIdx][info->configKey.policy];
   }
   else
   {
      DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("pers_db_open ==> invalid storage type"), DLT_STRING(dbPath));
   }
   return btree;
}



void pers_db_close(PersistenceInfo_s* info)
{
   int arrayIdx = info->configKey.storage + info->context.ldbid;

   if(info->configKey.storage <= PersistenceStorage_shared )
   {
      itzam_state  state = ITZAM_FAILED;
      state = itzam_btree_close(&gBtree[arrayIdx][info->configKey.policy]);
      if (state != ITZAM_OKAY)
      {
         DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("pers_db_close ==> itzam_btree_close => Itzam problem"), DLT_STRING(STATE_MESSAGES[state]));
      }
      gBtreeCreated[arrayIdx][info->configKey.policy] = 0;
   }
   else
   {
      DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("pers_db_close ==> invalid storage type"), DLT_INT(info->context.ldbid ));
   }
}



void pers_db_close_all()
{
   int i = 0;

   for(i=0; i<DbTableSize; i++)
   {
      // close write cached database
      if(gBtreeCreated[i][PersistencePolicy_wc] == 1)
      {
         itzam_state  state = ITZAM_FAILED;
         state = itzam_btree_close(&gBtree[i][PersistencePolicy_wc]);
         if (state != ITZAM_OKAY)
         {
            DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("pers_db_close_all ==> itzam_btree_close => Itzam problem:"), DLT_STRING(STATE_MESSAGES[state]) );
         }
         gBtreeCreated[i][PersistencePolicy_wc] = 0;
      }

      // close write through database
      if(gBtreeCreated[i][PersistencePolicy_wt] == 1)
      {
         itzam_state  state = ITZAM_FAILED;
         state = itzam_btree_close(&gBtree[i][PersistencePolicy_wt]);
         if (state != ITZAM_OKAY)
         {
            DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("pers_db_close_all ==>itzam_btree_close => Itzam problem:"), DLT_STRING(STATE_MESSAGES[state]));
         }
         gBtreeCreated[i][PersistencePolicy_wt] = 0;
      }
   }
}


int pers_db_read_key(char* dbPath, char* key, PersistenceInfo_s* info, unsigned char* buffer, unsigned int buffer_size)
{
   int read_size = -1;

   if(   PersistenceStorage_shared == info->configKey.storage
      || PersistenceStorage_local == info->configKey.storage)
   {
      itzam_btree* btree = NULL;
      int keyFound = 0;
      itzam_state  state = ITZAM_FAILED;

      btree = pers_db_open(info, dbPath);
      if(btree != NULL)
      {
         KeyValuePair_s search;

         if(itzam_true == itzam_btree_find(btree, key, &search))
         {
            read_size = search.m_data_size;
            if(read_size > buffer_size)
            {
               read_size = buffer_size;   // truncate data size to buffer size
            }
            memcpy(buffer, search.m_data, read_size);
            keyFound = 1;
         }
      }
      if(keyFound == 0) // check for default values.
      {
         read_size = pers_get_default_data(dbPath, key, buffer, buffer_size);
      }
   }
   else if(PersistenceStorage_custom == info->configKey.storage)   // custom storage implementation via custom library
   {
      int idx =  custom_client_name_to_id(dbPath, 1);
      char workaroundPath[128];  // workaround, because /sys/ can not be accessed on host!!!!
      snprintf(workaroundPath, 128, "%s%s", "/Data", dbPath  );

      if( (idx < PersCustomLib_LastEntry) && (gPersCustomFuncs[idx].custom_plugin_get_data != NULL) )
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

         if(read_size < 0) // check if for custom storage default values are available
         {
            read_size = pers_get_default_data(dbPath, key, buffer, buffer_size);
         }
      }
      else
      {
         read_size = EPERS_NOPLUGINFUNCT;
      }
   }
   return read_size;
}



int pers_get_default_data(char* dbPath, char* key, char* buffer, unsigned int buffer_size)
{
   int keyFound = 0;
   int read_size = 0;
   KeyValuePair_s search;

   itzam_state  state = ITZAM_FAILED;
   itzam_btree btreeConfDefault;
   itzam_btree btreeDefault;

   // 1. check if _configurable_ default data is available
   // --------------------------------
   if(pers_db_open_default(&btreeConfDefault, dbPath, 1) != -1)
   {
      if(itzam_true == itzam_btree_find(&btreeConfDefault, key, &search)) // read db
      {
         read_size = search.m_data_size;
         if(read_size > buffer_size)
         {
            read_size = buffer_size;   // truncate data size to buffer size
         }
         memcpy(buffer, search.m_data, read_size);

         keyFound = 1;
      }
      else
      {
         DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("pers_db_read_key ==> 2. resource not found in default config => search in default db"), DLT_STRING(key));
      }

      state = itzam_btree_close(&btreeConfDefault);
      if (state != ITZAM_OKAY)
      {
         DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("pers_db_read_key ==> default: itzam_btree_close => Itzam problem"), DLT_STRING(STATE_MESSAGES[state]));
      }
   }

   // 2. check if default data is available
   // --------------------------------
   if(keyFound == 0)
   {
      if(pers_db_open_default(&btreeDefault, dbPath, 0) != -1)
      {
         if(itzam_true == itzam_btree_find(&btreeDefault, key, &search)) // read db
         {
            read_size = search.m_data_size;
            if(read_size > buffer_size)
            {
               read_size = buffer_size;   // truncate data size to buffer size
            }
            memcpy(buffer, search.m_data, read_size);
         }
         else
         {
            DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("pers_db_read_key ==> 3. reasoure not found in both default db's"), DLT_STRING(key) );
            read_size = EPERS_NOKEY;   // the key is not available neither in regular db nor in the default db's
         }

         state = itzam_btree_close(&btreeDefault);
         if (state != ITZAM_OKAY)
         {
            DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("pers_db_read_key ==> default: itzam_btree_close => Itzam problem"), DLT_STRING(STATE_MESSAGES[state]));
         }
      }
      else
      {
         DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("pers_db_read_key ==>no resource config table"), DLT_STRING(dbPath), DLT_STRING(key) );
         read_size = EPERS_NOPRCTABLE;
      }
   }
   return read_size;
}



int pers_db_write_key(char* dbPath, char* key, PersistenceInfo_s* info, unsigned char* buffer, unsigned int buffer_size)
{
   int write_size = -1;

   if(   PersistenceStorage_local == info->configKey.storage
      || PersistenceStorage_shared == info->configKey.storage )
   {
      write_size = buffer_size;
      itzam_btree* btree = NULL;
      itzam_state  state = ITZAM_FAILED;
      KeyValuePair_s insert;

      btree = pers_db_open(info, dbPath);
      if(btree != NULL)
      {
         int keySize = 0;
         keySize = (int)strlen((const char*)key);
         if(keySize < DbKeySize)
         {
            int dataSize = 0;
            dataSize = (int)strlen( (const char*)buffer);
            if(dataSize < DbValueSize)
            {
               // -----------------------------------------------------------------------------
               // transaction start
               itzam_btree_transaction_start(btree);

               // key
               memset(insert.m_key, 0, DbKeySize);
               memcpy(insert.m_key, key, keySize);
               if(itzam_true == itzam_btree_find(btree, key, &insert))
               {
                  // key already available, so delete "old" key
                  state = itzam_btree_remove(btree, (const void *)&insert);
               }

               // data
               memset(insert.m_data, 0, DbValueSize);
               memcpy(insert.m_data, buffer, dataSize);

               // data size
               insert.m_data_size = buffer_size;

               state = itzam_btree_insert(btree,(const void *)&insert);
               if (state != ITZAM_OKAY)
               {
                  DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("pers_db_write_key ==> itzam_btree_insert => Itzam problem"), DLT_STRING(STATE_MESSAGES[state]) );
                  write_size = EPERS_DB_ERROR_INTERNAL;
               }

               itzam_btree_transaction_commit(btree);
               // transaction end
               // -----------------------------------------------------------------------------

               if(PersistenceStorage_shared == info->configKey.storage)
               {
                  write_size = pers_send_Notification_Signal(key, &info->context, pclNotifyStatus_changed);
               }
            }
            else
            {
               DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("pers_db_write_key ==> data to long » size:"), DLT_INT(dataSize), DLT_INT(DbValueSize) );
               write_size = EPERS_DB_VALUE_SIZE;
            }
         }
         else
         {
            DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("pers_db_write_key ==> key to long » size"), DLT_INT(keySize), DLT_INT(DbKeySize) );
            write_size = EPERS_DB_KEY_SIZE;
         }
      }
      else
      {
         DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("pers_db_write_key ==> no resource config table"), DLT_STRING(dbPath), DLT_STRING(key));
         write_size = EPERS_NOPRCTABLE;
      }
   }
   else if(PersistenceStorage_custom == info->configKey.storage)   // custom storage implementation via custom library
   {
      int idx = custom_client_name_to_id(dbPath, 1);
      if((idx < PersCustomLib_LastEntry) && (gPersCustomFuncs[idx].custom_plugin_set_data != NULL) )
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

         if(write_size >= 0)  // success ==> send deleted notification
         {
            write_size = pers_send_Notification_Signal(key, &info->context, pclNotifyStatus_changed);
         }
      }
      else
      {
         write_size = EPERS_NOPLUGINFUNCT;
      }
   }
   return write_size;
}



int pers_db_get_key_size(char* dbPath, char* key, PersistenceInfo_s* info)
{
   int read_size = -1;

   if(   PersistenceStorage_shared == info->configKey.storage
      || PersistenceStorage_local == info->configKey.storage)
   {
      int keySize = 0;
      itzam_btree*  btree = NULL;
      KeyValuePair_s search;

      btree = pers_db_open(info, dbPath);
      if(btree != NULL)
      {
         keySize = (int)strlen((const char*)key);
         if(keySize < DbKeySize)
         {
            memset(search.m_key,0, DbKeySize);
            memcpy(search.m_key, key, keySize);
            if(itzam_true == itzam_btree_find(btree, key, &search))
            {
               read_size = search.m_data_size;
            }
            else
            {
               read_size = EPERS_NOKEY;
            }
         }
         else
         {
            DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("pers_db_get_key_size ==> key to long"), DLT_INT(keySize), DLT_INT(DbKeySize));
            read_size = EPERS_DB_KEY_SIZE;
         }
      }
      else
      {
         DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("pers_db_get_key_size ==> no config table"), DLT_STRING(dbPath), DLT_STRING(key));
         read_size = EPERS_NOPRCTABLE;
      }
   }
   else if(PersistenceStorage_custom == info->configKey.storage)   // custom storage implementation via custom library
   {
      int idx = custom_client_name_to_id(dbPath, 1);
      if((idx < PersCustomLib_LastEntry) && (gPersCustomFuncs[idx].custom_plugin_get_size != NULL) )
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
   return read_size;
}



int pers_db_delete_key(char* dbPath, char* key, PersistenceInfo_s* info)
{
   int ret = 0;
   if(PersistenceStorage_custom != info->configKey.storage)
   {
      itzam_btree*  btree = NULL;
      KeyValuePair_s delete;

      btree = pers_db_open(info, dbPath);
      if(btree != NULL)
      {
         int keySize = 0;
         keySize = (int)strlen((const char*)key);
         if(keySize < DbKeySize)
         {
            // -----------------------------------------------------------------------------
            // transaction start
            itzam_btree_transaction_start(btree);

            itzam_state  state;

            memset(delete.m_key,0, DbKeySize);
            memcpy(delete.m_key, key, keySize);
            state = itzam_btree_remove(btree, (const void *)&delete);
            if (state != ITZAM_OKAY)
            {
               DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("pers_db_delete_key ==> itzam_btree_remove => Itzam problem"), DLT_STRING(STATE_MESSAGES[state]));
               ret = EPERS_DB_ERROR_INTERNAL;
            }
            itzam_btree_transaction_commit(btree);
            // transaction end
            // -----------------------------------------------------------------------------

            if(PersistenceStorage_shared == info->configKey.storage)
            {
               ret = pers_send_Notification_Signal(key, &info->context, pclNotifyStatus_deleted);
            }
         }
         else
         {
            DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("pers_db_delete_key ==> key to long"), DLT_INT(keySize), DLT_INT(DbKeySize));
            ret = EPERS_DB_KEY_SIZE;
         }
      }
      else
      {
         DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("pers_db_delete_key ==> no resource config table"), DLT_STRING(dbPath), DLT_STRING(key));
         ret = EPERS_NOPRCTABLE;
      }
   }
   else   // custom storage implementation via custom library
   {
      int idx = custom_client_name_to_id(dbPath, 1);
      if((idx < PersCustomLib_LastEntry) && (gPersCustomFuncs[idx].custom_plugin_delete_data != NULL) )
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
         if(ret >= 0)   // success ==> send deleted notification
         {
            ret = pers_send_Notification_Signal(key, &info->context, pclNotifyStatus_deleted);
         }
      }
      else
      {
         ret = EPERS_NOPLUGINFUNCT;
      }
   }
   return ret;
}


int persistence_notify_on_change(char* key, unsigned int ldbid, unsigned int user_no, unsigned int seat_no,
                                 pclChangeNotifyCallback_t callback, PersNotifyRegPolicy_e regPolicy)
{
   int rval = 0;

   if(regPolicy < Notify_lastEntry)
   {
      snprintf(gRegNotifykey, DbKeyMaxLen, "%s", key);
      gRegNotifyLdbid  = ldbid;     // to do: pass correct ==> JUST TESTING!!!!
      gRegNotifyUserNo = user_no;
      gRegNotifySeatNo = seat_no;
      gRegNotifyPolicy = regPolicy;

      if(regPolicy == Notify_lastEntry)
      {
         // assign callback
         gChangeNotifyCallback = callback;
      }
      else if(regPolicy == Notify_unregister)
      {
         // remove callback
         gChangeNotifyCallback = NULL;
      }

      if(-1 == deliverToMainloop(CMD_REG_NOTIFY_SIGNAL, 0, 0))
      {
         DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("persistence_notify_on_change => failed to write to pipe"), DLT_INT(errno));
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
      snprintf(gSendNotifykey,  DbKeyMaxLen,       "%s", key);

      gSendNotifyLdbid  = context->ldbid;     // to do: pass correct ==> JUST TESTING!!!!
      gSendNotifyUserNo = context->user_no;
      gSendNotifySeatNo = context->seat_no;
      gSendNotifyReason = reason;

      if(-1 == deliverToMainloop(CMD_SEND_NOTIFY_SIGNAL, 0,0) )
      {
         DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("pers_send_Notification_Signal => failed to write to pipe"), DLT_INT(errno));
         rval = EPERS_NOTIFY_SIG;
      }
   }
   else
   {
      rval = EPERS_NOTIFY_SIG;
   }

   return rval;
}


//---------------------------------------------------------------------------------------------------------
// C U R S O R    F U N C T I O N S
//---------------------------------------------------------------------------------------------------------

int get_cursor_handle()
{
   int handle = 0;

   if(pthread_mutex_lock(&gMtx) == 0)
   {
      if(gFreeCursorHandleIdxHead > 0)   // check if we have a free spot in the array before the current max
      {
         handle = gFreeCursorHandleArray[--gFreeCursorHandleIdxHead];
      }
      else
      {
         if(gHandleIdx < MaxPersHandle-1)
         {
            handle = gHandleIdx++;  // no free spot before current max, increment handle index
         }
         else
         {
            DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("get_cursor_handle ==> Reached maximum of open handles:"), DLT_INT(MaxPersHandle));
            handle = -1;
         }
      }
      pthread_mutex_unlock(&gMtx);
   }
   return handle;
}


void close_cursor_handle(int handlerDB)
{
   if(pthread_mutex_lock(&gMtx) == 0)
   {
      if(gFreeCursorHandleIdxHead < MaxPersHandle)
      {
         gFreeCursorHandleArray[gFreeCursorHandleIdxHead++] = handlerDB;
      }
      pthread_mutex_unlock(&gMtx);
   }
}



int pers_db_cursor_create(char* dbPath)
{
   int handle = -1;
   itzam_state  state = ITZAM_FAILED;

   handle = get_cursor_handle();

   if(handle < MaxPersHandle && handle >= 0)
   {
      // open database
      state = itzam_btree_open(&gCursorArray[handle].m_btree, dbPath, itzam_comparator_string, error_handler, 1/*recover*/, 0/*read_only*/);
      if (state != ITZAM_OKAY)
      {
         DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("pers_db_cursor_create ==> itzam_btree_open"), DLT_STRING(STATE_MESSAGES[state]));
      }
      else
      {
         itzam_state  state;

         state = itzam_btree_cursor_create(&gCursorArray[handle].m_cursor, &gCursorArray[handle].m_btree);
         if(state == ITZAM_OKAY)
         {
            gCursorArray[handle].m_empty = 0;
         }
         else
         {
            gCursorArray[handle].m_empty = 1;
         }
      }
   }
   return handle;
}



int pers_db_cursor_next(unsigned int handlerDB)
{
   int rval = -1;
   //if(handlerDB < MaxPersHandle && handlerDB >= 0)
   if(handlerDB < MaxPersHandle )
   {
      if(gCursorArray[handlerDB].m_empty != 1)
      {
         itzam_bool success;
         success = itzam_btree_cursor_next(&gCursorArray[handlerDB].m_cursor);

         if(success == itzam_true)
         {
            rval = 0;
         }
         else
         {
            rval = EPERS_LAST_ENTRY_IN_DB;
         }
      }
      else
      {
         DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("pers_db_cursor_next ==> invalid handle: "), DLT_INT(handlerDB));
      }
   }
   else
   {
      DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("pers_db_cursor_next ==> handle bigger than max:"), DLT_INT(MaxPersHandle));
   }
   return rval;
}



int pers_db_cursor_get_key(unsigned int handlerDB, char * bufKeyName_out, int bufSize)
{
   int rval = -1;
   KeyValuePair_s search;

   if(handlerDB < MaxPersHandle)
   {
      if(gCursorArray[handlerDB].m_empty != 1)
      {
         int length = 0;
         itzam_btree_cursor_read(&gCursorArray[handlerDB].m_cursor ,(void *)&search);
         length = strlen(search.m_key);
         if(length < bufSize)
         {
            memcpy(bufKeyName_out, search.m_key, length);
            rval = 0;
         }
         else
         {
            DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("pers_db_cursor_get_key  ==> buffer to small » keySize: "), DLT_INT(bufSize));
         }
      }
      else
      {
         DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("persistence_db_cursor_get_key  ==>  invalid handle:"), DLT_INT(handlerDB));
      }
   }
   else
   {
      DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("persistence_db_cursor_get_key ==> handle bigger than max:"), DLT_INT(MaxPersHandle));
   }
   return rval;
}



int pers_db_cursor_get_data(unsigned int handlerDB, char * bufData_out, int bufSize)
{
   int rval = -1;
   KeyValuePair_s search;

   if(handlerDB < MaxPersHandle)
   {
      if(gCursorArray[handlerDB].m_empty != 1)
      {
         int length = 0;
         itzam_btree_cursor_read(&gCursorArray[handlerDB].m_cursor ,(void *)&search);

         length = strlen(search.m_data);
         if(length < bufSize)
         {
            memcpy(bufData_out, search.m_data, length);
            rval = 0;
         }
         else
         {
            DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("pers_db_cursor_get_data  ==> buffer to small » keySize: "), DLT_INT(bufSize));
         }
      }
      else
      {
         DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("persistence_db_cursor_get_data  ==>  invalid handle:"), DLT_INT(handlerDB));
      }
   }
   else
   {
      DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("persistence_db_cursor_get_data ==> handle bigger than max:"), DLT_INT(MaxPersHandle));
   }
   return rval;
}



int pers_db_cursor_get_data_size(unsigned int handlerDB)
{
   int size = -1;
   KeyValuePair_s search;

   if(handlerDB < MaxPersHandle)
   {
      if(gCursorArray[handlerDB].m_empty != 1)
      {
         itzam_btree_cursor_read(&gCursorArray[handlerDB].m_cursor ,(void *)&search);
         size = strlen(search.m_data);
      }
      else
      {
         DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("pers_db_cursor_get_data_size  ==>  invalid handle:"), DLT_INT(handlerDB));
      }
   }
   else
   {
      DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("persistence_db_cursor_get_data  ==>  handle bigger than max:"), DLT_INT(MaxPersHandle));
   }
   return size;
}



int pers_db_cursor_destroy(unsigned int handlerDB)
{
   int rval = -1;
   if(handlerDB < MaxPersHandle)
   {
      itzam_btree_cursor_free(&gCursorArray[handlerDB].m_cursor);
      gCursorArray[handlerDB].m_empty = 1;

      itzam_state state = ITZAM_FAILED;
      state = itzam_btree_close(&gCursorArray[handlerDB].m_btree);
      if (state != ITZAM_OKAY)
      {
            DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("pers_db_cursor_destroy  ==>  itzam_btree_close: Itzam problem"), DLT_STRING(STATE_MESSAGES[state]));
      }

      close_cursor_handle(handlerDB);

      rval = 0;
   }
   return rval;
}




//-----------------------------------------------------------------------------
// code to print database content (for debugging)
//-----------------------------------------------------------------------------
// walk the database
/*
KeyValuePair_s  rec;
itzam_btree_cursor cursor;
state = itzam_btree_cursor_create(&cursor, &btree);
if(state == ITZAM_OKAY)
{
  printf("==> Database content ==> db size: %d\n", (int)itzam_btree_count(&btree));
  do
  {
     // get the key pointed to by the cursor
     state = itzam_btree_cursor_read(&cursor,(void *)&rec);
     if (state == ITZAM_OKAY)
     {
       printf("   Key: %s \n     ==> data: %s\n", rec.m_key, rec.m_data);
     }
     else
        fprintf(stderr, "\nItzam problem: %s\n", STATE_MESSAGES[state]);
  }
  while (itzam_btree_cursor_next(&cursor));

  state = itzam_btree_cursor_free(&cursor);
}
*/
//-----------------------------------------------------------------------------





