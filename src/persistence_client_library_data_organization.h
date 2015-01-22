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

#include <dlt.h>
#include <string.h>



/// structure used to manage database context
typedef struct _PersistenceDbContext_s
{
	/// lofical database id
   unsigned int ldbid;
   /// user number
   int user_no;
   /// seat number
   unsigned int seat_no;
} PersistenceDbContext_s;

/// persistence information
typedef struct _PersistenceInfo_s
{
	/// database context ::PersistenceDbContext_s
   PersistenceDbContext_s           context;
   /// Persistence resource configuration key
   PersistenceConfigurationKey_s    configKey;

} PersistenceInfo_s;


/// storages to manage the data
typedef enum PersDefaultType_e_
{
	/// get the data from configurable defaults
   PersDefaultType_Configurable = 0,
   /// get the data from factory defaults
   PersDefaultType_Factory,

   /** insert new entries here ... */

   /// last entry
   PersDefaultType_LastEntry

} PersDefaultType_e;

/// storages to manage the data
typedef enum PersGetDefault_e_
{
	/// get the data from configurable defaults
   PersGetDefault_Data = 0,
   /// get the data from factory defaults
   PersGetDefault_Size,

   /** insert new entries here ... */

   /// last entry
   PersGetDefault_LastEntry

} PersGetDefault_e;

/// enumerator used to identify the policy to manage the data
typedef enum _PersNotifyRegPolicy_e
{
	/// register to change notifications
   Notify_register   = 0,
   ///  unregister for change notifications
   Notify_unregister = 1,

   /// last entry
   Notify_lastEntry,
} PersNotifyRegPolicy_e;


/// constant definitions
enum _PersistenceConstantDef
{
	/// flag to identify if a backup should NOT be created
	DONT_CREATE_BACKUP   = 0,
	/// flag to identify if a backup should be created
	CREATE_BACKUP        = 1,
	/// flag to identify that resource a not file
   ResIsNoFile          = 0,
   /// flag to identify that resource a file
   ResIsFile            = 1,
   /// flag to indicate that access is not locked
   AccessNoLock         = 1,
   /// flag to identify if file will be closed
   FileClosed           = 1,
   /// flag to identify if file has been opened
   FileOpen             = 1,
   /// make partial Shutdown (close but not free everything)
   Shutdown_Partial      = 0,
   /// make complete Shutdown (close and free everything)
   Shutdown_Full         = 1,
   /// max number of shutdown cancel calls
   Shutdown_MaxCount     = 3,
   /// lifecycle shutdown normal
   NsmShutdownNormal       = 1,
   /// lifecycle return OK indicator
   NsmErrorStatus_OK       = 1,
   /// lifecycle return failed indicator
   NsmErrorStatus_Fail     = -1,
   /// max checksum size
   ChecksumBufSize         = 64,
   /// max character sub match size
   DbusSubMatchSize        = 12,
   /// max character size of the dbus match rule size
   DbusMatchRuleSize       = 300,
   /// persistence resource config table max key size
   PrctKeySize             = PERS_RCT_MAX_LENGTH_RESOURCE_ID,
   /// persistence resource config table max value size
   PrctValueSize           = sizeof(PersistenceConfigurationKey_s),
   /// number of persistence resource config tables to store
   PrctDbTableSize         = 1024,
   /// write buffer size
   RDRWBufferSize          = 1024,
   /// database max key size
   DbKeySize               = PERS_DB_MAX_LENGTH_KEY_NAME,
   /// database max value size
   DbValueSize             = PERS_DB_MAX_SIZE_KEY_DATA,
   /// database table size
   DbTableSize             = 1024,
   /// persistence administration service block access
   PasMsg_Block            = 0x0001,
   /// persistence administration service unblock access
   PasMsg_Unblock          = 0x0002,
   /// persistence administration service write_back
   PasMsg_WriteBack        = 0x0010,
   /// persistence administration service msg return status
   PasErrorStatus_RespPend = 0x0001,
   /// persistence administration service msg return status
   PasErrorStatus_OK       = 0x0002,
   /// persistence administration service msg return status
   PasErrorStatus_FAIL     = 0x8000,
   /// max length of the custom library name and path
   CustLibMaxLen = PERS_RCT_MAX_LENGTH_CUSTOM_NAME,
   /// max database key length
   DbKeyMaxLen   = PERS_DB_MAX_LENGTH_KEY_NAME,
   /// max database key length
   DbResIDMaxLen = PERS_DB_MAX_LENGTH_KEY_NAME,
   /// max database path length
   DbPathMaxLen  = PERS_ORG_MAX_LENGTH_PATH_FILENAME,
   /// max application name
   MaxAppNameLen = PERS_RCT_MAX_LENGTH_RESPONSIBLE,
   /// max number of parallel open persistence handles
   MaxPersHandle = 255,
   /// length of the config key responsible name
   MaxConfKeyLengthResp    = 32,
   /// length of the config key custom name
   MaxConfKeyLengthCusName = 32,
   /// length of the customer ID
   MaxRctLengthCustom_ID   = 64,
   /// token array size
   TOKENARRAYSIZE = 255,
   /// default limit the key-value data size to 16kB
   defaultMaxKeyValDataSize = PERS_DB_MAX_SIZE_KEY_DATA
};


// define PERS_ORG_ROOT_PATH comes form persistence common object

/// cached path location
#define CACHEPREFIX         PERS_ORG_ROOT_PATH "/mnt-c/"
/// write through path location
#define WTPREFIX            PERS_ORG_ROOT_PATH "/mnt-wt/"

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

/// path prefix for local cached database: /Data/mnt_c/&lt;appId&gt;/&lt;database_name&gt;
extern const char* gLocalCachePath;
/// path prefix for local write through database /Data/mnt_wt/&lt;appId&gt;/&lt;database_name&gt;
extern const char* gLocalWtPath;
/// path prefix for shared cached database: /Data/mnt_c/Shared/Group/&lt;group_no&gt;/&lt;database_name&gt;
extern const char* gSharedCachePath;
/// path prefix for shared write through database: /Data/mnt_wt/Shared/Group/&lt;group_no&gt;/&lt;database_name&gt;
extern const char* gSharedWtPath;
/// path prefix for shared public cached database: /Data/mnt_c/Shared/Public//&lt;database_name&gt;
extern const char* gSharedPublicCachePath;
/// path prefix for shared public write through database: /Data/mnt_wt/Shared/Public/&lt;database_name&gt;
extern const char* gSharedPublicWtPath;

/// path prefix for local cached database: /Data/mnt_c/&lt;appId&gt;/&lt;database_name&gt;
extern const char* gLocalCachePathKey;
/// path prefix for local write through database /Data/mnt_wt/&lt;appId&gt;/&lt;database_name&gt;
extern const char* gLocalWtPathKey;
/// path prefix for shared cached database: /Data/mnt_c/Shared/Group/&lt;group_no&gt;/&lt;database_name&gt;
extern const char* gSharedCachePathKey;
/// path prefix for shared write through database: /Data/mnt_wt/Shared/Group/&lt;group_no&gt;/&lt;database_name&gt;
extern const char* gSharedWtPathKey;
/// path prefix for shared public cached database: /Data/mnt_c/Shared/Public//&lt;database_name&gt;
extern const char* gSharedPublicCachePathKey;
/// path prefix for shared public write through database: /Data/mnt_wt/Shared/Public/&lt;database_name&gt;
extern const char* gSharedPublicWtPathKey;

/// path prefix for local cached files: /Data/mnt_c/&lt;appId&gt;/&lt;user&gt;/&gt;userno&gt;/&lt;seat&gt;/&gt;seatno&gt;/&lt;resource&gt;
extern const char* gLocalCacheFilePath;

// backup blacklist filename
extern const char* gBackupFilename;

/// application id
extern char gAppId[MaxAppNameLen];

/// max key value data size
extern int gMaxKeyValDataSize;

/// the DLT context
extern DltContext gPclDLTContext;

/// flag to indicate if client library has been initialized
extern unsigned int gPclInitCounter;


/// change signal string
extern const char* gChangeSignal;
/// delete signal string
extern const char* gDeleteSignal;
/// create signal string
extern const char* gCreateSignal;

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

