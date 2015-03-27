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
#include "persistence_client_library_prct_access.h"
#include "persistence_client_library_db_access.h"


/// max key value data size [default 16kB]
static int gMaxKeyValDataSize = PERS_DB_MAX_SIZE_KEY_DATA;


// function declaration
static int handleRegNotifyOnChange(int key_handle, pclChangeNotifyCallback_t callback, PersNotifyRegPolicy_e regPolicy);
static int regNotifyOnChange(unsigned int ldbid, const char* resource_id, unsigned int user_no, unsigned int seat_no,
                      pclChangeNotifyCallback_t callback, PersNotifyRegPolicy_e regPolicy);

#if USE_APPCHECK
extern int doAppcheck(void);
#endif

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// function with handle
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

int pclKeyHandleOpen(unsigned int ldbid, const char* resource_id, unsigned int user_no, unsigned int seat_no)
{
   int rval   = 0, handle = EPERS_NOT_INITIALIZED;

   if(__sync_add_and_fetch(&gPclInitCounter, 0) > 0)
   {
#if USE_APPCHECK
      if(doAppcheck() == 1)
      {
#endif
         PersistenceInfo_s dbContext;

         char dbKey[PERS_DB_MAX_LENGTH_KEY_NAME]   = {0};    // database key
         char dbPath[PERS_ORG_MAX_LENGTH_PATH_FILENAME] = {0};    // database location

         dbContext.context.ldbid   = ldbid;
         dbContext.context.seat_no = seat_no;
         dbContext.context.user_no = user_no;

         // get database context: database path and database key
         rval = get_db_context(&dbContext, resource_id, ResIsNoFile, dbKey, dbPath);
         if((rval >= 0) && (dbContext.configKey.type == PersistenceResourceType_key))          // check if type matches
         {
            if(dbContext.configKey.storage < PersistenceStorage_LastEntry)    // check if store policy is valid
            {
               // remember data in handle array
               handle = set_key_handle_data(get_persistence_handle_idx(), resource_id, ldbid, user_no, seat_no);
            }
            else
            {
               handle = EPERS_BADPOL;
            }
         }
         else
         {
            DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("keyHandleOpen - no db context or res not a key "));
         }
#if USE_APPCHECK
      }
      else
      {
         handle = EPERS_SHUTDOWN_NO_TRUSTED;
      }
#endif
   }

   return handle;
}



int pclKeyHandleClose(int key_handle)
{
   int rval = EPERS_NOT_INITIALIZED;

   if(__sync_add_and_fetch(&gPclInitCounter, 0) > 0)
   {
#if USE_APPCHECK
      if(doAppcheck() == 1)
      {
#endif
         PersistenceKeyHandle_s persHandle;

         if(get_key_handle_data(key_handle, &persHandle) != -1)
         {
            if ('\0' != persHandle.resource_id[0])
            {
               /* Invalidate key handle data */
               set_persistence_handle_close_idx(key_handle);
               clear_key_handle_array(key_handle);
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
#if USE_APPCHECK
      }
      else
      {
         rval = EPERS_SHUTDOWN_NO_TRUSTED;
      }
#endif
   }

   return rval;
}



int pclKeyHandleGetSize(int key_handle)
{
   int size = EPERS_NOT_INITIALIZED;

   if(__sync_add_and_fetch(&gPclInitCounter, 0) > 0)
   {
#if USE_APPCHECK
      if(doAppcheck() == 1)
      {
#endif
         PersistenceKeyHandle_s persHandle;

         if(get_key_handle_data(key_handle, &persHandle) != -1)
         {
            if ('\0' != persHandle.resource_id[0])
            {
             size = pclKeyGetSize(persHandle.ldbid, persHandle.resource_id,
                                  persHandle.user_no, persHandle.seat_no);
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
#if USE_APPCHECK
      }
      else
      {
         size = EPERS_SHUTDOWN_NO_TRUSTED;
      }
#endif
   }

   return size;
}



int pclKeyHandleReadData(int key_handle, unsigned char* buffer, int buffer_size)
{
   int size = EPERS_NOT_INITIALIZED;

   if(__sync_add_and_fetch(&gPclInitCounter, 0) > 0)
   {
#if USE_APPCHECK
      if(doAppcheck() == 1)
      {
#endif
         PersistenceKeyHandle_s persHandle;

         if(get_key_handle_data(key_handle, &persHandle) != -1)
         {
            if ('\0' != persHandle.resource_id[0])
            {
             size = pclKeyReadData(persHandle.ldbid, persHandle.resource_id,
                                   persHandle.user_no, persHandle.seat_no,
                                   buffer, buffer_size);
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
#if USE_APPCHECK
      }
      else
      {
         size = EPERS_SHUTDOWN_NO_TRUSTED;
      }
#endif
   }

   return size;
}



int pclKeyHandleRegisterNotifyOnChange(int key_handle, pclChangeNotifyCallback_t callback)
{
   int rval = EPERS_COMMON;
   //DLT_LOG(gDLTContext, DLT_LOG_DEBUG, DLT_STRING("pclKeyHandleRegisterNotifyOnChange: "),
   //            DLT_INT(gKeyHandleArray[key_handle].info.context.ldbid), DLT_STRING(gKeyHandleArray[key_handle].resourceID) );
   if((gChangeNotifyCallback == callback) || (gChangeNotifyCallback == NULL))
   {
      rval = handleRegNotifyOnChange(key_handle, callback, Notify_register);
   }
   else
   {
      DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("keyHandleRegNotOnChange - Only one cBack allowed for ch notiy."));
      rval = EPERS_NOTIFY_NOT_ALLOWED;
   }
   return rval;
}

int pclKeyHandleUnRegisterNotifyOnChange(int key_handle, pclChangeNotifyCallback_t callback)
{
   return handleRegNotifyOnChange(key_handle, callback, Notify_unregister);
}



int handleRegNotifyOnChange(int key_handle, pclChangeNotifyCallback_t callback, PersNotifyRegPolicy_e regPolicy)
{
   int rval = EPERS_NOT_INITIALIZED;

   if(__sync_add_and_fetch(&gPclInitCounter, 0) > 0)
   {
   	PersistenceKeyHandle_s persHandle;

      if(get_key_handle_data(key_handle, &persHandle) != -1)
      {
         if ('\0' != persHandle.resource_id[0])
         {
            rval = regNotifyOnChange(persHandle.ldbid,   persHandle.resource_id,
            		                   persHandle.user_no, persHandle.seat_no,
                                     callback, regPolicy);
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

   if(__sync_add_and_fetch(&gPclInitCounter, 0) > 0)
   {
#if USE_APPCHECK
      if(doAppcheck() == 1)
      {
#endif
         PersistenceKeyHandle_s persHandle;

         if(get_key_handle_data(key_handle, &persHandle) != -1)
         {
            if ('\0' != persHandle.resource_id[0])
            {
             size = pclKeyWriteData(persHandle.ldbid,   persHandle.resource_id,
                                    persHandle.user_no, persHandle.seat_no, buffer, buffer_size);
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
#if USE_APPCHECK
      }
      else
      {
         size = EPERS_SHUTDOWN_NO_TRUSTED;
      }
#endif
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

   if(__sync_add_and_fetch(&gPclInitCounter, 0) > 0)
   {
#if USE_APPCHECK
      if(doAppcheck() == 1)
      {
#endif
         if(AccessNoLock != isAccessLocked() ) // check if access to persistent data is locked
         {
            PersistenceInfo_s dbContext;

           char dbKey[PERS_DB_MAX_LENGTH_KEY_NAME]   = {0};     // database key
           char dbPath[PERS_ORG_MAX_LENGTH_PATH_FILENAME] = {0};     // database location

           dbContext.context.ldbid   = ldbid;
           dbContext.context.seat_no = seat_no;
           dbContext.context.user_no = user_no;

           // get database context: database path and database key
           rval = get_db_context(&dbContext, resource_id, ResIsNoFile, dbKey, dbPath);
           if(   (rval >= 0)
              && (dbContext.configKey.type == PersistenceResourceType_key) )     // check if type is matching
           {
              if(   dbContext.configKey.storage < PersistenceStorage_LastEntry)  // check if store policy is valid
              {
                 rval = persistence_delete_data(dbPath, dbKey, resource_id, &dbContext);
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
#if USE_APPCHECK
      }
      else
      {
         rval = EPERS_SHUTDOWN_NO_TRUSTED;
      }
#endif
   }

   return rval;
}



int pclKeyGetSize(unsigned int ldbid, const char* resource_id, unsigned int user_no, unsigned int seat_no)
{
   int data_size = EPERS_NOT_INITIALIZED;

   if(__sync_add_and_fetch(&gPclInitCounter, 0) > 0)
   {
#if USE_APPCHECK
      if(doAppcheck() == 1)
      {
#endif
         PersistenceInfo_s dbContext;

         char dbKey[PERS_DB_MAX_LENGTH_KEY_NAME]   = {0};       // database key
         char dbPath[PERS_ORG_MAX_LENGTH_PATH_FILENAME] = {0};       // database location

         dbContext.context.ldbid   = ldbid;
         dbContext.context.seat_no = seat_no;
         dbContext.context.user_no = user_no;

         // get database context: database path and database key
         data_size = get_db_context(&dbContext, resource_id, ResIsNoFile, dbKey, dbPath);
         if(   (data_size >= 0)
            && (dbContext.configKey.type == PersistenceResourceType_key) )       // check if type matches
         {
            if(   dbContext.configKey.storage < PersistenceStorage_LastEntry)    // check if store policy is valid
            {
               data_size = persistence_get_data_size(dbPath, dbKey, resource_id, &dbContext);
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
#if USE_APPCHECK
      }
      else
      {
         data_size = EPERS_SHUTDOWN_NO_TRUSTED;
      }
#endif
   }

   return data_size;
}



int pclKeyReadData(unsigned int ldbid, const char* resource_id, unsigned int user_no, unsigned int seat_no,
                  unsigned char* buffer, int buffer_size)
{
   int data_size = EPERS_NOT_INITIALIZED;

   if(__sync_add_and_fetch(&gPclInitCounter, 0) > 0)
   {
#if USE_APPCHECK
      if(doAppcheck() == 1)
      {
#endif
         if(AccessNoLock != isAccessLocked() ) // check if access to persistent data is locked
         {
            PersistenceInfo_s dbContext;

            char dbKey[PERS_DB_MAX_LENGTH_KEY_NAME]   = {0};       // database key
            char dbPath[PERS_ORG_MAX_LENGTH_PATH_FILENAME] = {0};       // database location

            dbContext.context.ldbid   = ldbid;
            dbContext.context.seat_no = seat_no;
            dbContext.context.user_no = user_no;

            // get database context: database path and database key
            data_size = get_db_context(&dbContext, resource_id, ResIsNoFile, dbKey, dbPath);
            if(   (data_size >= 0)
               && (dbContext.configKey.type == PersistenceResourceType_key) )
            {

               if(dbContext.configKey.storage < PersistenceStorage_LastEntry)   // check if store policy is valid
               {
                     data_size = persistence_get_data(dbPath, dbKey, resource_id, &dbContext, buffer, buffer_size);
               }
               else
               {
                  data_size = EPERS_BADPOL;
               }
            }
            else
            {
               DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("keyReadData - no db context or res not a key"));
            }
         }
         else
         {
            data_size = EPERS_LOCKFS;
         }
#if USE_APPCHECK
      }
      else
      {
         data_size = EPERS_SHUTDOWN_NO_TRUSTED;
      }
#endif
   }

   return data_size;
}



int pclKeyWriteData(unsigned int ldbid, const char* resource_id, unsigned int user_no, unsigned int seat_no,
                   unsigned char* buffer, int buffer_size)
{
   int data_size = EPERS_NOT_INITIALIZED;

   if(__sync_add_and_fetch(&gPclInitCounter, 0) > 0)
   {
#if USE_APPCHECK
      if(doAppcheck() == 1)
      {
#endif
         if(AccessNoLock != isAccessLocked() )     // check if access to persistent data is locked
         {
            if(buffer_size <= gMaxKeyValDataSize)  // check data size
            {
               PersistenceInfo_s dbContext;

               char dbKey[PERS_DB_MAX_LENGTH_KEY_NAME]   = {0};       // database key
               char dbPath[PERS_ORG_MAX_LENGTH_PATH_FILENAME] = {0};       // database location

               dbContext.context.ldbid   = ldbid;
               dbContext.context.seat_no = seat_no;
               dbContext.context.user_no = user_no;

               // get database context: database path and database key
               data_size = get_db_context(&dbContext, resource_id, ResIsNoFile, dbKey, dbPath);
               if(   (data_size >= 0)
                  && (dbContext.configKey.type == PersistenceResourceType_key))
               {
                  if(dbContext.configKey.permission != PersistencePermission_ReadOnly)    // don't write to a read only resource
                  {
                     // store data
                     if(dbContext.configKey.storage < PersistenceStorage_LastEntry)       // check if store policy is valid
                     {
                        if(   (dbContext.configKey.storage == PersistenceStorage_shared)
                           && (0 != strncmp(dbContext.configKey.reponsible, gAppId, PERS_RCT_MAX_LENGTH_RESPONSIBLE) ) )
                        {
                           data_size = EPERS_NOT_RESP_APP;
                        }
                        else
                        {
                           data_size = persistence_set_data(dbPath, dbKey, resource_id, &dbContext, buffer, buffer_size);
                        }
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
                  DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("keyWriteData no db context or res is not a key"));
               }
            }
            else
            {
               data_size = EPERS_BUFLIMIT;
               DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("keyWriteData - buffer_size to big, limit is [bytes]:"), DLT_INT(gMaxKeyValDataSize));
            }
         }
         else
         {
            data_size = EPERS_LOCKFS;
         }
#if USE_APPCHECK
      }
      else
      {
         data_size = EPERS_SHUTDOWN_NO_TRUSTED;
      }
#endif
   }
   return data_size;
}



int pclKeyUnRegisterNotifyOnChange( unsigned int  ldbid, const char *  resource_id, unsigned int  user_no, unsigned int  seat_no, pclChangeNotifyCallback_t  callback)
{
   return regNotifyOnChange(ldbid, resource_id, user_no, seat_no, callback, Notify_unregister);
}



int pclKeyRegisterNotifyOnChange(unsigned int ldbid, const char* resource_id, unsigned int user_no, unsigned int seat_no, pclChangeNotifyCallback_t callback)
{
   int rval = EPERS_COMMON;
   //DLT_LOG(gDLTContext, DLT_LOG_DEBUG, DLT_STRING("pclKeyRegisterNotifyOnChange: "),
   //            DLT_INT(ldbid), DLT_STRING(resource_id) );
   if((gChangeNotifyCallback == callback) || (gChangeNotifyCallback == NULL))
   {
      rval = regNotifyOnChange(ldbid, resource_id, user_no, seat_no, callback, Notify_register);
   }
   else
   {
      DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("keyRegNotifyOnChange - Only one cBack is allowed for ch noti."));
      rval = EPERS_NOTIFY_NOT_ALLOWED;
   }
   return rval;
}



int regNotifyOnChange(unsigned int ldbid, const char* resource_id, unsigned int user_no, unsigned int seat_no, pclChangeNotifyCallback_t callback, PersNotifyRegPolicy_e regPolicy)
{
   int rval = EPERS_NOT_INITIALIZED;

   if(__sync_add_and_fetch(&gPclInitCounter, 0) > 0)
   {
#if USE_APPCHECK
      if(doAppcheck() == 1)
      {
#endif
         PersistenceInfo_s dbContext;

         //   unsigned int hash_val_data = 0;
         char dbKey[PERS_DB_MAX_LENGTH_KEY_NAME]   = {0};      // database key
         char dbPath[PERS_ORG_MAX_LENGTH_PATH_FILENAME] = {0};    // database location

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
               rval = persistence_notify_on_change(resource_id, dbKey, ldbid, user_no, seat_no, callback, regPolicy);
            }
            else
            {
               DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("regNotifyOnChange - Not allowed! Res is local or it is a file:"),
                                       DLT_STRING(resource_id), DLT_STRING("LDBID:"), DLT_UINT(ldbid));
               rval = EPERS_NOTIFY_NOT_ALLOWED;
            }
         }
         else
         {
            DLT_LOG(gPclDLTContext, DLT_LOG_ERROR,
                                 DLT_STRING("regNotifyOnChange - Not possible! get_db_context() returned:"),
                                 DLT_INT(rval));
         }
#if USE_APPCHECK
      }
      else
      {
         rval = EPERS_SHUTDOWN_NO_TRUSTED;
      }
#endif
   }

   return rval;
}
