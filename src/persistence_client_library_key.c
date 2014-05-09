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
#include "persistence_client_library_handle.h"
#include "persistence_client_library_pas_interface.h"
#include "persistence_client_library_custom_loader.h"
#include "persistence_client_library_prct_access.h"
#include "persistence_client_library_db_access.h"
#include "crc32.h"

#include <persComRct.h>



// function declaration
int handleRegNotifyOnChange(int key_handle, pclChangeNotifyCallback_t callback, PersNotifyRegPolicy_e regPolicy);
int regNotifyOnChange(unsigned int ldbid, const char* resource_id, unsigned int user_no, unsigned int seat_no,
                      pclChangeNotifyCallback_t callback, PersNotifyRegPolicy_e regPolicy);

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// function with handle
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

int pclKeyHandleOpen(unsigned int ldbid, const char* resource_id, unsigned int user_no, unsigned int seat_no)
{
   int handle = EPERS_NOT_INITIALIZED;

   if(gPclInitialized >= PCLinitialized)
   {
      PersistenceInfo_s dbContext;

      char dbKey[DbKeyMaxLen]   = {0};      // database key
      char dbPath[DbPathMaxLen] = {0};    // database location

      dbContext.context.ldbid   = ldbid;
      dbContext.context.seat_no = seat_no;
      dbContext.context.user_no = user_no;

      // get database context: database path and database key
      handle = get_db_context(&dbContext, resource_id, ResIsNoFile, dbKey, dbPath);
      if(   (handle >= 0) && (dbContext.configKey.type == PersistenceResourceType_key) )          // check if type matches
      {
         if(dbContext.configKey.storage < PersistenceStorage_LastEntry)    // check if store policy is valid
         {
            // generate handle for custom and for normal key
            handle = get_persistence_handle_idx();

            if((handle < MaxPersHandle) && (0 < handle))
            {
               // remember data in handle array
               strncpy(gKeyHandleArray[handle].dbPath, dbPath, DbPathMaxLen);
               strncpy(gKeyHandleArray[handle].dbKey,  dbKey,  DbKeyMaxLen);
               strncpy(gKeyHandleArray[handle].resourceID,  resource_id,  DbResIDMaxLen);
               gKeyHandleArray[handle].dbPath[DbPathMaxLen-1] = '\0'; // Ensures 0-Termination
               gKeyHandleArray[handle].dbKey[ DbKeyMaxLen-1] = '\0'; // Ensures 0-Termination
               gKeyHandleArray[handle].info = dbContext;
            }
            else
            {
               DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("pclKeyHandleOpen: error - handleId out of bounds:"), DLT_INT(handle));
            }
         }
         else
         {
            handle = EPERS_BADPOL;
         }
      }
      else
      {
         DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("pclKeyHandleOpen: error - no database context or resource is not a key "));
      }
   }

   return handle;
}



int pclKeyHandleClose(int key_handle)
{
   int rval = EPERS_NOT_INITIALIZED;

   if(gPclInitialized >= PCLinitialized)
   {
      if((key_handle < MaxPersHandle) && (key_handle > 0))
      {
         if ('\0' != gKeyHandleArray[key_handle].resourceID[0])
         {
            /* Invalidate key handle data */
        	set_persistence_handle_close_idx(key_handle);
            memset(&gKeyHandleArray[key_handle], 0, sizeof(gKeyHandleArray[key_handle]));
            rval = 1;
         }
         else
         {
            rval = EPERS_INVALID_HANDLE;
         }
      }
      else
      {
         rval = EPERS_MAXHANDLE;
      }
   }

   return rval;
}



int pclKeyHandleGetSize(int key_handle)
{
   int size = EPERS_NOT_INITIALIZED;

   if(gPclInitialized >= PCLinitialized)
   {
      if((key_handle < MaxPersHandle) && (key_handle > 0))
      {
         if ('\0' != gKeyHandleArray[key_handle].resourceID[0])
         {
        	 size = pclKeyGetSize(gKeyHandleArray[key_handle].info.context.ldbid,
                                  gKeyHandleArray[key_handle].resourceID,
                                  gKeyHandleArray[key_handle].info.context.user_no,
                                  gKeyHandleArray[key_handle].info.context.seat_no);
         }
         else
         {
        	 size = EPERS_INVALID_HANDLE;
         }
      }
      else
      {
         size = EPERS_MAXHANDLE;
      }
   }

   return size;
}



int pclKeyHandleReadData(int key_handle, unsigned char* buffer, int buffer_size)
{
   int size = EPERS_NOT_INITIALIZED;

   if(gPclInitialized >= PCLinitialized)
   {
      if((key_handle < MaxPersHandle) && (key_handle > 0))
      {
         if ('\0' != gKeyHandleArray[key_handle].resourceID[0])
         {
        	 size = pclKeyReadData(gKeyHandleArray[key_handle].info.context.ldbid,
                                   gKeyHandleArray[key_handle].resourceID,
                                   gKeyHandleArray[key_handle].info.context.user_no,
                                   gKeyHandleArray[key_handle].info.context.seat_no,
                                   buffer,
                                   buffer_size);
         }
         else
         {
        	 size = EPERS_INVALID_HANDLE;
         }
      }
      else
      {
         size = EPERS_MAXHANDLE;
      }
   }

   return size;
}



int pclKeyHandleRegisterNotifyOnChange(int key_handle, pclChangeNotifyCallback_t callback)
{
   return handleRegNotifyOnChange(key_handle, callback, Notify_register);
}

int pclKeyHandleUnRegisterNotifyOnChange(int key_handle, pclChangeNotifyCallback_t callback)
{
   return handleRegNotifyOnChange(key_handle, callback, Notify_unregister);
}



int handleRegNotifyOnChange(int key_handle, pclChangeNotifyCallback_t callback, PersNotifyRegPolicy_e regPolicy)
{
   int rval = EPERS_NOT_INITIALIZED;

   if(gPclInitialized >= PCLinitialized)
   {
      if((key_handle < MaxPersHandle) && (key_handle > 0))
      {
         if ('\0' != gKeyHandleArray[key_handle].resourceID[0])
         {
            rval = regNotifyOnChange(gKeyHandleArray[key_handle].info.context.ldbid,
                                     gKeyHandleArray[key_handle].resourceID,
                                     gKeyHandleArray[key_handle].info.context.user_no,
                                     gKeyHandleArray[key_handle].info.context.seat_no,
                                     callback,
                                     regPolicy);
         }
          else
          {
         	 rval = EPERS_INVALID_HANDLE;
          }
      }
      else
      {
         rval = EPERS_MAXHANDLE;
      }
   }
   return rval;
}



int pclKeyHandleWriteData(int key_handle, unsigned char* buffer, int buffer_size)
{
   int size = EPERS_NOT_INITIALIZED;

   if(gPclInitialized >= PCLinitialized)
   {
      if((key_handle < MaxPersHandle) && (key_handle > 0))
      {
         if ('\0' != gKeyHandleArray[key_handle].resourceID[0])
         {
        	 size = pclKeyWriteData(gKeyHandleArray[key_handle].info.context.ldbid,
                                    gKeyHandleArray[key_handle].resourceID,
                                    gKeyHandleArray[key_handle].info.context.user_no,
                                    gKeyHandleArray[key_handle].info.context.seat_no,
                                    buffer,
                                    buffer_size);
         }
         else
         {
        	 size = EPERS_INVALID_HANDLE;
         }
      }
      else
      {
         size = EPERS_MAXHANDLE;
      }
   }

   return size;
}





// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// functions to be used directly without a handle
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

int pclKeyDelete(unsigned int ldbid, const char* resource_id, unsigned int user_no, unsigned int seat_no)
{
   int rval = EPERS_NOT_INITIALIZED;

   if(gPclInitialized >= PCLinitialized)
   {
      if(AccessNoLock != isAccessLocked() ) // check if access to persistent data is locked
      {
         PersistenceInfo_s dbContext;

        char dbKey[DbKeyMaxLen]   = {0};      // database key
        char dbPath[DbPathMaxLen] = {0};    // database location

        dbContext.context.ldbid   = ldbid;
        dbContext.context.seat_no = seat_no;
        dbContext.context.user_no = user_no;

        // get database context: database path and database key
        rval = get_db_context(&dbContext, resource_id, ResIsNoFile, dbKey, dbPath);
        if(   (rval >= 0)
           && (dbContext.configKey.type == PersistenceResourceType_key) )  // check if type is matching
        {
           if(   dbContext.configKey.storage < PersistenceStorage_LastEntry)   // check if store policy is valid
           {
	           rval = persistence_delete_data(dbPath, dbKey, &dbContext);
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
   }

   return rval;
}



// status: OK
int pclKeyGetSize(unsigned int ldbid, const char* resource_id, unsigned int user_no, unsigned int seat_no)
{
   int data_size = EPERS_NOT_INITIALIZED;

   if(gPclInitialized >= PCLinitialized)
   {
      PersistenceInfo_s dbContext;

      char dbKey[DbKeyMaxLen]   = {0};      // database key
      char dbPath[DbPathMaxLen] = {0};    // database location

      dbContext.context.ldbid   = ldbid;
      dbContext.context.seat_no = seat_no;
      dbContext.context.user_no = user_no;

      // get database context: database path and database key
      data_size = get_db_context(&dbContext, resource_id, ResIsNoFile, dbKey, dbPath);
      if(   (data_size >= 0)
         && (dbContext.configKey.type == PersistenceResourceType_key) )    // check if type matches
      {
         if(   dbContext.configKey.storage < PersistenceStorage_LastEntry)   // check if store policy is valid
         {
            data_size = persistence_get_data_size(dbPath, dbKey, &dbContext);
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
   }

   return data_size;
}



// status: OK
int pclKeyReadData(unsigned int ldbid, const char* resource_id, unsigned int user_no, unsigned int seat_no,
                  unsigned char* buffer, int buffer_size)
{
   int data_size = EPERS_NOT_INITIALIZED;

   if(gPclInitialized >= PCLinitialized)
   {
      if(AccessNoLock != isAccessLocked() ) // check if access to persistent data is locked
      {
         PersistenceInfo_s dbContext;

         char dbKey[DbKeyMaxLen]   = {0};      // database key
         char dbPath[DbPathMaxLen] = {0};    // database location

         dbContext.context.ldbid   = ldbid;
         dbContext.context.seat_no = seat_no;
         dbContext.context.user_no = user_no;

         // get database context: database path and database key
         data_size = get_db_context(&dbContext, resource_id, ResIsNoFile, dbKey, dbPath);
         if(   (data_size >= 0)
            && (dbContext.configKey.type == PersistenceResourceType_key) )
         {

            if(   dbContext.configKey.storage < PersistenceStorage_LastEntry)   // check if store policy is valid
            {
                  data_size = persistence_get_data(dbPath, dbKey, &dbContext, buffer, buffer_size);
            }
            else
            {
               data_size = EPERS_BADPOL;
            }
         }
         else
         {
            DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("pclKeyReadData - error - no database context or resource is not a key"));
         }
      }
      else
      {
         data_size = EPERS_LOCKFS;
      }
   }

   return data_size;
}



int pclKeyWriteData(unsigned int ldbid, const char* resource_id, unsigned int user_no, unsigned int seat_no,
                   unsigned char* buffer, int buffer_size)
{
   int data_size = EPERS_NOT_INITIALIZED;

   if(gPclInitialized >= PCLinitialized)
   {
      if(AccessNoLock != isAccessLocked() )     // check if access to persistent data is locked
      {
         if(buffer_size <= gMaxKeyValDataSize)  // check data size
         {
            PersistenceInfo_s dbContext;

            unsigned int hash_val_data = 0;

            char dbKey[DbKeyMaxLen]   = {0};      // database key
            char dbPath[DbPathMaxLen] = {0};    // database location

            dbContext.context.ldbid   = ldbid;
            dbContext.context.seat_no = seat_no;
            dbContext.context.user_no = user_no;

            // get database context: database path and database key
            data_size = get_db_context(&dbContext, resource_id, ResIsNoFile, dbKey, dbPath);
            if(   (data_size >= 0)
               && (dbContext.configKey.type == PersistenceResourceType_key))
            {
               if(dbContext.configKey.permission != PersistencePermission_ReadOnly)  // don't write to a read only resource
               {
                  // get hash value of data to verify storing
                  hash_val_data = pclCrc32(hash_val_data, buffer, buffer_size);

                  // store data
                  if(   dbContext.configKey.storage < PersistenceStorage_LastEntry)   // check if store policy is valid
                  {
                     data_size = persistence_set_data(dbPath, dbKey, &dbContext, buffer, buffer_size);
                  }
                  else
                  {
                     data_size = EPERS_BADPOL;
                  }
               }
               else
               {
                  data_size = EPERS_RESOURCE_READ_ONLY;
               }
            }
            else
            {
               DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("pclKeyWriteData - error - no database context or resource is not a key"));
            }
         }
         else
         {
            data_size = EPERS_BUFLIMIT;
            DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("pclKeyWriteData: error - buffer_size to big, limit is [bytes]:"), DLT_INT(gMaxKeyValDataSize));
         }
      }
      else
      {
         data_size = EPERS_LOCKFS;
      }
   }
   return data_size;
}



int pclKeyUnRegisterNotifyOnChange( unsigned int  ldbid, const char *  resource_id, unsigned int  user_no, unsigned int  seat_no, pclChangeNotifyCallback_t  callback)
{
   return regNotifyOnChange(ldbid, resource_id, user_no, seat_no, callback, Notify_unregister);
}


int pclKeyRegisterNotifyOnChange(unsigned int ldbid, const char* resource_id, unsigned int user_no, unsigned int seat_no, pclChangeNotifyCallback_t callback)
{
   return regNotifyOnChange(ldbid, resource_id, user_no, seat_no, callback, Notify_register);
}




int regNotifyOnChange(unsigned int ldbid, const char* resource_id, unsigned int user_no, unsigned int seat_no, pclChangeNotifyCallback_t callback, PersNotifyRegPolicy_e regPolicy)
{
   int rval = EPERS_NOT_INITIALIZED;

   if(gPclInitialized >= PCLinitialized)
   {
      PersistenceInfo_s dbContext;

      //   unsigned int hash_val_data = 0;
      char dbKey[DbKeyMaxLen]   = {0};      // database key
      char dbPath[DbPathMaxLen] = {0};    // database location

      dbContext.context.ldbid   = ldbid;
      dbContext.context.seat_no = seat_no;
      dbContext.context.user_no = user_no;

      // get database context: database path and database key
      rval = get_db_context(&dbContext, resource_id, ResIsNoFile, dbKey, dbPath);

      if (rval==0)  // no error, key found
	  { 
         // registration is only on shared and custom keys possible
         if(   (dbContext.configKey.storage != PersistenceStorage_local)
            && (dbContext.configKey.type    == PersistenceResourceType_key) )
         {
            rval = persistence_notify_on_change(dbKey, ldbid, user_no, seat_no, callback, regPolicy);
         }
         else
         {
            DLT_LOG(gPclDLTContext, DLT_LOG_ERROR,
            		             DLT_STRING("regNotifyOnChange: Not allowed! Resource is local or it is a file:"),
            		             DLT_STRING(resource_id));
            rval = EPERS_NOTIFY_NOT_ALLOWED;
         }
      }
      else
      {
         DLT_LOG(gPclDLTContext, DLT_LOG_ERROR,
                              DLT_STRING("regNotifyOnChange: Not possible! get_db_context() returned:"),
                              DLT_INT(rval));
      }
   }

   return rval;
}







