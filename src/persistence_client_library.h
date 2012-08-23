#ifndef PERSISTENCE_CLIENT_LIBRARY_H
#define PERSISTENCE_CLIENT_LIBRARY_H

/******************************************************************************
 * Project         Persistency
 * (c) copyright   2012
 * Company         XS Embedded GmbH
 *****************************************************************************/
/******************************************************************************
   Permission is hereby granted, free of charge, to any person obtaining
   a copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included
   in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
   DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
   TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
******************************************************************************/
 /**
 * @file           persistence_client_library.h
 * @ingroup        Persistence client library
 * @author         Ingo Huerner
 * @brief          Header of the persistence client library.
 *                 Library provides an API to access persistent data
 * @see            
 */

#include "persistence_client_library_lc_interface.h"
#include <gvdb-reader.h>
#include "crc32.h"

#include <string.h>
#include <stdio.h>


/// constant definitions
enum _PersistenceConstantDef
{
   resIsNoFile          = 0,        /// flag to identify that resource a not file
   resIsFile            = 1,        /// flag to identify that resource a file
   accessNoLock         = 1,        /// flag to indicate that access is not locked

   FileClosed           = 0,
   FileOpen             = 1,

   NsmShutdownNormal       = 1,     /// lifecycle shutdown normal
   NsmErrorStatus_OK       = 1,
   NsmErrorStatus_Fail     = -1,

   PasMsg_Block            = 1,     /// persistence administration service block access
   PasMsg_WriteBack        = 2,     /// persistence administration service write_back
   PasMsg_Unblock          = 4,     /// persistence administration service unblock access
   PasErrorStatus_RespPend = 88,    /// persistence administration service msg return status
   PasErrorStatus_OK       = 100,   /// persistence administration service msg return status
   PasErrorStatus_FAIL     = -1,    /// persistence administration service msg return status

   CustLibMaxLen = 128,             /// max length of the custom library name and path
   dbKeyMaxLen   = 128,             /// max database key length
   dbPathMaxLen  = 128,             /// max database path length
   maxAppNameLen = 128,             /// max application name
   maxPersHandle = 256,             /// max number of parallel open persistence handles

   defaultMaxKeyValDataSize = 16384 /// default limit the key-value data size to 16kB
};

/// resource configuration table name
static const char* gResTableCfg = "/resource-table-cfg.gvdb";

/// shared cached default database
//static const char* gSharedCachedDefault = "cached-default.dconf";
/// shared cached database
static const char* gSharedCached        = "/cached.dconf";
/// shared write through default database
//static const char* gSharedWtDefault     = "wt-default.dconf";
/// shared write through database
static const char* gSharedWt            = "/wt.dconf";

/// local cached default database
//static const char* gLocalCachedDefault  = "cached-default.gvdb";
/// local cached default database
static const char* gLocalCached         = "/cached.gvdb";
/// local write through default database
//static const char* gLocalWtDefault      = "wt-default.gvdb";
/// local write through default database
static const char* gLocalWt             = "/wt.gvdb";


/// directory structure node name defintion
static const char* gNode = "/Node";
/// directory structure user name defintion
static const char* gUser = "/User/";
/// directory structure seat name defintion
static const char* gSeat = "/Seat/";


/// path prefic for local cached database: /Data/mnt_c/<appId>/<database_name>
static const char* gLocalCachePath        = "/Data/mnt-c/%s%s";
/// path prefic for local write through database /Data/mnt_wt/<appId>/<database_name>
static const char* gLocalWtPath           = "/Data/mnt-wt/%s%s";
/// path prefic for shared cached database: /Data/mnt_c/Shared/Group/<group_no>/<database_name>
static const char* gSharedCachePath       = "/Data/mnt-c/shared/group/%x%s";
/// path prefic for shared write through database: /Data/mnt_wt/Shared/Group/<group_no>/<database_name>
static const char* gSharedWtPath          = "/Data/mnt-wt/shared/group/%x%s";
/// path prefic for shared public cached database: /Data/mnt_c/Shared/Public//<database_name>
static const char* gSharedPublicCachePath = "/Data/mnt-c/shared/public%s";
/// path prefic for shared public write through database: /Data/mnt_wt/Shared/Public/<database_name>
static const char* gSharedPublicWtPath    = "/Data/mnt-wt/shared/public%s";


/// application id
char gAppId[maxAppNameLen];


/** enumerator used to identify the policy to manage the data */
typedef enum _PersistencePolicy_e
{
   PersistencePolicy_wc    = 0,  /**< the data is managed write cached */
   PersistencePolicy_wt    = 1,  /**< the data is managed write through */
   PersistencePolicy_na    = 2,  /**< the data is not applicable */

   /** insert new entries here ... */
   PersistencePolicy_LastEntry         /**< last entry */

} PersistencePolicy_e;


/** enumerator used to identify the persistence storage to manage the data */
typedef enum _PersistenceStorage_e
{
   PersistenceStorage_local    = 0,  /**< the data is managed local: gvdb */
   PersistenceStorage_shared   = 1,  /**< the data is managed shared: dconf */
   PersistenceStorage_custom   = 2,  /**< the data is managed over custom client implementation */

   /** insert new entries here ... */
   PersistenceStoragePolicy_LastEntry         /**< last entry */

} PersistenceStorage_e;


/** structure used to manage the persistence configuration for a key */
typedef struct _PersistenceConfigurationKey_s
{
   PersistencePolicy_e     policy;           /**< policy  */
   PersistenceStorage_e    storage;          /**< definition of storage to use */
   unsigned int            permission;       /**< access right, corresponds to UNIX */
   unsigned int            max_size;         /**< max size expected for the key */
   char *                  reponsible;       /**< name of responsible application */
   char *                  custom_name;      /**< name of the customer plugin */
} PersistenceConfigurationKey_s;


typedef enum _PersistenceRCT_e
{
   PersistenceRCT_local         = 0,
   PersistenceRCT_shared_public = 1,
   PersistenceRCT_shared_group  = 2,

   PersistenceRCT_LastEntry                // last Entry

} PersistenceRCT_e;


/// max key value data size
static int gMaxKeyValDataSize = defaultMaxKeyValDataSize;






#endif /* PERSISTENCY_CLIENT_LIBRARY_H */

