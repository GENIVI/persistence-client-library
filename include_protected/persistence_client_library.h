#ifndef PERSISTENCE_CLIENT_LIBRARY_H
#define PERSISTENCE_CLIENT_LIBRARY_H

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
 * @file           persistence_client_library.h
 * @ingroup        Persistence client library
 * @author         Ingo Huerner
 * @brief          Header of the persistence client library.
 *                 Library provides an API to access persistent data
 * @see            
 */


#ifdef __cplusplus
extern "C" {
#endif

#define  PERSIST_CLIENT_LIBRARY_INTERFACE_VERSION   (0x01000000U)

#include "../include/persistence_client_library_error_def.h"
#include "crc32.h"

#include <string.h>
#include <stdio.h>


/// constant definitions
enum _PersistenceConstantDef
{
   ResIsNoFile          = 0,        /// flag to identify that resource a not file
   ResIsFile            = 1,        /// flag to identify that resource a file
   AccessNoLock         = 1,        /// flag to indicate that access is not locked

   FileClosed           = 0,
   FileOpen             = 1,

   NsmShutdownNormal       = 1,     /// lifecycle shutdown normal
   NsmErrorStatus_OK       = 1,
   NsmErrorStatus_Fail     = -1,

   PrctKeySize             = 64,    /// persistence resource config table max key size
   PrctValueSize           = 256,   /// persistence resource config table max value size
   PrctDbTableSize         = 1024,  /// number of persistence resource config tables to store

   DbKeySize               = 64,    /// database max key size
   DbValueSize             = 16384, /// database max value size
   DbTableSize             = 1024,  /// database table size

   PasMsg_Block            = 1,     /// persistence administration service block access
   PasMsg_WriteBack        = 2,     /// persistence administration service write_back
   PasMsg_Unblock          = 4,     /// persistence administration service unblock access
   PasErrorStatus_RespPend = 88,    /// persistence administration service msg return status
   PasErrorStatus_OK       = 100,   /// persistence administration service msg return status
   PasErrorStatus_FAIL     = -1,    /// persistence administration service msg return status

   CustLibMaxLen = 128,             /// max length of the custom library name and path
   DbKeyMaxLen   = 128,             /// max database key length
   DbPathMaxLen  = 128,             /// max database path length
   MaxAppNameLen = 128,             /// max application name
   MaxPersHandle = 256,             /// max number of parallel open persistence handles

   MaxConfKeyLengthResp    = 32,    /// length of the config key responsible name
   MaxConfKeyLengthCusName = 32,    /// length of the config key custom name

   defaultMaxKeyValDataSize = 16384 /// default limit the key-value data size to 16kB
};


/// enumerator used to identify the policy to manage the data
typedef enum _PersistencePolicy_e
{
   PersistencePolicy_wc    = 0,  /**< the data is managed write cached */
   PersistencePolicy_wt    = 1,  /**< the data is managed write through */
   PersistencePolicy_na    = 2,  /**< the data is not applicable */

   /** insert new entries here ... */
   PersistencePolicy_LastEntry         /**< last entry */

} PersistencePolicy_e;


/// enumerator used to identify the persistence storage to manage the data
typedef enum _PersistenceStorage_e
{
   PersistenceStorage_local    = 0,  /**< the data is managed local */
   PersistenceStorage_shared   = 1,  /**< the data is managed shared */
   PersistenceStorage_custom   = 2,  /**< the data is managed over custom client implementation */

   /** insert new entries here ... */
   PersistenceStoragePolicy_LastEntry         /**< last entry */

} PersistenceStorage_e;


/// structure used to manage database context
typedef struct _PersistenceDbContext_s
{
   unsigned char ldbid;
   unsigned char user_no;
   unsigned char seat_no;
} PersistenceDbContext_s;


/// structure used to manage the persistence configuration for a key
typedef struct _PersistenceConfigurationKey_s
{
   PersistencePolicy_e  policy;                                /**< policy  */
   PersistenceStorage_e storage;                               /**< definition of storage to use */
   unsigned int         permission;                            /**< access right, corresponds to UNIX */
   unsigned int         max_size;                              /**< max size expected for the key */
   char                 reponsible[MaxConfKeyLengthResp];      /**< name of responsible application */
   char                 custom_name[MaxConfKeyLengthCusName];  /**< name of the customer plugin */
} PersistenceConfigurationKey_s;


/// structure definition of an persistence resource configuration table entry
typedef struct _PersistenceRctEntry_s
{
    char key[PrctKeySize];                                   /**< the key */
    PersistenceConfigurationKey_s data;                      /**< data for the key */
}
PersistenceRctEntry_s;


/// persistence information
typedef struct _PersistenceInfo_s
{
   PersistenceDbContext_s           context;          /**< database context*/
   PersistenceConfigurationKey_s    configKey;        /**< prct configuration key*/

} PersistenceInfo_s;



/// persistence resource config table type definition
typedef enum _PersistenceRCT_e
{
   PersistenceRCT_local         = 0,
   PersistenceRCT_shared_public = 1,
   PersistenceRCT_shared_group  = 2,

   PersistenceRCT_LastEntry                // last Entry

} PersistenceRCT_e;


/// resource configuration table name
extern const char* gResTableCfg;

/// shared cached default database
extern const char* gSharedCachedDefault;
/// shared cached database
extern const char* gSharedCached;
/// shared write through default database
extern const char* gSharedWtDefault;
/// shared write through database
extern const char* gSharedWt;

/// local cached default database
extern const char* gLocalCachedDefault;
/// local cached default database
extern const char* gLocalCached;
/// local write through default database
extern const char* gLocalWtDefault;
/// local write through default database
extern const char* gLocalWt;


/// directory structure node name defintion
extern const char* gNode;
/// directory structure user name defintion
extern const char* gUser;
/// directory structure seat name defintion
extern const char* gSeat;


/// path prefic for local cached database: /Data/mnt_c/<appId>/<database_name>
extern const char* gLocalCachePath;
/// path prefic for local write through database /Data/mnt_wt/<appId>/<database_name>
extern const char* gLocalWtPath;
/// path prefic for shared cached database: /Data/mnt_c/Shared/Group/<group_no>/<database_name>
extern const char* gSharedCachePath;
/// path prefic for shared write through database: /Data/mnt_wt/Shared/Group/<group_no>/<database_name>
extern const char* gSharedWtPath;
/// path prefic for shared public cached database: /Data/mnt_c/Shared/Public//<database_name>
extern const char* gSharedPublicCachePath;
/// path prefic for shared public write through database: /Data/mnt_wt/Shared/Public/<database_name>
extern const char* gSharedPublicWtPath;


/// application id
extern char gAppId[MaxAppNameLen];

/// max key value data size
extern int gMaxKeyValDataSize;

#ifdef __cplusplus
}
#endif

#endif /* PERSISTENCY_CLIENT_LIBRARY_H */

