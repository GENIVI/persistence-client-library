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

const char* gResTableCfg = "/resource-table-cfg.itz";


/// shared cached default database
const char* gSharedCachedDefault = "/cached-default.itz";
/// shared cached database
const char* gSharedCached        = "/cached.itz";
/// shared write through default database
const char* gSharedWtDefault     = "/wt-default.itz";
/// shared write through database
const char* gSharedWt            = "/wt.itz";


/// local cached default database
const char* gLocalCachedDefault  = "cached-default.itz";
/// local cached default database
const char* gLocalCached         = "/cached.itz";
/// local write through default database
const char* gLocalWtDefault      = "wt-default.itz";
/// local write through default database
const char* gLocalWt             = "/wt.itz";



/// directory structure node name defintion
const char* gNode = "/node";
/// directory structure user name defintion
const char* gUser = "/user/";
/// directory structure seat name defintion
const char* gSeat = "/seat/";


/// path prefic for local cached database: /Data/mnt_c/<appId>/<database_name>
const char* gLocalCachePath        = "/Data/mnt-c/%s%s";
/// path prefic for local write through database /Data/mnt_wt/<appId>/<database_name>
const char* gLocalWtPath           = "/Data/mnt-wt/%s%s";
/// path prefic for shared cached database: /Data/mnt_c/Shared/Group/<group_no>/<database_name>
const char* gSharedCachePath       = "/Data/mnt-c/%s/Shared_Group_%x%s";
/// path prefic for shared write through database: /Data/mnt_wt/Shared/Group/<group_no>/<database_name>
const char* gSharedWtPath          = "/Data/mnt-wt/%s/Shared_Group_%x%s";

/// path prefic for shared public cached database: /Data/mnt_c/Shared/Public//<database_name>
const char* gSharedPublicCachePath = "/Data/mnt-c/%s/Shared_Public%s";

/// path prefic for shared public write through database: /Data/mnt_wt/Shared/Public/<database_name>
const char* gSharedPublicWtPath    = "/Data/mnt-wt/%s/Shared_Public%s";


/// application id
char gAppId[MaxAppNameLen];

/// max key value data size [default 16kB]
int gMaxKeyValDataSize = defaultMaxKeyValDataSize;





