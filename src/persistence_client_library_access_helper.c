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
 * @file           persistence_client_library_access_helper.c
 * @ingroup        Persistence client library
 * @author         Ingo Huerner
 * @brief          Implementation of persistence access helper functions
 * @see
 */


#include "persistence_client_library_access_helper.h"
#include "persistence_client_library_itzam_errors.h"
#include <stdlib.h>


/// pointer to resource table database
itzam_btree gResource_table[PrctDbTableSize];
/// array to hold the information of database is already open
int gResourceOpen[PrctDbTableSize] = {0};


/// structure definition of an persistence resource configuration table entry
typedef struct record_t
{
    char m_key[PrctKeySize];        // the key
    char m_data[PrctValueSize];     // data for the key
}
prct_record;


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


itzam_btree* get_resource_cfg_table_by_idx(int i)
{
   return &gResource_table[i];
}


// status: OK
itzam_btree* get_resource_cfg_table(PersistenceRCT_e rct, int group)
{
   int arrayIdx = 0;
   itzam_btree* tree = NULL;

   // create array index: index is a combination of resource config table type and group
   arrayIdx = rct + group;

   if(arrayIdx < PrctDbTableSize)
   {
      if(gResourceOpen[arrayIdx] == 0)   // check if database is already open
      {
         itzam_state  state;
         char filename[DbPathMaxLen];
         memset(filename, 0, DbPathMaxLen);

         switch(rct)    // create db name
         {
         case PersistenceRCT_local:
            snprintf(filename, DbPathMaxLen, gLocalWtPath, gAppId, gResTableCfg);
            break;
         case PersistenceRCT_shared_public:
            snprintf(filename, DbPathMaxLen, gSharedPublicWtPath, gResTableCfg);
            break;
         case PersistenceRCT_shared_group:
            snprintf(filename, DbPathMaxLen, gSharedWtPath, group, gResTableCfg);
            break;
         default:
            printf("get_resource_cfg_table - error: no valid PersistenceRCT_e\n");
            break;
         }

         //printf("get_resource_cfg_table => %s \n", filename);
         state = itzam_btree_open(&gResource_table[arrayIdx], filename, itzam_comparator_string, error_handler, 0 , 0);
         if(state != ITZAM_OKAY)
         {
            fprintf(stderr, "\nget_resource_cfg_table => Itzam problem: %s\n", STATE_MESSAGES[state]);
         }
         gResourceOpen[arrayIdx] = 1;  // remember the index has an DB entry
      }
      tree = &gResource_table[arrayIdx];
   }

   return tree;
}



int serialize_data(PersistenceConfigurationKey_s pc, char* buffer)
{
   int rval = 0;
   rval = snprintf(buffer, gMaxKeyValDataSize, "%d %d %u %d %s %s",
                                               pc.policy, pc.storage, pc.permission, pc.max_size,
                                               pc.reponsible, pc.custom_name);
   return rval;
}



int de_serialize_data(char* buffer, PersistenceConfigurationKey_s* pc)
{
   int rval = 1;
   char* token = NULL;
   //printf("\nde_serialize_data: %s \n", buffer);
   if((buffer != NULL) && (pc != NULL))
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
         rval = EPERS_DESER_POLICY;
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
         rval = EPERS_DESER_STORE;
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
         rval = EPERS_DESER_PERM;
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
         rval = EPERS_DESER_MAXSIZE;
      }

      token = strtok (NULL, " ");      // reponsible
      if(token != 0)
      {
         int size = strlen(token)+1;
         pc->reponsible = malloc(size);

         if(pc->reponsible != NULL)
         {
            strncpy(pc->reponsible, token, size);
            //printf("     pc->reponsible %s | 0x%x \n", pc->reponsible, (int)pc->reponsible);
         }
         else
         {
            rval = EPERS_DESER_ALLOCMEM;
            printf("de_serialize_data - error: can't allocate memory [reponsible] \n");
         }
      }
      else
      {
         printf("de_serialize_data - error: can't get [reponsible] \n");
         rval = EPERS_DESER_RESP;
      }

      token = strtok (NULL, " ");      // custom_name
      if(token != 0)
      {
         int size = strlen(token)+1;
         pc->custom_name = malloc(size);
         if(pc->custom_name != NULL )
         {
            strncpy(pc->custom_name, token, size);
         }
         else
         {
            rval = EPERS_DESER_ALLOCMEM;
            printf("de_serialize_data - error: can't allocate memory [custom_name] \n");
         }
         //printf("     pc->custom_name %s | 0x%x \n", pc->custom_name, (int)pc->custom_name);
      }
      else
      {
         char* na = "na";
         int size = strlen(na)+1;
         // custom name not available => no custom plugin
         pc->custom_name = malloc(size);
         if(pc->custom_name != NULL )
         {
            strncpy(pc->custom_name, "na", size);
         }
         else
         {
            rval = EPERS_DESER_ALLOCMEM;
            printf("de_serialize_data - error: can't allocate memory [custom_name-default] \n");
         }
      }
   }
   else
   {
      printf("de_serialize_data - error: buffer or PersistenceConfigurationKey_s is NULL\n");
      rval = EPERS_DESER_BUFORKEY;
   }

   return rval;
}



void free_pers_conf_key(PersistenceConfigurationKey_s* pc)
{
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
}



// status: OK
int get_db_context(PersistenceInfo_s* dbContext, char* resource_id, unsigned int isFile, char dbKey[], char dbPath[])
{
   int rval = 0, resourceFound = 0, groupId = 0;

   PersistenceRCT_e rct = PersistenceRCT_LastEntry;

   rct = get_table_id(dbContext->context.ldbid, &groupId);

   // get resource configuration table
   itzam_btree* resource_table = get_resource_cfg_table(rct, groupId);

   if(resource_table != NULL)
   {
      prct_record search;
      // check if resouce id is in write through table
      if(itzam_true == itzam_btree_find(resource_table, resource_id, &search))
      {
         //printf("get_db_context ==> data: %s\n", search.m_data);
         de_serialize_data(search.m_data, &dbContext->configKey);
         if(dbContext->configKey.storage != PersistenceStorage_custom )
         {
            rval = get_db_path_and_key(dbContext, resource_id, isFile, dbKey, dbPath);
         }
         else
         {
            // if customer storage, we use the custom name as path
            strncpy(dbPath, dbContext->configKey.custom_name, strlen(dbContext->configKey.custom_name));
         }
         free_pers_conf_key(&dbContext->configKey);
         resourceFound = 1;
      }
      else
      {
         printf("get_db_context - resource_table: no value for key: %s \n", resource_id);
         rval = EPERS_NOKEYDATA;
      }
   }  // resource table
   else
   {
      printf("get_db_context - error resource table\n");
      rval = EPERS_NOPRCTABLE;
   }

   if(resourceFound == 0)
   {
      printf("get_db_context - error resource not found %s \n", resource_id);
      rval = EPERS_NOKEY;
   }

   return rval;
}



// status: OK
int get_db_path_and_key(PersistenceInfo_s* dbContext, char* resource_id, unsigned int isFile, char dbKey[], char dbPath[])
{
   int storePolicy = PersistenceStoragePolicy_LastEntry;

   //
   // create resource database key
   //
   if((dbContext->context.ldbid < 0x80) || (dbContext->context.ldbid == 0xFF) )
   {
      // The LDBID is used to find the DBID in the resource table.
      if((dbContext->context.user_no == 0) && (dbContext->context.seat_no == 0))
      {
         //
         // Node is added in front of the resource ID as the key string.
         //
         snprintf(dbKey, DbKeyMaxLen, "%s/%s", gNode, resource_id);
      }
      else
      {
         //
         // Node is added in front of the resource ID as the key string.
         //
         if(dbContext->context.seat_no == 0)
         {
            // /User/<user_no_parameter> is added in front of the resource ID as the key string.
            snprintf(dbKey, DbKeyMaxLen, "%s%d/%s", gUser, dbContext->context.user_no, resource_id);
         }
         else
         {
            // /User/<user_no_parameter>/Seat/<seat_no_parameter> is added in front of the resource ID as the key string.
            snprintf(dbKey, DbKeyMaxLen, "%s%d%s%d/%s", gUser, dbContext->context.user_no, gSeat, dbContext->context.seat_no, resource_id);
         }
      }
      storePolicy = PersistenceStorage_local;
   }

   if((dbContext->context.ldbid >= 0x80) && (dbContext->context.ldbid != 0xFF))
   {
      // The LDBID is used to find the DBID in the resource table.
      // /<LDBID parameter> is added in front of the resource ID as the key string.
      //  Rational: Creates a namespace within one data base.
      //  Rational: Reduction of number of databases -> reduction of maintenance costs
      // /User/<user_no_parameter> and /Seat/<seat_no_parameter> are add after /<LDBID parameter> if there are different than 0.

      if(dbContext->context.seat_no != 0)
      {
         snprintf(dbKey, DbKeyMaxLen, "/%x%s%d%s%d/%s", dbContext->context.ldbid, gUser, dbContext->context.user_no, gSeat, dbContext->context.seat_no, resource_id);
      }
      else
      {
         snprintf(dbKey, DbKeyMaxLen, "/%x%s%d/%s", dbContext->context.ldbid, gUser, dbContext->context.user_no, resource_id);
      }
      storePolicy = PersistenceStorage_local;
   }

   //
   // create resource database path
   //
   if(dbContext->context.ldbid < 0x80)
   {
      // S H A R E D  database

      if(dbContext->context.ldbid != 0)
      {
         // Additionally /GROUP/<LDBID_parameter> shall be added inside of the database path listed in the resource table. (Off target)
         //
         // shared  G R O U P  database * * * * * * * * * * * * *  * * * * * *
         //
         if(PersistencePolicy_wc == dbContext->configKey.policy)
         {
            if(isFile == ResIsNoFile)
               snprintf(dbPath, DbPathMaxLen, gSharedCachePath, dbContext->context.ldbid, gSharedCached);
            else
               snprintf(dbPath, DbPathMaxLen, gSharedCachePath, dbContext->context.ldbid, dbKey);
         }
         else if(PersistencePolicy_wt == dbContext->configKey.policy)
         {
            if(isFile == ResIsNoFile)
               snprintf(dbPath, DbPathMaxLen, gSharedWtPath, dbContext->context.ldbid, gSharedWt);
            else
               snprintf(dbPath, DbPathMaxLen, gSharedWtPath, dbContext->context.ldbid, dbKey);
         }
      }
      else
      {
         // Additionally /Shared/Public shall be added inside of the database path listed in the resource table. (Off target)
         //
         // shared  P U B L I C  database * * * * * * * * * * * * *  * * * * *
         //
         if(PersistencePolicy_wc == dbContext->configKey.policy)
         {
            if(isFile == ResIsNoFile)
               snprintf(dbPath, DbPathMaxLen, gSharedPublicCachePath, gSharedCached);
            else
               snprintf(dbPath, DbPathMaxLen, gSharedPublicCachePath, dbKey);
         }
         else if(PersistencePolicy_wt == dbContext->configKey.policy)
         {
            if(isFile == ResIsNoFile)
               snprintf(dbPath, DbPathMaxLen, gSharedPublicWtPath, gSharedWt);
            else
               snprintf(dbPath, DbPathMaxLen, gSharedPublicWtPath, dbKey);
         }
      }

      storePolicy = PersistenceStorage_shared;   // we have a shared database
   }
   else
   {
      // L O C A L   database

      if(PersistencePolicy_wc == dbContext->configKey.policy)
      {
         if(isFile == ResIsNoFile)
            snprintf(dbPath, DbPathMaxLen, gLocalCachePath, gAppId, gLocalCached);
         else
            snprintf(dbPath, DbPathMaxLen, gLocalCachePath, gAppId, dbKey);
      }
      else if(PersistencePolicy_wt == dbContext->configKey.policy)
      {
         if(isFile == ResIsNoFile)
            snprintf(dbPath, DbPathMaxLen, gLocalWtPath, gAppId, gLocalWt);
         else
            snprintf(dbPath, DbPathMaxLen, gLocalWtPath, gAppId, dbKey);
      }

      storePolicy = PersistenceStorage_local;   // we have a local database
   }

   //printf("get_db_path_and_key - dbKey  : [key ]: %s \n",  dbKey);
   //printf("get_db_path_and_key - dbPath : [path]: %s\n\n", dbPath);
   return storePolicy;
}






