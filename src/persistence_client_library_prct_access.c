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
 * @file           persistence_client_library_prct_access.c
 * @ingroup        Persistence client library
 * @author         Ingo Huerner
 * @brief          Implementation of persistence resource configuration config
 *                 table access functions
 * @see
 */

#include "persistence_client_library_prct_access.h"
#include "persistence_client_library_custom_loader.h"


/// pointer to resource table database
static int gResource_table[PrctDbTableSize] = {[0 ... PrctDbTableSize-1] = -1};
/// array to hold the information of database is already open
static int gResourceOpen[PrctDbTableSize] = { [0 ... PrctDbTableSize-1] = 0 };


/// persistence resource config table type definition
typedef enum _PersistenceRCT_e
{
	/// resource is local (only visible for an application)
   PersistenceRCT_local         = 0,
   /// resource is visible for public
   PersistenceRCT_shared_public = 1,
   /// resource is visible for a group
   PersistenceRCT_shared_group  = 2,

   PersistenceRCT_LastEntry                // last Entry

} PersistenceRCT_e;


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
      rctType = PersistenceRCT_local;   // we have a local database
   }
   return rctType;
}


int get_resource_cfg_table_by_idx(int i)
{
   int idx = -1;

   if(i >= 0 && i < PrctDbTableSize)
   {
      idx = gResource_table[i];
   }
   return idx;
}


void invalidate_resource_cfg_table(int i)
{
   if(i >= 0 && i < PrctDbTableSize)
   {
      gResource_table[i] = -1;
      gResourceOpen[i] = 0;
   }
}


int get_resource_cfg_table(PersistenceRCT_e rct, int group)
{
   int arrayIdx = 0, rval = -1;

   // create array index: index is a combination of resource config table type and group
   arrayIdx = rct + group;

   if(arrayIdx < PrctDbTableSize)
   {
      if(gResourceOpen[arrayIdx] == 0)   // check if database is already open
      {
         char filename[PERS_ORG_MAX_LENGTH_PATH_FILENAME] = { [0 ... PERS_ORG_MAX_LENGTH_PATH_FILENAME-1] = 0};

         switch(rct)    // create db name
         {
         case PersistenceRCT_local:
            snprintf(filename, PERS_ORG_MAX_LENGTH_PATH_FILENAME, getLocalWtPathKey(), gAppId, plugin_gResTableCfg);
            break;
         case PersistenceRCT_shared_public:
            snprintf(filename, PERS_ORG_MAX_LENGTH_PATH_FILENAME, getSharedPublicWtPathKey(), gAppId, plugin_gResTableCfg);
            break;
         case PersistenceRCT_shared_group:
            snprintf(filename, PERS_ORG_MAX_LENGTH_PATH_FILENAME, getSharedWtPathKey(), gAppId, group, plugin_gResTableCfg);
            break;
         default:
            DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("gRCT - no valid PersistenceRCT_e"));
            break;
         }
         gResource_table[arrayIdx] = plugin_persComRctOpen(filename, 0x04);   // 0x04 ==> open in read only mode

         if(gResource_table[arrayIdx] < 0)
         {
         	gResourceOpen[arrayIdx] = 0;
            DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("gRCT - RCT problem"), DLT_INT(gResource_table[arrayIdx] ));
         }
         else
         {
             gResourceOpen[arrayIdx] = 1 ;
         }
      }

      rval = gResource_table[arrayIdx];
   }

   return rval;
}



int get_db_context(PersistenceInfo_s* dbContext, const char* resource_id, unsigned int isFile, char dbKey[], char dbPath[])
{
   int rval = 0, resourceFound = 0, groupId = 0, handleRCT = 0;
   PersistenceRCT_e rct = PersistenceRCT_LastEntry;

   rct = get_table_id(dbContext->context.ldbid, &groupId);

   handleRCT = get_resource_cfg_table(rct, groupId);    // get resource configuration table

   if(handleRCT >= 0)
   {
      PersistenceConfigurationKey_s sRctEntry ;

      // check if resouce id is in write through table
      int iErrCode = plugin_persComRctRead(handleRCT, resource_id, &sRctEntry) ;
      
      if(sizeof(PersistenceConfigurationKey_s) == iErrCode)
      {
    	   memcpy(&dbContext->configKey, &sRctEntry, sizeof(dbContext->configKey)) ;
         if(sRctEntry.storage != PersistenceStorage_custom )
         {
            rval = get_db_path_and_key(dbContext, resource_id, dbKey, dbPath);
         }
         else
         {
            // if customer storage, we use the custom name as dbPath
            strncpy(dbPath, dbContext->configKey.custom_name, strlen(dbContext->configKey.custom_name));

            strncpy(dbKey, resource_id, strlen(resource_id));     // and resource_id as dbKey
         }
         resourceFound = 1;
      }
      else
      {
         DLT_LOG(gPclDLTContext, DLT_LOG_WARN, DLT_STRING("gDBCtx - RCT: no value for key:"), DLT_STRING(resource_id) );
         rval = EPERS_NOKEYDATA;
      }
   }  // resource table
   else
   {
      DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("gDBCtx - RCT"));
      rval = EPERS_NOPRCTABLE;
   }

   if((resourceFound == 0) && (dbContext->context.ldbid == PCL_LDBID_LOCAL) ) // create only when the resource is local data
   {
      // resource NOT found in resource table ==> default is local cached key
      dbContext->configKey.policy      = PersistencePolicy_wc;
      dbContext->configKey.storage     = PersistenceStorage_local;
      dbContext->configKey.permission  = PersistencePermission_ReadWrite;
      dbContext->configKey.max_size    = PERS_DB_MAX_SIZE_KEY_DATA;
      if(isFile == PersistenceResourceType_file)
      {
         dbContext->configKey.type = PersistenceResourceType_file;
       }
      else
      {
         dbContext->configKey.type  = PersistenceResourceType_key;
      }

      memcpy(dbContext->configKey.customID, "A_CUSTOM_ID", strlen("A_CUSTOM_ID"));
      memcpy(dbContext->configKey.reponsible, "default", strlen("default"));
      memcpy(dbContext->configKey.custom_name, "default", strlen("default"));

      DLT_LOG(gPclDLTContext, DLT_LOG_DEBUG, DLT_STRING("gDBCtx - create res not in PRCT => key:"), DLT_STRING(resource_id) );

      rval = get_db_path_and_key(dbContext, resource_id, dbKey, dbPath);
   }
   /* rval contains the return value of function get_db_path_and_key() if positive structure content 'dbContext' is valid.
    * rval can be 0,1 or 2 but get_db_context should only return '0' for success. */
   if (0 < rval)
   {
	   rval = 0;
   }

   return rval;
}



int get_db_path_and_key(PersistenceInfo_s* dbContext, const char* resource_id, char dbKey[], char dbPath[])
{
   int storePolicy = PersistenceStorage_LastEntry;

   // create resource database key
   if(((dbContext->context.ldbid < 0x80) || (dbContext->context.ldbid == PCL_LDBID_LOCAL)) &&  (NULL != dbKey))
   {
      // The LDBID is used to find the DBID in the resource table.
      if((dbContext->context.user_no == 0) && (dbContext->context.seat_no == 0))
      {
         // Node is added in front of the resource ID as the key string.
         snprintf(dbKey, PERS_DB_MAX_LENGTH_KEY_NAME, "%s/%s", plugin_gNode, resource_id);
      }
      else
      {
         // Node is added in front of the resource ID as the key string.
         if(dbContext->context.seat_no == 0)
         {
            // /User/<user_no_parameter> is added in front of the resource ID as the key string.
            snprintf(dbKey, PERS_DB_MAX_LENGTH_KEY_NAME, "%s%d/%s", plugin_gUser, dbContext->context.user_no, resource_id);
         }
         else
         {
            // /User/<user_no_parameter>/Seat/<seat_no_parameter> is added in front of the resource ID as the key string.
            snprintf(dbKey, PERS_DB_MAX_LENGTH_KEY_NAME, "%s%d%s%d/%s", plugin_gUser, dbContext->context.user_no, plugin_gSeat, dbContext->context.seat_no, resource_id);
         }
      }
      storePolicy = PersistenceStorage_local;
   }

   if((dbContext->context.ldbid >= 0x80) && (dbContext->context.ldbid != PCL_LDBID_LOCAL) && (NULL != dbKey))
   {
      // The LDBID is used to find the DBID in the resource table.
      // /<LDBID parameter> is added in front of the resource ID as the key string.
      //  Rational: Creates a namespace within one data base.
      //  Rational: Reduction of number of databases -> reduction of maintenance costs
      // /User/<user_no_parameter> and /Seat/<seat_no_parameter> are add after /<LDBID parameter> if there are different than 0.
      if(dbContext->context.seat_no != 0)
      {
         snprintf(dbKey, PERS_DB_MAX_LENGTH_KEY_NAME, "/%x%s%d%s%d/%s", dbContext->context.ldbid, plugin_gUser, dbContext->context.user_no, plugin_gSeat, dbContext->context.seat_no, resource_id);
      }
      else
      {
         snprintf(dbKey, PERS_DB_MAX_LENGTH_KEY_NAME, "/%x%s%d/%s", dbContext->context.ldbid, plugin_gUser, dbContext->context.user_no, resource_id);
      }
      storePolicy = PersistenceStorage_local;
   }

   // create resource database path
   if(dbContext->context.ldbid < 0x80)
   {
      // S H A R E D  database

      if(dbContext->context.ldbid != PCL_LDBID_PUBLIC)
      {
         // Additionally /GROUP/<LDBID_parameter> shall be added inside of the database path listed in the resource table. (Off target)
         //
         // shared  G R O U P  database * * * * * * * * * * * * *  * * * * * *
         if(PersistencePolicy_wc == dbContext->configKey.policy)
         {
            if(dbContext->configKey.type == PersistenceResourceType_key)
               snprintf(dbPath, PERS_ORG_MAX_LENGTH_PATH_FILENAME, getSharedCachePath(), gAppId, dbContext->context.ldbid, "");
            else
               snprintf(dbPath, PERS_ORG_MAX_LENGTH_PATH_FILENAME, getSharedCachePathKey(), gAppId, dbContext->context.ldbid, dbKey);
         }
         else if(PersistencePolicy_wt == dbContext->configKey.policy)
         {
            if(dbContext->configKey.type == PersistenceResourceType_key)
               snprintf(dbPath, PERS_ORG_MAX_LENGTH_PATH_FILENAME, getSharedWtPath(), gAppId, dbContext->context.ldbid);
            else
               snprintf(dbPath, PERS_ORG_MAX_LENGTH_PATH_FILENAME, getSharedWtPathKey(), gAppId, dbContext->context.ldbid, dbKey);
         }
      }
      else
      {
         // Additionally /Shared/Public shall be added inside of the database path listed in the resource table. (Off target)
         //
         // shared  P U B L I C  database * * * * * * * * * * * * *  * * * * *
         if(PersistencePolicy_wc == dbContext->configKey.policy)
         {
            if(dbContext->configKey.type == PersistenceResourceType_key)
               snprintf(dbPath, PERS_ORG_MAX_LENGTH_PATH_FILENAME, getSharedPublicCachePath(), gAppId, "");
            else
               snprintf(dbPath, PERS_ORG_MAX_LENGTH_PATH_FILENAME, getSharedPublicCachePathKey(), gAppId, dbKey);
         }
         else if(PersistencePolicy_wt == dbContext->configKey.policy)
         {
            if(dbContext->configKey.type == PersistenceResourceType_key)
               snprintf(dbPath, PERS_ORG_MAX_LENGTH_PATH_FILENAME, getSharedPublicWtPath(), gAppId, "");
            else
               snprintf(dbPath, PERS_ORG_MAX_LENGTH_PATH_FILENAME, getSharedPublicWtPathKey(), gAppId, dbKey);
         }
      }
      storePolicy = PersistenceStorage_shared;   // we have a shared database
   }
   else
   {
      // L O C A L   database
      if(PersistencePolicy_wc == dbContext->configKey.policy)
      {
         if(dbContext->configKey.type == PersistenceResourceType_key)
            snprintf(dbPath, PERS_ORG_MAX_LENGTH_PATH_FILENAME, getLocalCachePath(), gAppId, "");
         else
            snprintf(dbPath, PERS_ORG_MAX_LENGTH_PATH_FILENAME, getLocalCachePathKey(), gAppId, dbKey);
      }
      else if(PersistencePolicy_wt == dbContext->configKey.policy)
      {
         if(dbContext->configKey.type == PersistenceResourceType_key)
            snprintf(dbPath, PERS_ORG_MAX_LENGTH_PATH_FILENAME-1, getLocalWtPath(), gAppId, "");
         else
            snprintf(dbPath, PERS_ORG_MAX_LENGTH_PATH_FILENAME-1, getLocalWtPathKey(), gAppId, dbKey);
      }

      storePolicy = PersistenceStorage_local;   // we have a local database
   }

   //printf("get_db_path_and_key - dbKey  : [key ]: %s \n",  dbKey);
   //printf("get_db_path_and_key - dbPath : [path]: %s\n", dbPath);

   return storePolicy;
}
