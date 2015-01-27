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
 * @file           persistence_client_library_data_organization.c
 * @ingroup        Persistence client library
 * @author         Ingo Huerner
 * @brief          Implementation of persistence database low level access
 * @see            
 */

#include "persistence_client_library_data_organization.h"

/// path for the backup location
const char* gBackupPrefix  	= PERS_ORG_ROOT_PATH "/mnt-backup/";

// size of cached path string
const int gCPathPrefixSize = sizeof(CACHEPREFIX)-1;
// size of write through string
const int gWTPathPrefixSize = sizeof(WTPREFIX)-1;


// backup filename postfix
const char* gBackupPostfix 	= "~";
// backup checksum filename postfix
const char* gBackupCsPostfix 	= "~.crc";

// path prefix for local cached database: /Data/mnt_c/<appId>/ (<database_name>
const char* gLocalCachePath        = CACHEPREFIX "%s";
// path prefix for local write through database /Data/mnt_wt/<appId>/<database_name>
const char* gLocalWtPath           = WTPREFIX "%s";
// path prefix for shared cached database: /Data/mnt_c/Shared/Group/<group_no>/<database_name>
const char* gSharedCachePath       = CACHEPREFIX "%s/shared_group_%x";
// path prefix for shared write through database: /Data/mnt_wt/Shared/Group/<group_no>/<database_name>
const char* gSharedWtPath          = WTPREFIX "%s/shared_group_%x";
// path prefix for shared public cached database: /Data/mnt_c/Shared/Public//<database_name>
const char* gSharedPublicCachePath = CACHEPREFIX "%s/shared_public";
// path prefix for shared public write through database: /Data/mnt_wt/Shared/Public/<database_name>
const char* gSharedPublicWtPath    = WTPREFIX "%s/shared_public";

// path prefix for local cached database: /Data/mnt_c/<appId>/ (<database_name>
const char* gLocalCachePathKey        = CACHEPREFIX "%s%s";
// path prefix for local write through database /Data/mnt_wt/<appId>/<database_name>
const char* gLocalWtPathKey           = WTPREFIX "%s%s";
// path prefix for shared cached database: /Data/mnt_c/Shared/Group/<group_no>/<database_name>
const char* gSharedCachePathKey       = CACHEPREFIX "%s/shared_group_%x%s";
// path prefix for shared write through database: /Data/mnt_wt/Shared/Group/<group_no>/<database_name>
const char* gSharedWtPathKey          = WTPREFIX "%s/shared_group_%x%s";
// path prefix for shared public cached database: /Data/mnt_c/Shared/Public//<database_name>
const char* gSharedPublicCachePathKey = CACHEPREFIX "%s/shared_public%s";
// path prefix for shared public write through database: /Data/mnt_wt/Shared/Public/<database_name>
const char* gSharedPublicWtPathKey    = WTPREFIX "%s/shared_public%s";

// path prefix for local cached files: /Data/mnt_c/<appId>/<user>/<seat>/<resource>
const char* gLocalCacheFilePath        = CACHEPREFIX "%s"PERS_ORG_USER_FOLDER_NAME_"%d"PERS_ORG_SEAT_FOLDER_NAME_"%d/%s";

const char* gBackupFilename = "BackupFileList.info";

const char* gChangeSignal = "PersistenceResChange";
const char* gDeleteSignal = "PersistenceResDelete";
const char* gCreateSignal = "PersistenceResCreate";

int gTimeoutMs = 5000;

int gDbusPendingRvalue = 0;


/// application id
char gAppId[MaxAppNameLen] = { [0 ... MaxAppNameLen-1] = 0};


/// max key value data size [default 16kB]
int gMaxKeyValDataSize = defaultMaxKeyValDataSize;


unsigned int gPclInitCounter = 0;


DltContext gPclDLTContext;

int(* gChangeNotifyCallback)(pclNotification_s * notifyStruct);


const char gCharLookup[] =
{
   0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,  // from 0x0 (NULL)  to 0x1F (unit seperator)
   0,1,1,1,1,1,1,1,  1,1,1,1,1,1,1,1,  1,1,1,1,1,1,1,1,  1,1,1,1,1,1,1,1,  // from 020 (space) to 0x2F (?)
   1,1,1,1,1,1,1,1,  1,1,1,1,1,1,1,1,  1,1,1,1,1,1,1,1,  1,1,1,1,1,1,1,1,  // from 040 (@)     to 0x5F (_)
   1,1,1,1,1,1,1,1,  1,1,1,1,1,1,1,1,  1,1,1,1,1,1,1,1,  1,1,1,1,1,1,1     // from 060 (')     to 0x7E (~)

};


const char* gPluginTypeDefault   = "default";
const char* gPluginTypeEarly     = "early";
const char* gPluginTypeSecure    = "secure";
const char* gPluginTypeEmergency = "emergency";
const char* gPluginTypeHwInfo    = "hwinfo";
const char* gPluginTypeCustom1   = "custom1";
const char* gPluginTypeCustom2   = "custom2";
const char* gPluginTypeCustom3   = "custom3";


