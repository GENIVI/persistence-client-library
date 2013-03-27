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


/// definition of a key-value pair stored in the database
typedef struct _KeyValuePair_s
{
    char m_key[DbKeySize];       /// the key
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


itzam_btree* pers_db_open(PersistenceInfo_s* info, const char* dbPath)
{
   int arrayIdx = 0;
   itzam_btree* btree = NULL;

   // create array index: index is a combination of resource config table type and group
   arrayIdx = info->configKey.storage + info->context.ldbid ;

   //if(arrayIdx <= DbTableSize)
   if(arrayIdx < DbTableSize)
   {
      if(gBtreeCreated[arrayIdx][info->configKey.policy] == 0)
      {
         itzam_state  state = ITZAM_FAILED;
         state = itzam_btree_open(&gBtree[arrayIdx][info->configKey.policy], dbPath,
                                  itzam_comparator_string, error_handler, 0/*recover*/, 0/*read_only*/);
         if (state != ITZAM_OKAY)
         {
            fprintf(stderr, "pers_db_open ==> Open Itzam problem: %s\n", STATE_MESSAGES[state]);
         }
         gBtreeCreated[arrayIdx][info->configKey.policy] = 1;
      }
      // assign database
      btree = &gBtree[arrayIdx][info->configKey.policy];
   }
   else
   {
      printf("btree_get ==> invalid storage type\n");
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
         fprintf(stderr, "pers_db_close ==> Close Itzam problem: %s\n", STATE_MESSAGES[state]);
      }
      gBtreeCreated[arrayIdx][info->configKey.policy] = 0;
   }
   else
   {
      printf("pers_db_close ==> invalid storage type\n");
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
            fprintf(stderr, "pers_db_close_all ==> Close WC: Itzam problem: %s\n", STATE_MESSAGES[state]);
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
            fprintf(stderr, "pers_db_close_all ==> Close WT: Itzam problem: %s\n", STATE_MESSAGES[state]);
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
      itzam_state  state = ITZAM_FAILED;
      KeyValuePair_s search;

      btree = pers_db_open(info, dbPath);
      if(btree != NULL)
      {
         if(itzam_true == itzam_btree_find(btree, key, &search))
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
            read_size = EPERS_NOKEY;
         }

         //
         // workaround till lifecycle is working correctly
         //
         pers_db_close(info);
      }
      else
      {
         read_size = EPERS_NOPRCTABLE;
         fprintf(stderr, "\npersistence_get_data ==> Open Itzam problem: %s\n", STATE_MESSAGES[state]);
      }
   }
   else if(PersistenceStorage_custom == info->configKey.storage)   // custom storage implementation via custom library
   {
      int idx =  custom_client_name_to_id(dbPath, 1);
      char workaroundPath[128];  // workaround, because /sys/ can not be accessed on host!!!!
      snprintf(workaroundPath, 128, "%s%s", "/Data", dbPath  );

      if( (idx < PersCustomLib_LastEntry) && (gPersCustomFuncs[idx].custom_plugin_handle_get_data != NULL) )
      {
         if(info->configKey.customID[0] == '\0')   // if we have not a customID we use the key
            gPersCustomFuncs[idx].custom_plugin_get_data(key, (char*)buffer, buffer_size);
         else
            gPersCustomFuncs[idx].custom_plugin_get_data(info->configKey.customID, (char*)buffer, buffer_size);
      }
      else
      {
         read_size = EPERS_NOPLUGINFUNCT;
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
                  fprintf(stderr, "\npersistence_set_data ==> Insert Itzam problem: %s\n", STATE_MESSAGES[state]);
                  write_size = EPERS_DB_ERROR_INTERNAL;
               }

               if(PersistenceStorage_shared == info->configKey.storage)
               {
                  // send changed notification
                  DBusMessage* message;
                  char ldbid_array[12];
                  char user_array[12];
                  char seat_array[12];
                  const char* ldbid_ptr = ldbid_array;
                  const char* user_ptr = user_array;
                  const char* seat_ptr = seat_array;

                  memset(ldbid_array, 0, 12);
                  memset(user_array, 0, 12);
                  memset(seat_array, 0, 12);

                  // dbus_bus_add_match is used for the notification mechanism,
                  // and this works only for type DBUS_TYPE_STRING as message arguments
                  // this is the reason to use string instead of integer types directly
                  snprintf(ldbid_array, 12, "%d", info->context.ldbid);
                  snprintf(user_array,  12, "%d", info->context.user_no);
                  snprintf(seat_array,  12, "%d", info->context.seat_no);

                  message = dbus_message_new_signal("/org/genivi/persistence/adminconsumer",     // const char *path,
                                                    "org.genivi.persistence.adminconsumer",      // const char *interface,
                                                    "PersistenceValueChanged" );                 // const char *name

                  dbus_message_append_args(message,
                                           DBUS_TYPE_STRING, &key,
                                           DBUS_TYPE_STRING, &ldbid_ptr,
                                           DBUS_TYPE_STRING, &user_ptr,
                                           DBUS_TYPE_STRING, &seat_ptr,
                                           DBUS_TYPE_INVALID);

                   // Send the signal
                   dbus_connection_send(get_dbus_connection(), message, NULL);

                   // Free the signal now we have finished with it
                   dbus_message_unref(message);
               }
            }
            else
            {
               fprintf(stderr, "\npersistence_set_data ==> set_value_to_table_itzam => data to long » size %d | maxSize: %d\n", dataSize, DbKeySize);
               write_size = EPERS_DB_VALUE_SIZE;
            }

            //
            // workaround till lifecycle is working correctly
            //
            pers_db_close(info);
         }
         else
         {
            fprintf(stderr, "\nset_value_to_table_itzam => key to long » size: %d | maxSize: %d\n", keySize, DbKeySize);
            write_size = EPERS_DB_KEY_SIZE;
         }
      }
      else
      {
         write_size = EPERS_NOPRCTABLE;
         fprintf(stderr, "\npersistence_set_data ==> Open Itzam problem: %s\n", STATE_MESSAGES[state]);
      }
   }
   else if(PersistenceStorage_custom == info->configKey.storage)   // custom storage implementation via custom library
   {
      int idx = custom_client_name_to_id(dbPath, 1);
      if((idx < PersCustomLib_LastEntry) && (gPersCustomFuncs[idx].custom_plugin_handle_set_data) )
      {
         if(info->configKey.customID[0] == '\0')   // if we have not a customID we use the key
            gPersCustomFuncs[idx].custom_plugin_set_data(key, (char*)buffer, buffer_size);
         else
            gPersCustomFuncs[idx].custom_plugin_set_data(info->configKey.customID, (char*)buffer, buffer_size);
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
      itzam_state  state = ITZAM_FAILED;
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
               read_size = strlen(search.m_data);
            }
            else
            {
               read_size = EPERS_NOKEY;
            }
         }
         else
         {
            fprintf(stderr, "persistence_get_data_size => key to long » size: %d | maxSize: %d\n", keySize, DbKeySize);
            read_size = EPERS_DB_KEY_SIZE;
         }
         //
         // workaround till lifecycle is working correctly
         //
         pers_db_close(info);
      }
      else
      {
         read_size = EPERS_NOPRCTABLE;
         fprintf(stderr, "\npersistence_get_data_size ==> Open Itzam problem: %s\n", STATE_MESSAGES[state]);
      }
   }
   else if(PersistenceStorage_custom == info->configKey.storage)   // custom storage implementation via custom library
   {
      int idx = custom_client_name_to_id(dbPath, 1);
      if((idx < PersCustomLib_LastEntry) && (gPersCustomFuncs[idx].custom_plugin_handle_set_data) )
      {
         if(info->configKey.customID[0] == '\0')   // if we have not a customID we use the key
            gPersCustomFuncs[idx].custom_plugin_get_size(key);
         else
            gPersCustomFuncs[idx].custom_plugin_get_size(info->configKey.customID);
      }
      else
      {
         read_size = EPERS_NOPLUGINFUNCT;
      }
   }
   return read_size;
}



int pers_db_delete_key(char* dbPath, char* dbKey, PersistenceInfo_s* info)
{
   int ret = 0;
   if(PersistenceStorage_custom != info->configKey.storage)
   {
      itzam_btree*  btree = NULL;
      KeyValuePair_s delete;

      //printf("delete_key_from_table_itzam => Path: \"%s\" | key: \"%s\" \n", dbPath, key);
      btree = pers_db_open(info, dbPath);
      if(btree != NULL)
      {
         int keySize = 0;
         keySize = (int)strlen((const char*)dbKey);
         if(keySize < DbKeySize)
         {
            itzam_state  state;

            memset(delete.m_key,0, DbKeySize);
            memcpy(delete.m_key, dbKey, keySize);
            state = itzam_btree_remove(btree, (const void *)&delete);
            if (state != ITZAM_OKAY)
            {
               fprintf(stderr, "persistence_delete_data ==> Remove Itzam problem: %s\n", STATE_MESSAGES[state]);
               ret = EPERS_DB_ERROR_INTERNAL;
            }
         }
         else
         {
            fprintf(stderr, "persistence_delete_data => key to long » size: %d | maxSize: %d\n", keySize, DbKeySize);
            ret = EPERS_DB_KEY_SIZE;
         }
         //
         // workaround till lifecycle is working correctly
         //
         pers_db_close(info);
      }
      else
      {
         fprintf(stderr, "persistence_delete_data => no prct table\n");
         ret = EPERS_NOPRCTABLE;
      }
   }
   else   // custom storage implementation via custom library
   {
      int idx = custom_client_name_to_id(dbPath, 1);
      if((idx < PersCustomLib_LastEntry) && (gPersCustomFuncs[idx].custom_plugin_handle_set_data) )
      {
         if(info->configKey.customID[0] == '\0')   // if we have not a customID we use the key
            gPersCustomFuncs[idx].custom_plugin_delete_data(dbKey);
         else
            gPersCustomFuncs[idx].custom_plugin_delete_data(info->configKey.customID);
      }
      else
      {
         ret = EPERS_NOPLUGINFUNCT;
      }
   }
   return ret;
}


int persistence_reg_notify_on_change(char* dbPath, char* key, unsigned int ldbid, unsigned int user_no, unsigned int seat_no,
                                     changeNotifyCallback_t callback)
{
   int rval = -1;
   DBusError error;
   dbus_error_init (&error);
   char rule[300];
   char ldbid_array[12];
   char user_array[12];
   char seat_array[12];

   memset(ldbid_array, 0, 12);
   memset(user_array, 0, 12);
   memset(seat_array, 0, 12);

   // assign callback
   gChangeNotifyCallback = callback;

   // dbus_bus_add_match works only for type DBUS_TYPE_STRING as message arguments
   // this is the reason to use string instead of integer types directly
   snprintf(ldbid_array, 12, "%d", ldbid);
   snprintf(user_array,  12, "%d", user_no);
   snprintf(seat_array,  12, "%d", seat_no);

   snprintf(rule, 256, "type='signal',interface='org.genivi.persistence.adminconsumer',member='PersistenceValueChanged',path='/org/genivi/persistence/adminconsumer',arg0='%s',arg1='%s',arg2='%s',arg3='%s'",
            key, ldbid_array, user_array, seat_array);

   dbus_bus_add_match(get_dbus_connection(), rule, &error);

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
            handle = -1;
            printf("get_persistence_handle_idx => Reached maximum of open handles: %d \n", MaxPersHandle);
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
      state = itzam_btree_open(&gCursorArray[handle].m_btree, dbPath, itzam_comparator_string, error_handler, 0/*recover*/, 0/*read_only*/);
      if (state != ITZAM_OKAY)
      {
         fprintf(stderr, "pers_db_open ==> Open Itzam problem: %s\n", STATE_MESSAGES[state]);
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
         printf("persistence_db_cursor_get_key ==> invalid handle: %u \n", handlerDB);
      }
   }
   else
   {
      printf("persistence_db_cursor_get_key ==> handle bigger than max » handleDB: %u | max: : %d \n", handlerDB, MaxPersHandle);
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
            printf("persistence_db_cursor_get_key ==> buffer to small » keySize: %d | bufSize: %d \n", length, bufSize);
         }
      }
      else
      {
         printf("persistence_db_cursor_get_key ==> invalid handle: %u \n", handlerDB);
      }
   }
   else
   {
      printf("persistence_db_cursor_get_key ==> handle bigger than max » handleDB: %u | max: : %d \n", handlerDB, MaxPersHandle);
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
            printf("persistence_db_cursor_get_data ==> buffer to small » keySize: %d | bufSize: %d \n", length, bufSize);
         }
      }
      else
      {
         printf("persistence_db_cursor_get_data ==> invalid handle: %u \n", handlerDB);
      }
   }
   else
   {
      printf("persistence_db_cursor_get_data ==> handle bigger than max » handleDB: %u | max: : %d \n", handlerDB, MaxPersHandle);
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
         printf("persistence_db_cursor_get_data ==> invalid handle: %u \n", handlerDB);
      }
   }
   else
   {
      printf("persistence_db_cursor_get_data ==> handle bigger than max » handleDB: %u | max: : %d \n", handlerDB, MaxPersHandle);
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
            fprintf(stderr, "pers_db_cursor_destroy ==> Close: Itzam problem: %s\n", STATE_MESSAGES[state]);
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





