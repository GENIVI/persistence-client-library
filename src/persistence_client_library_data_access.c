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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <itzam.h>
#include "persistence_client_library_itzam_errors.h"


typedef struct _KeyValuePair_s
{
    char m_key[KeySize];
    char m_data[ValueSize];
}
KeyValuePair_s;


void error_handler(const char * function_name, itzam_error error)
{
    fprintf(stderr, "Itzam error in %s: %s\n", function_name, ERROR_STRINGS[error]);
}


#ifdef USE_GVDB
int set_value_to_table_gvdb(const char* dbPath, char* key, unsigned char* buffer, unsigned long buffer_size)
{
   int size_written = buffer_size;

   GError *error = NULL;
   GHashTable* hash_table = NULL;

   hash_table = gvdb_hash_table_new(NULL, NULL);

   if(hash_table != NULL)
   {
      gvdb_hash_table_insert_string(hash_table, key,  (const gchar*)buffer);

      gboolean success = gvdb_table_write_contents(hash_table, dbPath, FALSE, &error);
      if(success != TRUE)
      {
         printf("persistence_set_data => error: %s \n", error->message );
         g_error_free(error);
         error = NULL;
         size_written = EPERS_SETDTAFAILED;
      }
      else
      {
         printf("persistence_set_data - Database  E R R O R: %s\n", error->message);
         size_written = EPERS_NOPRCTABLE;
         g_error_free(error);
         error = NULL;
      }
   }
   return size_written;
}
#else
int set_value_to_table_itzam(const char* dbPath, char* key, unsigned char* buffer, unsigned long buffer_size)
{
   int size_written = buffer_size;
   itzam_btree  btree;
   itzam_state  state;
   KeyValuePair_s insert;

   //printf("set_value_to_table_itzam => Path: %s key: \"%s\" buffer: \"%s\" \n", dbPath, key, buffer);

   state = itzam_btree_open(&btree, dbPath, itzam_comparator_string, error_handler, 0/*recover*/, 0/*read_only*/);
   if(state == ITZAM_OKAY)
   {
      int keySize = 0;
      keySize = (int)strlen((const char*)key);
      if(keySize < KeySize)
      {
         int dataSize = 0;
         dataSize = (int)strlen( (const char*)buffer);
         if(dataSize < ValueSize)
         {
            memcpy(insert.m_key, key, keySize);
            if(itzam_true == itzam_btree_find(&btree, key, &insert))
            {
               // key already available, so delete "old" key
               state = itzam_btree_remove(&btree, (const void *)&insert);
            }
            memcpy(insert.m_data, buffer, dataSize);
            state = itzam_btree_insert(&btree,(const void *)&insert);
            if (state != ITZAM_OKAY)
            {
               fprintf(stderr, "\nInsert Itzam problem: %s\n", STATE_MESSAGES[state]);
            }
         }
         else
         {
            fprintf(stderr, "\nset_value_to_table_itzam => data to long » size %d | maxSize: %d\n", dataSize, KeySize);
         }

         state = itzam_btree_close(&btree);
         if (state != ITZAM_OKAY)
         {
            fprintf(stderr, "\nClose Itzam problem: %s\n", STATE_MESSAGES[state]);
         }
      }
      else
      {
         fprintf(stderr, "\nset_value_to_table_itzam => key to long » size: %d | maxSize: %d\n", keySize, KeySize);
      }
   }
   else
   {
      size_written = EPERS_NOPRCTABLE;
      fprintf(stderr, "\nOpen Itzam problem: %s\n", STATE_MESSAGES[state]);
   }

   return size_written;
}
#endif


#ifdef USE_GVDB
int get_value_from_table_gvdb(const char* dbPath, char* key, unsigned char* buffer, unsigned long buffer_size)
{
   GError *error = NULL;
   int read_size = 0;
   GvdbTable* database = gvdb_table_new(dbPath, TRUE, &error);;
   gvdb_table_ref(database);
   if(database != NULL)
   {
      GVariant* dbValue = NULL;

      dbValue = gvdb_table_get_value(database, key);
      
      if(dbValue != NULL)
      {
        gconstpointer valuePtr = NULL;

        read_size = g_variant_get_size(dbValue);
        valuePtr = g_variant_get_data(dbValue);   // get the "data" part from GVariant

        if( (valuePtr != NULL))
        {
           if(read_size > buffer_size)
           {
              read_size = buffer_size;   // truncate data size to buffer size
           }
           memcpy(buffer, valuePtr, read_size-1);
        }
        else
        {
           read_size = EPERS_NOKEYDATA;
           printf("get_value_from_table:  E R R O R getting size and/or data for key: %s \n", key);
        }
      }
      else
      {
        read_size = EPERS_NOKEY;
        printf("get_value_from_table:  E R R O R  getting value for key: %s \n", key);
      }
      gvdb_table_unref(database);
   }
   else
   {
     read_size = EPERS_NOPRCTABLE;
     printf("persistence_get_data - Database  E R R O R: %s\n", error->message);
     g_error_free(error);
     error = NULL;
   }
   return read_size;
} 
#else
int get_value_from_table_itzam(const char* dbPath, char* key, unsigned char* buffer, unsigned long buffer_size)
{
   int read_size = 0;
   itzam_btree  btree;
   itzam_state  state;
   KeyValuePair_s search;

   //printf("get_value_from_table_itzam => Path: %s key: \"%s\" \n", dbPath, key);

   state = itzam_btree_open(&btree, dbPath, itzam_comparator_string, error_handler, 0/*recover*/, 0/*read_only*/);
   if(state == ITZAM_OKAY)
   {
      if(itzam_true == itzam_btree_find(&btree, key, &search))
      {
         read_size = strlen(search.m_data);
         if(read_size > buffer_size)
         {
            read_size = buffer_size;   // truncate data size to buffer size
         }
         memcpy(buffer, search.m_data, read_size);
         //printf("get_value_from_table_itzam => \n m_data: %s \n buffer: %s \n", search.m_data, buffer);
      }
      else
      {
         read_size = EPERS_NOKEY;
      }

      state = itzam_btree_close(&btree);
      if (state != ITZAM_OKAY)
      {
         fprintf(stderr, "\nClose Itzam problem: %s\n", STATE_MESSAGES[state]);
      }
   }
   else
   {
      read_size = EPERS_NOPRCTABLE;
      fprintf(stderr, "\nOpen Itzam problem: %s\n", STATE_MESSAGES[state]);
   }
   return read_size;
}
#endif


#ifdef USE_GVDB
int get_size_from_table_gvdb(const char* dbPath, char* key)
{
   GError *error = NULL;
   int read_size = 0;
   GvdbTable* database = gvdb_table_new(dbPath, TRUE, &error);;

   if(database != 0)
   {
      GVariant* dbValue = gvdb_table_get_value(database, key);

      if(dbValue != NULL)
      {
         read_size = g_variant_get_size(dbValue);
      }
      else
      {
         read_size = EPERS_NOKEY;
         printf("get_size_from_table:  E R R O R getting value for key: %s \n", key);
      }
      gvdb_table_unref(database);
   }
   else
   {
      read_size = EPERS_NOPRCTABLE;
      printf("persistence_get_data_size - Database  E R R O R: %s\n", error->message);
      g_error_free(error);
      error = NULL;
   }
   return read_size;
}
#else
int get_size_from_table_itzam(const char* dbPath, char* key)
{
   int read_size = 0;
     itzam_btree  btree;
     itzam_state  state;
     KeyValuePair_s search;

     //printf("get_value_from_table_itzam => Path: %s key: \"%s\" \n", dbPath, key);

     state = itzam_btree_open(&btree, dbPath, itzam_comparator_string, error_handler, 0/*recover*/, 0/*read_only*/);
     if(state == ITZAM_OKAY)
     {
        if(itzam_true == itzam_btree_find(&btree, key, &search))
        {
           read_size = strlen(search.m_data);
        }
        else
        {
           read_size = EPERS_NOKEY;
        }

        state = itzam_btree_close(&btree);
        if (state != ITZAM_OKAY)
        {
           fprintf(stderr, "\nClose Itzam problem: %s\n", STATE_MESSAGES[state]);
        }
     }
     else
     {
        read_size = EPERS_NOPRCTABLE;
        fprintf(stderr, "\nOpen Itzam problem: %s\n", STATE_MESSAGES[state]);
     }
     return read_size;
}
#endif


int persistence_get_data(char* dbPath, char* key, PersistenceStorage_e storage, unsigned char* buffer, unsigned long buffer_size)
{
   int read_size = -1;

   if(PersistenceStorage_shared == storage)       // check if shared data (dconf)
   {
      printf("    S H A R E D   D A T A  => not implemented yet\n");
      //DConfClient* dconf_client_new(const gchar *profile, DConfWatchFunc watch_func, gpointer user_data, GDestroyNotify notify);

      //GVariant* dconf_client_read(DConfClient *client, const gchar *key);
      strncpy((char*)buffer, "S H A R E D   D A T A  => not implemented yet", buffer_size-1);
   }
   else if(PersistenceStorage_local == storage)   // it is local data (gvdb)
   {
#ifdef USE_GVDB
      read_size = get_value_from_table_gvdb(dbPath, key, buffer, buffer_size-1);
#else
      read_size = get_value_from_table_itzam(dbPath, key, buffer, buffer_size-1);
#endif
   }
   else if(PersistenceStorage_custom == storage)   // custom storage implementation via custom library
   {
      int idx =  custom_client_name_to_id(dbPath, 1);
      char workaroundPath[128];  // workaround, because /sys/ can not be accessed on host!!!!
      snprintf(workaroundPath, 128, "%s%s", "/tmp", dbPath  );

      printf("    get C U S T O M   D A T A  => not implemented yet - path: %s | index: %d \n", dbPath , idx);

      if( (idx < PersCustomLib_LastEntry) && (gPersCustomFuncs[idx].custom_plugin_handle_get_data != NULL) )
      {
         gPersCustomFuncs[idx].custom_plugin_handle_get_data(88, (char*)buffer, buffer_size-1);
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

   if(PersistenceStorage_shared == storage)       // check if shared data (dconf)
   {
      //GVariant *value = NULL;
      //gboolean ok = FALSE;

      printf("    S H A R E D   D A T A  => NOW IMPLEMENTING implemented yet\n");
      //DConfDBusClient *dcdbc = dconf_dbus_client_new("/com/canonical/indicator/power/", NULL, NULL);
      //ok =  dconf_dbus_client_write(dcdbc, key, value);

   }
   else if(PersistenceStorage_local == storage)   // it is local data (gvdb)
   {
#ifdef USE_GVDB
      write_size = set_value_to_table_gvdb(dbPath, key, buffer, buffer_size);
#else
      write_size = set_value_to_table_itzam(dbPath, key, buffer, buffer_size);
#endif
   }
   else if(PersistenceStorage_custom == storage)   // custom storage implementation via custom library
   {
      int idx = custom_client_name_to_id(dbPath, 1);
      printf("    set C U S T O M   D A T A  => not implemented yet - path: %s | index: %d \n", dbPath , idx);
      if((idx < PersCustomLib_LastEntry) && (gPersCustomFuncs[idx].custom_plugin_handle_set_data) )
      {
         gPersCustomFuncs[idx].custom_plugin_handle_set_data(88, (char*)buffer, buffer_size);
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

   if(PersistenceStorage_shared == storage)       // check if shared data (dconf)
   {
      printf("S H A R E D  D A T A  => not implemented yet\n");
   }
   else if(PersistenceStorage_local == storage)   // it is local data (gvdb)
   {
#ifdef USE_GVDB
      read_size = get_size_from_table_gvdb(dbPath, key);
#else
      read_size = get_size_from_table_itzam(dbPath, key);
#endif
   }
   else if(PersistenceStorage_custom == storage)   // custom storage implementation via custom library
   {
      printf("    get C U S T O M   D A T A  => NOW IMPLEMENTING implemented yet\n");
   }

   return read_size;

}






int persistence_reg_notify_on_change(char* dbPath, char* key)
{
   int rval = -1;

   return rval;
}





