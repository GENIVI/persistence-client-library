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

#include "../include_protected/persistence_client_library_data_organization.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// rrsource configuration database name
const char* gResTableCfg = "/resource-table-cfg.itz";

/// configurable default database name
const char* gConfigDefault = "/configurable-default-data.itz";

/// default database name
const char* gDefault = "/default-data.itz";

/// write through database name
const char* gWt             = "/wt.itz";
/// cached database name
const char* gCached        = "/cached.itz";


/// directory structure node name defintion
const char* gNode = "/node";
/// directory structure user name defintion
const char* gUser = "/user/";
/// directory structure seat name defintion
const char* gSeat = "/seat/";


/// path prefix for local cached database: /Data/mnt_c/<appId>/ (<database_name>
const char* gLocalCachePath        = "/Data/mnt-c/%s";
/// path prefix for local write through database /Data/mnt_wt/<appId>/<database_name>
const char* gLocalWtPath           = "/Data/mnt-wt/%s";
/// path prefix for shared cached database: /Data/mnt_c/Shared/Group/<group_no>/<database_name>
const char* gSharedCachePath       = "/Data/mnt-c/%s/Shared_Group_%x";
/// path prefix for shared write through database: /Data/mnt_wt/Shared/Group/<group_no>/<database_name>
const char* gSharedWtPath          = "/Data/mnt-wt/%s/Shared_Group_%x";
/// path prefix for shared public cached database: /Data/mnt_c/Shared/Public//<database_name>
const char* gSharedPublicCachePath = "/Data/mnt-c/%s/Shared_Public";
/// path prefix for shared public write through database: /Data/mnt_wt/Shared/Public/<database_name>
const char* gSharedPublicWtPath    = "/Data/mnt-wt/%s/Shared_Public";

/// path prefix for local cached database: /Data/mnt_c/<appId>/ (<database_name>
const char* gLocalCachePathKey        = "/Data/mnt-c/%s%s";
/// path prefix for local write through database /Data/mnt_wt/<appId>/<database_name>
const char* gLocalWtPathKey           = "/Data/mnt-wt/%s%s";
/// path prefix for shared cached database: /Data/mnt_c/Shared/Group/<group_no>/<database_name>
const char* gSharedCachePathKey       = "/Data/mnt-c/%s/Shared_Group_%x%s";
/// path prefix for shared write through database: /Data/mnt_wt/Shared/Group/<group_no>/<database_name>
const char* gSharedWtPathKey          = "/Data/mnt-wt/%s/Shared_Group_%x%s";
/// path prefix for shared public cached database: /Data/mnt_c/Shared/Public//<database_name>
const char* gSharedPublicCachePathKey = "/Data/mnt-c/%s/Shared_Public%s";
/// path prefix for shared public write through database: /Data/mnt_wt/Shared/Public/<database_name>
const char* gSharedPublicWtPathKey    = "/Data/mnt-wt/%s/Shared_Public%s";

/// path prefix for local cached files: /Data/mnt_c/<appId>/<user>/<seat>/<resource>
const char* gLocalCacheFilePath        = "/Data/mnt-c/%s/user/%d/seat/%d/%s";


const char* gChangeSignal = "PersistenceResChange";
const char* gDeleteSignal = "PersistenceResDelete";
const char* gCreateSignal = "PersistenceResCreate";


char gNotifykey[DbKeyMaxLen] = {0};
unsigned int gNotifyLdbid  = 0;
unsigned int gNotifyUserNo = 0;
unsigned int gNotifySeatNo = 0;
pclNotifyStatus_e       gNotifyReason = 0;
PersNotifyRegPolicy_e   gNotifyPolicy = 0;


int gTimeoutMs = 5000;

int gDbusPendingRvalue = 0;


/// application id
char gAppId[MaxAppNameLen] = {0};


/// max key value data size [default 16kB]
int gMaxKeyValDataSize = defaultMaxKeyValDataSize;


unsigned int gPclInitialized = PCLnotInitialized;


DltContext gDLTContext;

int(* gChangeNotifyCallback)(pclNotification_s * notifyStruct);


