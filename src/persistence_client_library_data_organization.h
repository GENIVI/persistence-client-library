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


// define PERS_ORG_ROOT_PATH has been defined in persistence common object

/// cached path location
#define CACHEPREFIX         PERS_ORG_ROOT_PATH "/mnt-c/"
/// write through path location
#define WTPREFIX            PERS_ORG_ROOT_PATH "/mnt-wt/"


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
   Notify_lastEntry
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
   /// persistence resource config table max value size
   PrctValueSize           = sizeof(PersistenceConfigurationKey_s),
   /// number of persistence resource config tables to store
   PrctDbTableSize         = 1024,
   /// write buffer size
   RDRWBufferSize          = 1024,
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
};

/**
 * @brief get the local cache path
 *
 * @ return the path
 */
const char* getLocalCachePath(void);

/**
 * @brief get the local wt path
 *
 * @ return the path
 */
const char* getLocalWtPath(void);

/**
 * @brief get the shared cache path
 *
 * @ return the path
 */
const char* getSharedCachePath(void);

/**
 * @brief get the shared wt path
 *
 * @ return the path
 */
const char* getSharedWtPath(void);

/**
 * @brief get the shared public cache path
 *
 * @ return the path
 */
const char* getSharedPublicCachePath(void);

/**
 * @brief get shared public wt path
 *
 * @ return the path
 */
const char* getSharedPublicWtPath(void);

/**
 * @brief get the local cache path key
 *
 * @ return the key
 */
const char* getLocalCachePathKey(void);

/**
 * @brief get local wt path key
 *
 * @ return the key
 */
const char* getLocalWtPathKey(void);

/**
 * @brief get the shared cache path key
 *
 * @ return the key
 */
const char* getSharedCachePathKey(void);

/**
 * @brief get the shared wt path key
 *
 * @ return the key
 */
const char* getSharedWtPathKey(void);

/**
 * @brief get the shared public cache path key
 *
 * @ return the key
 */
const char* getSharedPublicCachePathKey(void);

/**
 * @brief get the shared public write through path
 *
 * @ return the key
 */
const char* getSharedPublicWtPathKey(void);

/**
 * @brief get local cache file path
 *
 * @ return the path
 */
const char* getLocalCacheFilePath(void);



/// application id
extern char gAppId[PERS_RCT_MAX_LENGTH_RESPONSIBLE] __attribute__ ((visibility ("hidden")));

/// the DLT context
extern DltContext gPclDLTContext __attribute__ ((visibility ("hidden")));

/// flag to indicate if client library has been initialized
extern unsigned int gPclInitCounter __attribute__ ((visibility ("hidden")));

/// dbus pending return value
extern int gDbusPendingRvalue __attribute__ ((visibility ("hidden")));


/**
 * @brief definition of change callback function
 *
 * @param pclNotification_s callback notification structure
 */
extern int(* gChangeNotifyCallback)(pclNotification_s * notifyStruct);

/// character lookup table used for parsing configuration files
extern const char gCharLookup[] __attribute__ ((visibility ("hidden")));


#ifdef __cplusplus
}
#endif

#endif /* PERSISTENCY_CLIENT_LIBRARY_DATA_ORGANIZATION_H */

