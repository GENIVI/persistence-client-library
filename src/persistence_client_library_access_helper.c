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
 * @file           persistence_client_library_access_helper.c
 * @ingroup        Persistence client library
 * @author         Ingo Huerner
 * @brief          Implementation of persistence access helper functions
 * @see
 */


#include "persistence_client_library_access_helper.h"
#include "gvdb-builder.h"

#include <stdlib.h>


/// pointer to resource table database
GvdbTable* gResource_table[] = { NULL, NULL, NULL, NULL };



PersistenceRCT_e get_table_id(int ldbid, int* groupId)
{
   PersistenceRCT_e rctType = PersistenceRCT_LastEntry;
   if(ldbid < 0x80)
   {
      // S H A R E D  database
      if(ldbid != 0)
      {
         // shared  G R O U P  database * * * * * * * * * * * * *  * * * * * *
         *groupId = ldbid;  // assign group ID
         rctType = PersistenceRCT_shared_group;
      }
      else
      {
         // shared  P U B L I C  database * * * * * * * * * * * * *  * * * * *
         *groupId = 0;      // no group ID for public data
         rctType = PersistenceRCT_shared_public;
      }
   }
   else
   {
      // L O C A L   database
      *groupId = 0;      // no group ID for local data
      rctType = PersistenceStorage_local;   // we have a local database
   }
   return rctType;
}


GvdbTable* get_resource_cfg_table_by_idx(int i)
{
   return gResource_table[i];
}


// status: OK
GvdbTable* get_resource_cfg_table(PersistenceRCT_e rct, int group)
{
   if(gResource_table[rct] == NULL)   // check if database is already open
   {
      GError* error = NULL;
      char filename[dbPathMaxLen];

      switch(rct)    // create db name
      {
      case PersistenceRCT_local:
         snprintf(filename, dbPathMaxLen, gLocalWtPath, gAppId, gResTableCfg);
         break;
      case PersistenceRCT_shared_public:
         snprintf(filename, dbPathMaxLen, gSharedPublicWtPath, gResTableCfg);
         break;
      case PersistenceRCT_shared_group:
         snprintf(filename, dbPathMaxLen, gSharedWtPath, group, gResTableCfg);
         break;
      default:
         printf("get_resource_cfg_table - error: no valid PersistenceRCT_e\n");
         break;
      }

      gResource_table[rct] = gvdb_table_new(filename, TRUE, &error);
      printf("get_resource_cfg_table - group %d | db filename: %s \n", group, filename);

      if(gResource_table[rct] == NULL)
      {
         printf("get_resource_cfg_table - Database error: %s\n", error->message);
         g_error_free(error);
         error = NULL;
      }
   }

   return gResource_table[rct];
}



int serialize_data(PersistenceConfigurationKey_s pc, char* buffer)
{
   int rval = 0;
   rval = snprintf(buffer, gMaxKeyValDataSize, "%d %d %u %d %s %s",
                                               pc.policy, pc.storage, pc.permission, pc.max_size,
                                               pc.reponsible, pc.custom_name);

   printf("serialize_data: %s \n", buffer);
   return rval;
}



int de_serialize_data(char* buffer, PersistenceConfigurationKey_s* pc)
{
   int rval = 1;
   char* token = NULL;
   if(buffer != NULL)
   {
      token = strtok(buffer, " ");     // policy
      if(token != 0)
      {
         pc->policy = strtol(token, 0, 10);
         //printf("pc->policy %d \n", pc->policy);
      }
      else
      {
         printf("de_serialize_data - error: can't get [policy] \n");
         rval = -1;
      }

      token = strtok (NULL, " ");      // storage
      if(token != 0)
      {
         pc->storage = strtol(token, 0, 10);
         //printf("pc->storage %d \n", pc->storage);
      }
      else
      {
         printf("de_serialize_data - error: can't get [storage] \n");
         rval = -1;
      }

      token = strtok (NULL, " ");      // permission
      if(token != 0)
      {
         pc->permission = strtol(token, 0, 10);
         //printf("pc->permission %d \n", pc->permission);
      }
      else
      {
         printf("de_serialize_data - error: can't get [permission] \n");
         rval = -1;
      }

      token = strtok (NULL, " ");      // max_size
      if(token != 0)
      {
         pc->max_size = strtol(token, 0, 10);
         //printf("pc->max_size %d \n", pc->max_size);
      }
      else
      {
         printf("de_serialize_data - error: can't get [max_size] \n");
         rval = -1;
      }

      token = strtok (NULL, " ");      // reponsible
      if(token != 0)
      {
         int size = strlen(token)+1;
         pc->reponsible = malloc(size);
         strncpy(pc->reponsible, token, size);
         //printf("     pc->reponsible %s | 0x%x \n", pc->reponsible, (int)pc->reponsible);
      }
      else
      {
         printf("de_serialize_data - error: can't get [reponsible] \n");
         rval = -1;
      }

      token = strtok (NULL, " ");      // custom_name
      if(token != 0)
      {
         int size = strlen(token)+1;
         pc->custom_name = malloc(size);
         strncpy(pc->custom_name, token, size);
         //printf("     pc->custom_name %s | 0x%x \n", pc->custom_name, (int)pc->custom_name);
      }
      else
      {
         char* na = "na";
         int size = strlen(na)+1;
         // custom name not available => no custom plugin
         pc->custom_name = malloc(size);
         strncpy(pc->custom_name, "na", size);
      }
   }
   else
   {
      printf("de_serialize_data - error: buffer is NULL\n");
      rval = -1;
   }

   return rval;
}



int free_pers_conf_key(PersistenceConfigurationKey_s* pc)
{
   int rval = 1;

   if(pc != NULL)
   {
      if(pc->reponsible != NULL)
      {
         free(pc->reponsible);
         pc->reponsible = NULL;
         //printf("free_pers_conf_key => free(pc->reponsible);");
      }

      if(pc->custom_name != NULL)
      {
         free(pc->custom_name);
         pc->custom_name = NULL;
         //printf("free_pers_conf_key => free(pc->reponsible);");
      }
   }

   return rval;
}



// status: OK
int get_db_context(unsigned char ldbid, char* resource_id, unsigned char user_no, unsigned char seat_no,
                   unsigned int isFile, char dbKey[], char dbPath[])
{
   int rval = 0,
       resourceFound = 0,
       groupId = 0;

   PersistenceRCT_e rct = PersistenceRCT_LastEntry;

   rct = get_table_id(ldbid, &groupId);

   // get resource configuration table
   GvdbTable* resource_table = get_resource_cfg_table(rct, groupId);

   if(resource_table != NULL)
   {
      GVariant* dbValue = NULL;

      // check if resouce id is in write through table
      dbValue = gvdb_table_get_value(resource_table, resource_id);

      if(dbValue != NULL)
      {
         PersistenceConfigurationKey_s dbEntry;

         gconstpointer valuePtr = NULL;
         int size = g_variant_get_size(dbValue);
         valuePtr = g_variant_get_data(dbValue);
         if(valuePtr != NULL)
         {
            char* buffer = malloc(size);
            memcpy(buffer, valuePtr, size);
            de_serialize_data(buffer, &dbEntry);

            if(dbEntry.storage != PersistenceStorage_custom )
            {
               // TODO check rval ==> double defined shared/local/custom via ldbid and dbEntry.policy
               rval = get_db_path_and_key(ldbid, resource_id, user_no, seat_no, isFile, dbKey, dbPath, dbEntry.policy);
               if(rval != -1)
               {
                  rval = dbEntry.storage;
               }
            }
            else
            {
               printf("***************** dbEntry.custom_name %s \n", dbEntry.custom_name);
               // if customer storage, we use the custom name as path
               strncpy(dbPath, dbEntry.custom_name, strlen(dbEntry.custom_name));
               rval = dbEntry.storage;
            }

            free(buffer);
            buffer = NULL;
            free_pers_conf_key(&dbEntry);
            resourceFound = 1;
         }
      }
      else
      {
         printf("get_db_context - resource_table: no value for key: %s \n", resource_id);
         rval = -1;
      }
   }  // resource table


   if(resourceFound == 0)
   {
      printf("get_db_context - error resource not found %s \n", resource_id);
   }

   return rval;
}



// status: OK
int get_db_path_and_key(unsigned char ldbid, char* resource_id, unsigned char user_no, unsigned char seat_no,
                        unsigned int isFile, char dbKey[], char dbPath[], unsigned char cached_resource)
{
   int storePolicy = PersistenceStoragePolicy_LastEntry;

   //
   // create resource database key
   //
   if((ldbid < 0x80) || (ldbid == 0xFF) )
   {
      // The LDBID is used to find the DBID in the resource table.
      if((user_no == 0) && (seat_no == 0))
      {
         //
         // Node is added in front of the resource ID as the key string.
         //
         snprintf(dbKey, dbKeyMaxLen, "%s%s", gNode, resource_id);
      }
      else
      {
         //
         // Node is added in front of the resource ID as the key string.
         //
         if(seat_no == 0)
         {
            // /User/<user_no_parameter> is added in front of the resource ID as the key string.
            snprintf(dbKey, dbKeyMaxLen, "%s%d%s", gUser, user_no, resource_id);
         }
         else
         {
            // /User/<user_no_parameter>/Seat/<seat_no_parameter> is added in front of the resource ID as the key string.
            snprintf(dbKey, dbKeyMaxLen, "%s%d%s%d%s", gUser, user_no, gSeat, seat_no, resource_id);
         }
      }
      storePolicy = PersistenceStorage_local;
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
      storePolicy = PersistenceStorage_local;
   }

   //
   // create resource database path
   //
   if(ldbid < 0x80)
   {
      // S H A R E D  database

      if(ldbid != 0)
      {
         // Additionally /GROUP/<LDBID_parameter> shall be added inside of the database path listed in the resource table. (Off target)
         //
         // shared  G R O U P  database * * * * * * * * * * * * *  * * * * * *
         //
         if(PersistencePolicy_wc == cached_resource)
         {
            if(isFile == resIsNoFile)
               snprintf(dbPath, dbPathMaxLen, gSharedCachePath, ldbid, gSharedCached);
            else
               snprintf(dbPath, dbPathMaxLen, gSharedCachePath, ldbid, dbKey);
         }
         else if(PersistencePolicy_wt == cached_resource)
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
         //
         // shared  P U B L I C  database * * * * * * * * * * * * *  * * * * *
         //
         if(PersistencePolicy_wc == cached_resource)
         {
            if(isFile == resIsNoFile)
               snprintf(dbPath, dbPathMaxLen, gSharedPublicCachePath, gSharedCached);
            else
               snprintf(dbPath, dbPathMaxLen, gSharedPublicCachePath, dbKey);
         }
         else if(PersistencePolicy_wt == cached_resource)
         {
            if(isFile == resIsNoFile)
               snprintf(dbPath, dbPathMaxLen, gSharedPublicWtPath, gSharedWt);
            else
               snprintf(dbPath, dbPathMaxLen, gSharedPublicWtPath, dbKey);
         }
      }

      storePolicy = PersistenceStorage_shared;   // we have a shared database
   }
   else
   {
      // L O C A L   database

      if(PersistencePolicy_wc == cached_resource)
      {
         if(isFile == resIsNoFile)
            snprintf(dbPath, dbPathMaxLen, gLocalCachePath, gAppId, gLocalCached);
         else
            snprintf(dbPath, dbPathMaxLen, gLocalCachePath, gAppId, dbKey);
      }
      else if(PersistencePolicy_wt == cached_resource)
      {
         if(isFile == resIsNoFile)
            snprintf(dbPath, dbPathMaxLen, gLocalWtPath, gAppId, gLocalWt);
         else
            snprintf(dbPath, dbPathMaxLen, gLocalWtPath, gAppId, dbKey);
      }

      storePolicy = PersistenceStorage_local;   // we have a local database
   }

   //printf("get_db_path_and_key - dbKey  : [key ]: %s \n",  dbKey);
   //printf("get_db_path_and_key - dbPath : [path]: %s\n\n", dbPath);
   return storePolicy;
}






