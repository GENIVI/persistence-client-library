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
 * @file           persistence_client_library_key.c
 * @ingroup        Persistence client library
 * @author         Ingo Huerner
 * @brief          Implementation of the persistence client library.
 *                 Library provides an API to access persistent data
 * @see            
 */

#include "persistence_client_library_key.h"

#include "../include_protected/persistence_client_library_db_access.h"
#include "../include_protected/persistence_client_library_rc_table.h"
#include "../include_protected/crc32.h"

#include "persistence_client_library_handle.h"
#include "persistence_client_library_pas_interface.h"
#include "persistence_client_library_prct_access.h"
#include "persistence_client_library_custom_loader.h"


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// function with handle
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

int key_handle_open(unsigned int ldbid, const char* resource_id, unsigned int user_no, unsigned int seat_no)
{
   int handle = 0;
   PersistenceInfo_s dbContext;

   char dbKey[DbKeyMaxLen];      // database key
   char dbPath[DbPathMaxLen];    // database location

   memset(dbKey, 0, DbKeyMaxLen);
   memset(dbPath, 0, DbPathMaxLen);

   dbContext.context.ldbid   = ldbid;
   dbContext.context.seat_no = seat_no;
   dbContext.context.user_no = user_no;

   // get database context: database path and database key
   handle = get_db_context(&dbContext, resource_id, ResIsNoFile, dbKey, dbPath);
   if(handle >= 0)
   {
      if(dbContext.configKey.storage < PersistenceStoragePolicy_LastEntry)  // check if store policy is valid
      {
         if(PersistenceStorage_custom ==  dbContext.configKey.storage)
         {
            int idx =  custom_client_name_to_id(dbPath, 1);
            char workaroundPath[128];  // workaround, because /sys/ can not be accessed on host!!!!
            snprintf(workaroundPath, 128, "%s%s", "/Data", dbPath  );

            if( (idx < PersCustomLib_LastEntry) && (gPersCustomFuncs[idx].custom_plugin_handle_open != NULL) )
            {
               int flag = 0, mode = 0;
               handle = gPersCustomFuncs[idx].custom_plugin_handle_open(workaroundPath, flag, mode);
            }
            else
            {
               handle = EPERS_NOPLUGINFUNCT;
            }
         }
         else
         {
            handle = get_persistence_handle_idx();
         }

         if(handle < MaxPersHandle && handle != -1)
         {
            // remember data in handle array
            strncpy(gHandleArray[handle].dbPath, dbPath, DbPathMaxLen);
            strncpy(gHandleArray[handle].dbKey, dbKey,   DbKeyMaxLen);
            gHandleArray[handle].info = dbContext;
         }
         else
         {
            printf("key_handle_open: error - handleId out of bounds [%d]\n", handle);
         }
      }
   }


   return handle;
}



int key_handle_close(int key_handle)
{
   int rval = 0;

   if(key_handle < MaxPersHandle)
   {
      if(PersistenceStorage_custom == gHandleArray[key_handle].info.configKey.storage )
      {
         int idx =  custom_client_name_to_id(gHandleArray[key_handle].dbPath, 1);

         if( (idx < PersCustomLib_LastEntry) && (gPersCustomFuncs[idx].custom_plugin_handle_close) )
         {
            gPersCustomFuncs[idx].custom_plugin_handle_close(key_handle);
         }
         else
         {
            rval = EPERS_NOPLUGINFUNCT;
         }
      }
      else
      {
         set_persistence_handle_close_idx(key_handle);
      }

      // invalidate entries
      strncpy(gHandleArray[key_handle].dbPath, "", DbPathMaxLen);
      strncpy(gHandleArray[key_handle].dbKey  ,"", DbKeyMaxLen);
      gHandleArray[key_handle].info.configKey.storage = -1;
   }
   else
   {
      rval = -1;
   }

   return rval;
}



int key_handle_get_size(int key_handle)
{
   int size = 0;

   if(key_handle < MaxPersHandle)
   {
      if(PersistenceStorage_custom ==  gHandleArray[key_handle].info.configKey.storage)
      {
         int idx =  custom_client_name_to_id(gHandleArray[key_handle].dbPath, 1);

         if(idx < PersCustomLib_LastEntry && &(gPersCustomFuncs[idx].custom_plugin_get_size) != NULL)
         {
            gPersCustomFuncs[idx].custom_plugin_get_size(gHandleArray[key_handle].dbPath);
         }
         else
         {
            size = EPERS_NOPLUGINFUNCT;
         }
      }
      else
      {
         size = pers_db_get_key_size(gHandleArray[key_handle].dbPath, gHandleArray[key_handle].dbKey,
                                          &gHandleArray[key_handle].info);
      }
   }

   return size;
}



int key_handle_read_data(int key_handle, unsigned char* buffer, int buffer_size)
{
   int size = 0;
   if(key_handle < MaxPersHandle)
   {
      if(PersistenceStorage_custom ==  gHandleArray[key_handle].info.configKey.storage)
      {
         int idx =  custom_client_name_to_id(gHandleArray[key_handle].dbPath, 1);

         if(idx < PersCustomLib_LastEntry && &(gPersCustomFuncs[idx].custom_plugin_handle_get_data) != NULL)
         {
            gPersCustomFuncs[idx].custom_plugin_handle_get_data(key_handle, (char*)buffer, buffer_size-1);
         }
         else
         {
            size = EPERS_NOPLUGINFUNCT;
         }
      }
      else
      {
         size = pers_db_read_key(gHandleArray[key_handle].dbPath, gHandleArray[key_handle].dbKey,
                                     &gHandleArray[key_handle].info, buffer, buffer_size);
      }
   }

   return size;
}



int key_handle_register_notify_on_change(int key_handle)
{
   int rval = -1;

   return rval;
}



int key_handle_write_data(int key_handle, unsigned char* buffer, int buffer_size)
{
   int size = 0;

   if(AccessNoLock != isAccessLocked() )     // check if access to persistent data is locked
   {
      if(buffer_size <= gMaxKeyValDataSize)  // check data size
      {
         if(key_handle < MaxPersHandle)
         {
            if(PersistenceStorage_custom ==  gHandleArray[key_handle].info.configKey.storage)
            {
               int idx =  custom_client_name_to_id(gHandleArray[key_handle].dbPath, 1);

               if(idx < PersCustomLib_LastEntry && *gPersCustomFuncs[idx].custom_plugin_handle_set_data != NULL)
               {
                  gPersCustomFuncs[idx].custom_plugin_handle_set_data(key_handle, (char*)buffer, buffer_size-1);
               }
               else
               {
                  size = EPERS_NOPLUGINFUNCT;
               }
            }
            else
            {
               size = pers_db_write_key(gHandleArray[key_handle].dbPath, gHandleArray[key_handle].dbKey,
                                           &gHandleArray[key_handle].info, buffer, buffer_size);
            }
         }
         else
         {
            size = EPERS_MAXHANDLE;
         }
      }
      else
      {
         printf("key_handle_write_data: error - buffer_size to big, limit is [%d] bytes\n", gMaxKeyValDataSize);
      }
   }
   else
   {
      size = EPERS_LOCKFS;
   }

   return size;
}





// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// functions to be used directly without a handle
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

int key_delete(unsigned int ldbid, const char* resource_id, unsigned int user_no, unsigned int seat_no)
{
   int rval = 0;

   if(AccessNoLock != isAccessLocked() ) // check if access to persistent data is locked
   {
      PersistenceInfo_s dbContext;

     char dbKey[DbKeyMaxLen];      // database key
     char dbPath[DbPathMaxLen];    // database location

     memset(dbKey, 0, DbKeyMaxLen);
     memset(dbPath, 0, DbPathMaxLen);

     dbContext.context.ldbid   = ldbid;
     dbContext.context.seat_no = seat_no;
     dbContext.context.user_no = user_no;

     // get database context: database path and database key
     rval = get_db_context(&dbContext, resource_id, ResIsNoFile, dbKey, dbPath);
     if(rval >= 0)
     {
        if(   dbContext.configKey.storage < PersistenceStoragePolicy_LastEntry
           && dbContext.configKey.storage >= PersistenceStorage_local)   // check if store policy is valid
        {
           rval = pers_db_delete_key(dbPath, dbKey, &dbContext);
        }
        else
        {
          rval = EPERS_BADPOL;
        }
     }
   }
   else
   {
      rval = EPERS_LOCKFS;
   }

   return rval;
}



// status: OK
int key_get_size(unsigned int ldbid, const char* resource_id, unsigned int user_no, unsigned int seat_no)
{
   int data_size = 0;
   PersistenceInfo_s dbContext;

   char dbKey[DbKeyMaxLen];      // database key
   char dbPath[DbPathMaxLen];    // database location

   memset(dbKey, 0, DbKeyMaxLen);
   memset(dbPath, 0, DbPathMaxLen);

   dbContext.context.ldbid   = ldbid;
   dbContext.context.seat_no = seat_no;
   dbContext.context.user_no = user_no;

   // get database context: database path and database key
   data_size = get_db_context(&dbContext, resource_id, ResIsNoFile, dbKey, dbPath);
   if(data_size >= 0)
   {
      if(   dbContext.configKey.storage < PersistenceStoragePolicy_LastEntry
         && dbContext.configKey.storage >= PersistenceStorage_local)   // check if store policy is valid
      {
         data_size = pers_db_get_key_size(dbPath, dbKey, &dbContext);
      }
      else
      {
        data_size = EPERS_BADPOL;
      }
   }
   else
   {
     data_size = EPERS_BADPOL;
   }

   return data_size;
}



// status: OK
int key_read_data(unsigned int ldbid, const char* resource_id, unsigned int user_no, unsigned int seat_no,
                  unsigned char* buffer, int buffer_size)
{
   int data_size = 0;

   if(AccessNoLock != isAccessLocked() ) // check if access to persistent data is locked
   {
      PersistenceInfo_s dbContext;

      char dbKey[DbKeyMaxLen];      // database key
      char dbPath[DbPathMaxLen];    // database location

      memset(dbKey, 0, DbKeyMaxLen);
      memset(dbPath, 0, DbPathMaxLen);

      dbContext.context.ldbid   = ldbid;
      dbContext.context.seat_no = seat_no;
      dbContext.context.user_no = user_no;

      // get database context: database path and database key
      data_size = get_db_context(&dbContext, resource_id, ResIsNoFile, dbKey, dbPath);
      if(data_size >= 0)
      {

         if(   dbContext.configKey.storage <  PersistenceStoragePolicy_LastEntry
            && dbContext.configKey.storage >= PersistenceStorage_local)   // check if store policy is valid
         {
            data_size = pers_db_read_key(dbPath, dbKey, &dbContext, buffer, buffer_size);
         }
         else
         {
            data_size = EPERS_BADPOL;
         }
      }
   }
   else
   {
      data_size = EPERS_LOCKFS;
   }

   return data_size;
}



int key_write_data(unsigned int ldbid, const char* resource_id, unsigned int user_no, unsigned int seat_no,
                   unsigned char* buffer, int buffer_size)
{
   int data_size = 0;

   if(AccessNoLock != isAccessLocked() )     // check if access to persistent data is locked
   {
      if(buffer_size <= gMaxKeyValDataSize)  // check data size
      {
         PersistenceInfo_s dbContext;

         unsigned int hash_val_data = 0;

         char dbKey[DbKeyMaxLen];      // database key
         char dbPath[DbPathMaxLen];    // database location

         memset(dbKey, 0, DbKeyMaxLen);
         memset(dbPath, 0, DbPathMaxLen);

         dbContext.context.ldbid   = ldbid;
         dbContext.context.seat_no = seat_no;
         dbContext.context.user_no = user_no;

         // get database context: database path and database key
         data_size = get_db_context(&dbContext, resource_id, ResIsNoFile, dbKey, dbPath);
         if(data_size >= 0)
         {
            // get hash value of data to verify storing
            hash_val_data = crc32(hash_val_data, buffer, buffer_size);

            // store data
            if(   dbContext.configKey.storage <  PersistenceStoragePolicy_LastEntry
               && dbContext.configKey.storage >= PersistenceStorage_local)   // check if store policy is valid
            {
               data_size = pers_db_write_key(dbPath, dbKey, &dbContext, buffer, buffer_size);
            }
            else
            {
               data_size = EPERS_BADPOL;
            }
         }
      }
      else
      {
         data_size = EPERS_BUFLIMIT;
         printf("key_write_data: error - buffer_size to big, limit is [%d] bytes\n", gMaxKeyValDataSize);
      }
   }
   else
   {
      data_size = EPERS_LOCKFS;
   }

   return data_size;
}



// status: TODO implement register on change
int key_register_notify_on_change(unsigned int ldbid, const char* resource_id, unsigned int user_no, unsigned int seat_no)
{
   int rval = 0;
   PersistenceInfo_s dbContext;

   //   unsigned int hash_val_data = 0;
   char dbKey[DbKeyMaxLen];      // database key
   char dbPath[DbPathMaxLen];    // database location

   memset(dbKey, 0, DbKeyMaxLen);
   memset(dbPath, 0, DbPathMaxLen);

   dbContext.context.ldbid   = ldbid;
   dbContext.context.seat_no = seat_no;
   dbContext.context.user_no = user_no;

   // get database context: database path and database key
   rval = get_db_context(&dbContext, resource_id, ResIsNoFile, dbKey, dbPath);

   // registration is only on shared key possible
   if(PersistenceStorage_shared == rval)
   {
      rval = persistence_reg_notify_on_change(dbPath, dbKey);
   }

   return rval;
}








