#ifndef PERSISTENCE_CLIENT_LIBRARY_DATA_ORGANIZATION_H
#define PERSISTENCE_CLIENT_LIBRARY_DATA_ORGANIZATION_H

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
 * @file           persistence_client_library_data_organization.h
 * @ingroup        Persistence client library
 * @author         Ingo Huerner
 * @brief          Header of the persistence client library data organization.
 * @see            
 */


#ifdef __cplusplus
extern "C" {
#endif

#define  PERSIST_CLIENT_LIBRARY_DATA_ORGANIZATION_INTERFACE_VERSION   (0x03000000U)

#include "../include/persistence_client_library_error_def.h"
#include "../include/persistence_client_library_key.h"

#include <persComRct.h>
#include <persComDbAccess.h>
#include <persComDataOrg.h>

#include <dlt/dlt.h>
#include <dlt/dlt_common.h>

#include <string.h>
#include <stdio.h>



/// structure used to manage database context
typedef struct _PersistenceDbContext_s
{
   unsigned int ldbid;
   unsigned int user_no;
   unsigned int seat_no;
} PersistenceDbContext_s;

/// persistence information
typedef struct _PersistenceInfo_s
{
   PersistenceDbContext_s           context;          /**< database context*/
   PersistenceConfigurationKey_s    configKey;        /**< prct configuration key*/

} PersistenceInfo_s;


/** storages to manage the data */
typedef enum PersDefaultType_e_
{
   PersDefaultType_Configurable = 0,  /**< get the data from configurable defaults */
   PersDefaultType_Factory,           /**< get the data from factory defaults */

   /** insert new entries here ... */
   PersDefaultType_LastEntry          /**< last entry */

} PersDefaultType_e;

/** storages to manage the data */
typedef enum PersGetDefault_e_
{
   PersGetDefault_Data = 0,           /**< get the data from configurable defaults */
   PersGetDefault_Size,               /**< get the data from factory defaults */

   /** insert new entries here ... */
   PersGetDefault_LastEntry           /**< last entry */

} PersGetDefault_e;

/// enumerator used to identify the policy to manage the data
typedef enum _PersNotifyRegPolicy_e
{
   Notify_register   = 0,  /**< register to change notifications*/
   Notify_unregister = 1,  /**< unregister for change notifications */
   Notify_lastEntry,       /**<last entry */
} PersNotifyRegPolicy_e;


/// constant definitions
enum _PersistenceConstantDef
{
   ResIsNoFile          = 0,        /// flag to identify that resource a not file
   ResIsFile            = 1,        /// flag to identify that resource a file
   AccessNoLock         = 1,        /// flag to indicate that access is not locked

   PCLnotInitialized    = 0,        /// indication if PCL is not initialized
   PCLinitialized       = 1,        /// indication if PCL is initialized

   FileClosed           = 1,        /// flag to identify if file will be closed
   FileOpen             = 1,        /// flag to identify if file has been opend

   Shutdown_Partial      = 0,			/// make partial Shutdown (close but not free everything)
   Shutdown_Full         = 1,			/// make complete Shutdown (close and free everything)
   Shutdown_MaxCount     = 3,			/// max number of shutdown cancel calls

   NsmShutdownNormal       = 1,     /// lifecycle shutdown normal
   NsmErrorStatus_OK       = 1,     /// lifecycle return OK indicator
   NsmErrorStatus_Fail     = -1,    /// lifecycle return failed indicator

   ChecksumBufSize         = 64,       /// max checksum size

   DbusSubMatchSize        = 12,       /// max character sub match size
   DbusMatchRuleSize       = 300,      /// max character size of the dbus match rule size

   PrctKeySize             = PERS_RCT_MAX_LENGTH_RESOURCE_ID,       		/// persistence resource config table max key size
   PrctValueSize           = sizeof(PersistenceConfigurationKey_s),   	/// persistence resource config table max value size
   PrctDbTableSize         = 1024,     /// number of persistence resource config tables to store

   RDRWBufferSize          = 1024,     /// write buffer size

   DbKeySize               = PERS_DB_MAX_LENGTH_KEY_NAME,	/// database max key size
   DbValueSize             = PERS_DB_MAX_SIZE_KEY_DATA,    	/// database max value size
   DbTableSize             = 1024,     /// database table size

   PasMsg_Block            = 0x0001,   /// persistence administration service block access
   PasMsg_Unblock          = 0x0002,   /// persistence administration service unblock access
   PasMsg_WriteBack        = 0x0010,   /// persistence administration service write_back

   PasErrorStatus_RespPend = 0x0001,   /// persistence administration service msg return status
   PasErrorStatus_OK       = 0x0002,   /// persistence administration service msg return status
   PasErrorStatus_FAIL     = 0x8000,   /// persistence administration service msg return status

   CustLibMaxLen = PERS_RCT_MAX_LENGTH_CUSTOM_NAME,		/// max length of the custom library name and path
   DbKeyMaxLen   = PERS_DB_MAX_LENGTH_KEY_NAME,       	/// max database key length
   DbResIDMaxLen = PERS_DB_MAX_LENGTH_KEY_NAME,          /// max database key length
   DbPathMaxLen  = PERS_ORG_MAX_LENGTH_PATH_FILENAME,    /// max database path length
   MaxAppNameLen = PERS_RCT_MAX_LENGTH_RESPONSIBLE,      /// max application name
   MaxPersHandle = 128,             /// max number of parallel open persistence handles

   MaxConfKeyLengthResp    = 32,    /// length of the config key responsible name
   MaxConfKeyLengthCusName = 32,    /// length of the config key custom name
   MaxRctLengthCustom_ID   = 64,    /// length of the customer ID

   TOKENARRAYSIZE = 255,

   defaultMaxKeyValDataSize = PERS_DB_MAX_SIZE_KEY_DATA  /// default limit the key-value data size to 16kB
};

/// path for the backup location
extern const char* gBackupPrefix;
/// backup filename postfix
extern const char* gBackupPostfix;
/// backup checksum filename postfix
extern const char* gBackupCsPostfix;

/// size of cached prefix string
extern const int gCPathPrefixSize;
/// size of write through prefix string
extern const int gWTPathPrefixSize;

/// path prefix for local cached database: /Data/mnt_c/<appId>/<database_name>
extern const char* gLocalCachePath;
/// path prefix for local write through database /Data/mnt_wt/<appId>/<database_name>
extern const char* gLocalWtPath;
/// path prefix for shared cached database: /Data/mnt_c/Shared/Group/<group_no>/<database_name>
extern const char* gSharedCachePath;
/// path prefix for shared write through database: /Data/mnt_wt/Shared/Group/<group_no>/<database_name>
extern const char* gSharedWtPath;
/// path prefix for shared public cached database: /Data/mnt_c/Shared/Public//<database_name>
extern const char* gSharedPublicCachePath;
/// path prefix for shared public write through database: /Data/mnt_wt/Shared/Public/<database_name>
extern const char* gSharedPublicWtPath;

/// path prefix for local cached database: /Data/mnt_c/<appId>/<database_name>
extern const char* gLocalCachePathKey;
/// path prefix for local write through database /Data/mnt_wt/<appId>/<database_name>
extern const char* gLocalWtPathKey;
/// path prefix for shared cached database: /Data/mnt_c/Shared/Group/<group_no>/<database_name>
extern const char* gSharedCachePathKey;
/// path prefix for shared write through database: /Data/mnt_wt/Shared/Group/<group_no>/<database_name>
extern const char* gSharedWtPathKey;
/// path prefix for shared public cached database: /Data/mnt_c/Shared/Public//<database_name>
extern const char* gSharedPublicCachePathKey;
/// path prefix for shared public write through database: /Data/mnt_wt/Shared/Public/<database_name>
extern const char* gSharedPublicWtPathKey;

/// path prefix for local cached files: /Data/mnt_c/<appId>/<user>/>userno>/<seat>/>seatno>/<resource>
extern const char* gLocalCacheFilePath;

/// application id
extern char gAppId[MaxAppNameLen];

/// max key value data size
extern int gMaxKeyValDataSize;

/// the DLT context
extern DltContext gPclDLTContext;

/// flag to indicate if client library has been initialized
extern unsigned int gPclInitialized;


/// change signal string
extern const char* gChangeSignal;
/// delete signal string
extern const char* gDeleteSignal;
/// create signal string
extern const char* gCreateSignal;


/**
 * Global notification variables, will be used to pass
 * the notification information into the mainloop.
 */
/// notification key string
extern char gNotifykey[DbKeyMaxLen];
/// notification lbid
extern unsigned int gNotifyLdbid;
/// notification user number
extern unsigned int gNotifyUserNo;
/// notification seat number
extern unsigned int gNotifySeatNo;
/// notification reason (created, changed, deleted)
extern pclNotifyStatus_e      gNotifyReason;
/// notification policy (register or unregister)
extern PersNotifyRegPolicy_e  gNotifyPolicy;


// dbus timeout (5 seconds)
extern int gTimeoutMs;

// dbus pending return value
extern int gDbusPendingRvalue;


/**
 * @brief definition of change callback function
 *
 * @param pclNotification_s callback notification structure
 */
extern int(* gChangeNotifyCallback)(pclNotification_s * notifyStruct);

/// character lookup table
extern const char gCharLookup[];


#ifdef __cplusplus
}
#endif

#endif /* PERSISTENCY_CLIENT_LIBRARY_DATA_ORGANIZATION_H */

