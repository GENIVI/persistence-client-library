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
 * @file           persistence_client_library_data_access.c
 * @ingroup        Persistence client library
 * @author         Ingo Huerner
 * @brief          Implementation of persistence data access
 *                 Library provides an API to access persistent data
 * @see            
 */

#include "persistence_client_library_data_access.h"
#include "persistence_client_library_custom_loader.h"
#include "persistence_client_library_itzam_errors.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <itzam.h>


typedef struct _KeyValuePair_s
{
    char m_key[KeySize];
    char m_data[ValueSize];
}
KeyValuePair_s;


/// btree array
static itzam_btree gBtree[2];
static int gBtreeCreated[] = { 0, 0 };


void error_handler(const char * function_name, itzam_error error)
{
    fprintf(stderr, "Itzam error in %s: %s\n", function_name, ERROR_STRINGS[error]);
}



itzam_btree* database_get(PersistenceStorage_e storage, const char* dbPath)
{
   itzam_btree* btree = NULL;
   if(   (storage >= PersistenceStorage_local)
      && (storage <= PersistenceStorage_shared)  )
   {
      if(gBtreeCreated[storage] == 0)
      {
         itzam_state  state = ITZAM_FAILED;
         state = itzam_btree_open(&gBtree[storage], dbPath, itzam_comparator_string, error_handler, 0/*recover*/, 0/*read_only*/);
         if (state != ITZAM_OKAY)
         {
            fprintf(stderr, "Open Itzam problem: %s\n", STATE_MESSAGES[state]);
         }
         gBtreeCreated[storage] = 1;
      }
      // return tree
      btree = &gBtree[storage];
   }
   else
   {
      printf("btree_get ==> invalid storage type\n");
   }

   return btree;
}


void database_close(PersistenceStorage_e storage)
{
   if(   (storage >= PersistenceStorage_local)
      && (storage <= PersistenceStorage_shared)  )
   {
      itzam_state  state = ITZAM_FAILED;
      state = itzam_btree_close(&gBtree[storage]);
      if (state != ITZAM_OKAY)
      {
         fprintf(stderr, "Close Itzam problem: %s\n", STATE_MESSAGES[state]);
      }
      gBtreeCreated[storage] = 0;
   }
   else
   {
      printf("database_close ==> invalid storage type\n");
   }
}





int persistence_get_data(char* dbPath, char* key, PersistenceStorage_e storage, unsigned char* buffer, unsigned long buffer_size)
{
   int read_size = -1;

   if(PersistenceStorage_shared == storage || PersistenceStorage_local == storage)
   {
      itzam_btree* btree = NULL;
      itzam_state  state = ITZAM_FAILED;
      KeyValuePair_s search;

      btree = database_get(storage, dbPath);
      if(btree != NULL)
      {
         if(itzam_true == itzam_btree_find(btree, key, &search))
         {
            read_size = strlen(search.m_data);
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
         database_close(storage);
      }
      else
      {
         read_size = EPERS_NOPRCTABLE;
         fprintf(stderr, "\nOpen Itzam problem: %s\n", STATE_MESSAGES[state]);
      }
   }
   else if(PersistenceStorage_custom == storage)   // custom storage implementation via custom library
   {
      int idx =  custom_client_name_to_id(dbPath, 1);
      char workaroundPath[128];  // workaround, because /sys/ can not be accessed on host!!!!
      snprintf(workaroundPath, 128, "%s%s", "/Data", dbPath  );

      if( (idx < PersCustomLib_LastEntry) && (gPersCustomFuncs[idx].custom_plugin_handle_get_data != NULL) )
      {
         gPersCustomFuncs[idx].custom_plugin_get_data(key, (char*)buffer, buffer_size);
      }
      else
      {
         read_size = EPERS_NOPLUGINFUNCT;
      }
   }
   return read_size;
}



int persistence_set_data(char* dbPath, char* key, PersistenceStorage_e storage, unsigned char* buffer, unsigned long buffer_size)
{
   int write_size = -1;

   if(PersistenceStorage_shared == storage || PersistenceStorage_local == storage)
   {
      write_size = buffer_size;
      itzam_btree* btree = NULL;
      itzam_state  state = ITZAM_FAILED;
      KeyValuePair_s insert;

      btree = database_get(storage, dbPath);
      if(btree != NULL)
      {
         int keySize = 0;
         keySize = (int)strlen((const char*)key);
         if(keySize < KeySize)
         {
            int dataSize = 0;
            dataSize = (int)strlen( (const char*)buffer);
            if(dataSize < ValueSize)
            {
               memset(insert.m_key, 0, KeySize);
               memcpy(insert.m_key, key, keySize);
               if(itzam_true == itzam_btree_find(btree, key, &insert))
               {
                  // key already available, so delete "old" key
                  state = itzam_btree_remove(btree, (const void *)&insert);
               }
               memset(insert.m_data, 0, ValueSize);
               memcpy(insert.m_data, buffer, dataSize);
               state = itzam_btree_insert(btree,(const void *)&insert);
               if (state != ITZAM_OKAY)
               {
                  fprintf(stderr, "\nInsert Itzam problem: %s\n", STATE_MESSAGES[state]);
               }
            }
            else
            {
               fprintf(stderr, "\nset_value_to_table_itzam => data to long » size %d | maxSize: %d\n", dataSize, KeySize);
            }

            //
            // workaround till lifecycle is working correctly
            //
            database_close(storage);
         }
         else
         {
            fprintf(stderr, "\nset_value_to_table_itzam => key to long » size: %d | maxSize: %d\n", keySize, KeySize);
         }
      }
      else
      {
         write_size = EPERS_NOPRCTABLE;
         fprintf(stderr, "\nOpen Itzam problem: %s\n", STATE_MESSAGES[state]);
      }
   }
   else if(PersistenceStorage_custom == storage)   // custom storage implementation via custom library
   {
      int idx = custom_client_name_to_id(dbPath, 1);
      if((idx < PersCustomLib_LastEntry) && (gPersCustomFuncs[idx].custom_plugin_handle_set_data) )
      {
         gPersCustomFuncs[idx].custom_plugin_set_data(key, (char*)buffer, buffer_size);
      }
      else
      {
         write_size = EPERS_NOPLUGINFUNCT;
      }
   }
   return write_size;
}



int persistence_get_data_size(char* dbPath, char* key, PersistenceStorage_e storage)
{
   int read_size = -1;

   if(PersistenceStorage_shared == storage || PersistenceStorage_local == storage)
   {
      int keySize = 0;
      itzam_btree*  btree = NULL;
      itzam_state  state = ITZAM_FAILED;
      KeyValuePair_s search;

      btree = database_get(storage, dbPath);
      if(btree != NULL)
      {
         keySize = (int)strlen((const char*)key);
         if(keySize < KeySize)
         {
            memset(search.m_key,0, KeySize);
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
         //
         // workaround till lifecycle is working correctly
         //
         database_close(storage);
      }
      else
      {
         read_size = EPERS_NOPRCTABLE;
         fprintf(stderr, "\nOpen Itzam problem: %s\n", STATE_MESSAGES[state]);
      }
   }
   else if(PersistenceStorage_custom == storage)   // custom storage implementation via custom library
   {
      int idx = custom_client_name_to_id(dbPath, 1);
      if((idx < PersCustomLib_LastEntry) && (gPersCustomFuncs[idx].custom_plugin_handle_set_data) )
      {
         gPersCustomFuncs[idx].custom_plugin_get_size(key);
      }
      else
      {
         read_size = EPERS_NOPLUGINFUNCT;
      }
   }
   return read_size;
}



int persistence_delete_data(char* dbPath, char* dbKey, PersistenceStorage_e storage)
{
   int ret = 0;
   if(PersistenceStorage_custom != storage)
   {
      itzam_btree*  btree = NULL;
      KeyValuePair_s delete;

      //printf("delete_key_from_table_itzam => Path: \"%s\" | key: \"%s\" \n", dbPath, key);
      btree = database_get(storage, dbPath);
      if(btree != NULL)
      {
         int keySize = 0;
         keySize = (int)strlen((const char*)dbKey);
         if(keySize < KeySize)
         {
            itzam_state  state;

            memset(delete.m_key,0, KeySize);
            memcpy(delete.m_key, dbKey, keySize);
            state = itzam_btree_remove(btree, (const void *)&delete);
            if (state != ITZAM_OKAY)
            {
               fprintf(stderr, "Remove Itzam problem: %s\n", STATE_MESSAGES[state]);
            }
         }
         else
         {
            fprintf(stderr, "persistence_delete_data => key to long » size: %d | maxSize: %d\n", keySize, KeySize);
         }
         //
         // workaround till lifecycle is working correctly
         //
         database_close(storage);
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
         gPersCustomFuncs[idx].custom_plugin_delete_data(dbKey);
      }
      else
      {
         ret = EPERS_NOPLUGINFUNCT;
      }
   }
   return ret;
}


int persistence_reg_notify_on_change(char* dbPath, char* key)
{
   int rval = -1;

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





