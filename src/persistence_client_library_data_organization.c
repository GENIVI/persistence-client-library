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


int gDbusPendingRvalue = 0;

/// application id
char gAppId[PERS_RCT_MAX_LENGTH_RESPONSIBLE] = { [0 ... PERS_RCT_MAX_LENGTH_RESPONSIBLE-1] = 0};

/// flag to indicate if client library has been initialized
unsigned int gPclInitCounter = 0;

/// the DLT context
DltContext gPclDLTContext;

int(* gChangeNotifyCallback)(pclNotification_s * notifyStruct);

/// character lookup table used for parsing configuration files
const char gCharLookup[] =
{
   0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,  // from 0x0 (NULL)  to 0x1F (unit seperator)
   0,1,1,1,1,1,1,1,  1,1,1,1,1,1,1,1,  1,1,1,1,1,1,1,1,  1,1,1,1,1,1,1,1,  // from 020 (space) to 0x2F (?)
   1,1,1,1,1,1,1,1,  1,1,1,1,1,1,1,1,  1,1,1,1,1,1,1,1,  1,1,1,1,1,1,1,1,  // from 040 (@)     to 0x5F (_)
   1,1,1,1,1,1,1,1,  1,1,1,1,1,1,1,1,  1,1,1,1,1,1,1,1,  1,1,1,1,1,1,1     // from 060 (')     to 0x7E (~)

};


// path prefix for local cached database: /Data/mnt_c/<appId>/ (<database_name>
static const char* gLocalCachePath_        = CACHEPREFIX "%s";
// path prefix for local write through database /Data/mnt_wt/<appId>/<database_name>
static const char* gLocalWtPath_           = WTPREFIX "%s";
// path prefix for shared cached database: /Data/mnt_c/Shared/Group/<group_no>/<database_name>
static const char* gSharedCachePath_       = CACHEPREFIX "%s/shared_group_%x";
// path prefix for shared write through database: /Data/mnt_wt/Shared/Group/<group_no>/<database_name>
static const char* gSharedWtPath_          = WTPREFIX "%s/shared_group_%x";
// path prefix for shared public cached database: /Data/mnt_c/Shared/Public//<database_name>
static const char* gSharedPublicCachePath_ = CACHEPREFIX "%s/shared_public";
// path prefix for shared public write through database: /Data/mnt_wt/Shared/Public/<database_name>
static const char* gSharedPublicWtPath_    = WTPREFIX "%s/shared_public";

// path prefix for local cached database: /Data/mnt_c/<appId>/ (<database_name>
static const char* gLocalCachePathKey        = CACHEPREFIX "%s%s";
// path prefix for local write through database /Data/mnt_wt/<appId>/<database_name>
static const char* gLocalWtPathKey           = WTPREFIX "%s%s";
// path prefix for shared cached database: /Data/mnt_c/Shared/Group/<group_no>/<database_name>
static const char* gSharedCachePathKey       = CACHEPREFIX "%s/shared_group_%x%s";
// path prefix for shared write through database: /Data/mnt_wt/Shared/Group/<group_no>/<database_name>
static const char* gSharedWtPathKey          = WTPREFIX "%s/shared_group_%x%s";
// path prefix for shared public cached database: /Data/mnt_c/Shared/Public//<database_name>
static const char* gSharedPublicCachePathKey = CACHEPREFIX "%s/shared_public%s";
// path prefix for shared public write through database: /Data/mnt_wt/Shared/Public/<database_name>
static const char* gSharedPublicWtPathKey    = WTPREFIX "%s/shared_public%s";

// path prefix for local cached files: /Data/mnt_c/<appId>/<user>/<seat>/<resource>
const char* gLocalCacheFilePath        = CACHEPREFIX "%s"PERS_ORG_USER_FOLDER_NAME_"%d"PERS_ORG_SEAT_FOLDER_NAME_"%d/%s";


const char* getLocalCachePath(void)
{
   return gLocalCachePath_;
}
const char* getLocalWtPath(void)
{
   return gLocalWtPath_;
}
const char* getSharedCachePath(void)
{
   return gSharedCachePath_;
}
const char* getSharedWtPath(void)
{
   return gSharedWtPath_;
}
const char* getSharedPublicCachePath(void)
{
   return gSharedPublicCachePath_;
}
const char* getSharedPublicWtPath(void)
{
   return gSharedPublicWtPath_;
}
const char* getLocalCachePathKey(void)
{
   return gLocalCachePathKey;
}
const char* getLocalWtPathKey(void)
{
   return gLocalWtPathKey;
}
const char* getSharedCachePathKey(void)
{
   return gSharedCachePathKey;
}
const char* getSharedWtPathKey(void)
{
   return gSharedWtPathKey;
}
const char* getSharedPublicCachePathKey(void)
{
   return gSharedPublicCachePathKey;
}
const char* getSharedPublicWtPathKey(void)
{
   return gSharedPublicWtPathKey;
}

const char* getLocalCacheFilePath(void)
{
   return gLocalCacheFilePath;
}

