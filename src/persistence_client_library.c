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
   TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTIcacheON WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
******************************************************************************/
 /**
 * @file           persistence_client_library.c
 * @ingroup        Persistence client library
 * @author         Ingo Huerner
 * @brief          Implementation of the persistence client library.
 *                 Library provides an API to access persistent data
 * @see            
 */


#include "persistence_client_library.h"
#include "persistence_client_library_lc_interface.h"
#include "persistence_client_library_pas_interface.h"
#include "persistence_client_library_dbus_service.h"
#include "persistence_client_library_handle.h"

#include <string.h>
#include <errno.h>
#include <dlfcn.h>
#include <stdlib.h>

#include <dlt/dlt.h>
#include <dlt/dlt_common.h>


/// pointer to resource table database
static GvdbTable* gResource_table = NULL;

/// debug log and trace (DLT) setup
DLT_DECLARE_CONTEXT(persClientLibCtx);

/// library constructor
void pers_library_init(void) __attribute__((constructor));
/// library deconstructor
void pers_library_destroy(void) __attribute__((destructor));





void pers_library_init(void)
{
   DLT_REGISTER_APP("Persistence Client Library","persClientLib");
   DLT_REGISTER_CONTEXT(persClientLibCtx,"persClientLib","Context for Logging");

   DLT_LOG(persClientLibCtx, DLT_LOG_ERROR, DLT_STRING("Initialize Persistence Client Library!!!!"));

   setup_dbus_mainloop();

   // register for lifecycle and persistence admin service dbus messages
   register_lifecycle();
   register_pers_admin_service();

   // clear the open file descriptor array
   memset(gOpenFdArray, maxPersHandle, sizeof(int));

   printf("A p p l i c a t i o n   n a m e : %s \n", program_invocation_short_name);   // TODO: only temp solution for application name
   strncpy(gAppId, program_invocation_short_name, maxAppNameLen);
}



void pers_library_destroy(void)
{
   // dereference opend database
   if(gResource_table != NULL)
   {
      gvdb_table_unref(gResource_table);
   }

   // unregister for lifecycle and persistence admin service dbus messages
   unregister_lifecycle();
   unregister_pers_admin_service();


   DLT_UNREGISTER_CONTEXT(persClientLibCtx);
   DLT_UNREGISTER_APP();
   dlt_free();
}



// status: OK
int get_db_context(unsigned char ldbid, char* resource_id, unsigned char user_no, unsigned char seat_no,
                   unsigned int isFile, char dbKey[], char dbPath[])
{
   unsigned char cached = 0;
   GVariant* dbValue = NULL;
   // get resource configuration table
   GvdbTable* resource_table = get_resource_cfg_table();

   // check if resouce id is in write through table
   dbValue = gvdb_table_get_value(resource_table, resource_id);
   if(dbValue != NULL)
   {
      cached = storeWt; // it's a write through value
   }
   else
   {
      cached = storeCached; // must be a cached value
   }
   return get_db_path_and_key(ldbid, resource_id, user_no, seat_no, isFile, dbKey, dbPath, cached);
}



// status: OK
int get_db_path_and_key(unsigned char ldbid, char* resource_id, unsigned char user_no, unsigned char seat_no,
                        unsigned int isFile, char dbKey[], char dbPath[], unsigned char cached_resource)
{
   int rval = -1;

   //
   // create resource database key 
   //
   if((ldbid < 0x80) || (ldbid == 0xFF) )
   {
      // The LDBID is used to find the DBID in the resource table.
      if((user_no == 0) && (seat_no == 0))
      { 
         // Node is added in front of the resource ID as the key string.
         snprintf(dbKey, dbKeyMaxLen, "%s%s", gNode, resource_id);
         rval = 0;
      }
      else
      {
         if(seat_no == 0)
         {
            // /User/<user_no_parameter> is added in front of the resource ID as the key string.
            snprintf(dbKey, dbKeyMaxLen, "%s%d%s", gUser, user_no, resource_id);
            rval = 0;
         }
         else
         {
            // /User/<user_no_parameter>/Seat/<seat_no_parameter> is added in front of the resource ID as the key string.
            snprintf(dbKey, dbKeyMaxLen, "%s%d%s%d%s", gUser, user_no, gSeat, seat_no, resource_id);
            rval = 0;
         }
      }
   }
   
   if((ldbid >= 0x80) && ( ldbid != 0xFF))
   {
      // The LDBID is used to find the DBID in the resource table.
      // /<LDBID parameter> is added in front of the resource ID as the key string.
      //  Rational: Creates a namespace within one data base.
      //  Rational: Reduction of number of databases -> reduction of maintenance costs
      // /User/<user_no_parameter> and /Seat/<seat_no_parameter> are add after /<LDBID parameter> if there are different than 0.

      if(seat_no != 0)
      {
         snprintf(dbKey, dbKeyMaxLen, "/%x%s%d%s%d%s", ldbid, gUser, user_no, gSeat, seat_no, resource_id);
      }
      else
      {  
         snprintf(dbKey, dbKeyMaxLen, "/%x%s%d%s", ldbid, gUser, user_no, resource_id);
      }
      rval = 0;  
   }


   //
   // create resource database path
   //
   if(ldbid < 0x80)
   {
      // -------------------------------------
      // shared database
      // -------------------------------------

      if(ldbid != 0) 
      {
         // Additionally /GROUP/<LDBID_parameter> shall be added inside of the database path listed in the resource table. (Off target)
         // Rational: To ensure data separation using the Linux user right policy (different data base files in different locations).

         if(storeCached == cached_resource)
         {
            if(isFile == resIsNoFile)
               snprintf(dbPath, dbPathMaxLen, gSharedCachePath, ldbid, gSharedCached);
            else
               snprintf(dbPath, dbPathMaxLen, gSharedCachePath, ldbid, dbKey);
         }
         else if(storeWt == cached_resource)
         {
            if(isFile == resIsNoFile)
               snprintf(dbPath, dbPathMaxLen, gSharedWtPath, ldbid, gSharedWt);
            else
               snprintf(dbPath, dbPathMaxLen, gSharedWtPath, ldbid, dbKey);
         }
      }
      else
      {
         // Additionally /Shared/Public shall be added inside of the database path listed in the resource table. (Off target)

         if(storeCached == cached_resource)
         {
            if(isFile == resIsNoFile)
               snprintf(dbPath, dbPathMaxLen, gSharedPublicCachePath, gSharedCached);
            else
               snprintf(dbPath, dbPathMaxLen, gSharedPublicCachePath, dbKey);
         }
         else if(storeWt == cached_resource)
         {
            if(isFile == resIsNoFile)
               snprintf(dbPath, dbPathMaxLen, gSharedPublicWtPath, gSharedWt);
            else
               snprintf(dbPath, dbPathMaxLen, gSharedPublicWtPath, dbKey);
         }
      }

      rval = dbShared;   // we have a shared database
   }
   else
   {
      // -------------------------------------
      // local database
      // -------------------------------------

      if(storeCached == cached_resource)
      {
         if(isFile == resIsNoFile)
            snprintf(dbPath, dbPathMaxLen, gLocalCachePath, gAppId, gLocalCached);
         else
            snprintf(dbPath, dbPathMaxLen, gLocalCachePath, gAppId, dbKey);
      }
      else if(storeWt == cached_resource)
      {
         if(isFile == resIsNoFile)
            snprintf(dbPath, dbPathMaxLen, gLocalWtPath, gAppId, gLocalWt);
         else
            snprintf(dbPath, dbPathMaxLen, gLocalWtPath, gAppId, dbKey);
      }

      rval = dbLocal;   // we have a local database
   }

   printf("dbKey  : [key ]: %s \n",  dbKey);
   printf("dbPath : [path]: %s\n\n", dbPath);

   return rval;
}


// status: OK
GvdbTable* get_resource_cfg_table()
{
   if(gResource_table == NULL)   // check if database is already open
   {
      GError* error = NULL;
      char filename[dbPathMaxLen]; 
      snprintf(filename, dbPathMaxLen, gLocalWtPath, gAppId, gResTableCfg);
      gResource_table = gvdb_table_new(filename, TRUE, &error);

      if(gResource_table == NULL)   
      {
         printf("Database error: %s\n", error->message);
         g_error_free(error);
         error = NULL;
      }
   }

   return gResource_table;
}




