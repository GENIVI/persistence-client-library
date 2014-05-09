#ifndef PERSISTENCE_CLIENT_LIBRARY_HANDLE_H
#define PERSISTENCE_CLIENT_LIBRARY_HANDLE_H

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
 * @file           persistence_client_library_handle.h
 * @ingroup        Persistence client library
 * @author         Ingo Huerner
 * @brief          Header of the persistence client library handle.
 *                 Library provides an API to access persistent data
 * @see            
 */

#include "persistence_client_library_data_organization.h"

#include <persComRct.h>

/// key handle structure definition
typedef struct _PersistenceKeyHandle_s
{
   PersistenceInfo_s info;          /// persistence info
   char dbPath[DbPathMaxLen];       /// path to the database
   char dbKey[DbKeyMaxLen];         /// database key
   char resourceID[DbResIDMaxLen];  /// resourceID
} PersistenceKeyHandle_s;


/// file handle structure definition
typedef struct _PersistenceFileHandle_s
{
   PersistencePermission_e permission;    /// access permission read/write
   int backupCreated;                     /// flag to indicate if a backup has already been created
   char backupPath[DbPathMaxLen];         /// path to the backup file
   char csumPath[DbPathMaxLen];           /// path to the checksum file
   char* filePath;                        /// the path
} PersistenceFileHandle_s;




/// persistence key handle array
extern PersistenceKeyHandle_s gKeyHandleArray[MaxPersHandle];


/// persistence file handle array
extern PersistenceFileHandle_s gFileHandleArray[MaxPersHandle];

/// persistence handle array for OSS and third party handles
extern PersistenceFileHandle_s gOssHandleArray[MaxPersHandle];


/// open file descriptor handle array
extern int gOpenFdArray[MaxPersHandle];

/// handle array
extern int gOpenHandleArray[MaxPersHandle];

/**
 * @brief get persistence handle
 *
 * @return a new handle or 0 if an error occured or EPERS_MAXHANDLE if max no of handles is reached
 */
int get_persistence_handle_idx();


/**
 * @brief close persistence handle
 *
 * @param the handle to close
 */
void set_persistence_handle_close_idx(int handle);


/**
 * @brief close open key handles
 *
 */
void close_all_persistence_handle();


#endif /* PERSISTENCY_CLIENT_LIBRARY_HANDLE_H */

