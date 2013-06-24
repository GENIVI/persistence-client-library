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

#define  PERSIST_CLIENT_LIBRARY_DATA_ORGANIZATION_INTERFACE_VERSION   (0x01040000U)

#include "../include/persistence_client_library_error_def.h"
#include "../include/persistence_client_library_key.h"

#include <dlt/dlt.h>
#include <dlt/dlt_common.h>

#include <string.h>
#include <stdio.h>


/// constant definitions
enum _PersistenceConstantDef
{
   ResIsNoFile          = 0,        /// flag to identify that resource a not file
   ResIsFile            = 1,        /// flag to identify that resource a file
   AccessNoLock         = 1,        /// flag to indicate that access is not locked

   PCLnotInitialized    = 0,        /// 
   PCLinitialized       = 1,        ///

   FileClosed           = 0,
   FileOpen             = 1,

   NsmShutdownNormal       = 1,        /// lifecycle shutdown normal
   NsmErrorStatus_OK       = 1,
   NsmErrorStatus_Fail     = -1,

   ChecksumBufSize         = 64,       /// max checksum size

   DbusSubMatchSize        = 12,       /// max character sub match size
   DbusMatchRuleSize       = 300,      /// max character size of the dbus match rule size

   PrctKeySize             = 64,       /// persistence resource config table max key size
   PrctValueSize           = 256,      /// persistence resource config table max value size
   PrctDbTableSize         = 1024,     /// number of persistence resource config tables to store

   RDRWBufferSize          = 1024,     /// write buffer size

   DbKeySize               = 64,       /// database max key size
   DbValueSize             = 16384,    /// database max value size
   DbTableSize             = 1024,     /// database table size

   PasMsg_Block            = 0x0001,   /// persistence administration service block access
   PasMsg_Unblock          = 0x0002,   /// persistence administration service unblock access
   PasMsg_WriteBack        = 0x0010,   /// persistence administration service write_back

   PasErrorStatus_RespPend = 0x0001,   /// persistence administration service msg return status
   PasErrorStatus_OK       = 0x0002,   /// persistence administration service msg return status
   PasErrorStatus_FAIL     = 0x8000,   /// persistence administration service msg return status

   CustLibMaxLen = 128,             /// max length of the custom library name and path
   DbKeyMaxLen   = 128,             /// max database key length
   DbResIDMaxLen = 128,             /// max database key length
   DbPathMaxLen  = 128,             /// max database path length
   MaxAppNameLen = 128,             /// max application name
   MaxPersHandle = 256,             /// max number of parallel open persistence handles

   MaxConfKeyLengthResp    = 32,    /// length of the config key responsible name
   MaxConfKeyLengthCusName = 32,    /// length of the config key custom name
   MaxRctLengthCustom_ID   = 64,    /// length of the customer ID

   defaultMaxKeyValDataSize = 16384 /// default limit the key-value data size to 16kB
};



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

/// path prefix for local cached files: /Data/mnt_c/<appId>/<user>/>userno>/<seat>/>seatno>/<resource>
extern const char* gLocalCacheFilePath;

/// application id
extern char gAppId[MaxAppNameLen];

/// max key value data size
extern int gMaxKeyValDataSize;

/// the DLT context
extern DltContext gDLTContext;

/// flag to indicate if client library has been initialized
extern unsigned int gPclInitialized;


/**
 * @brief definition of change callback function
 *
 * @param pclNotification_s callback notification structure
 */
extern int(* gChangeNotifyCallback)(pclNotification_s * notifyStruct);


#ifdef __cplusplus
}
#endif

#endif /* PERSISTENCY_CLIENT_LIBRARY_DATA_ORGANIZATION_H */

