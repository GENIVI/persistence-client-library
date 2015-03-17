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

/// key handle structure definition
typedef struct _PersistenceKeyHandle_s
{
   /// logical database id
   unsigned int ldbid;
   /// User No
   unsigned int user_no;
   /// Seat No
   unsigned int seat_no;
   /// Resource ID
   char resource_id[PERS_DB_MAX_LENGTH_KEY_NAME];
} PersistenceKeyHandle_s;


/// file handle structure definition
typedef struct _PersistenceFileHandle_s
{
	/// access permission read/write
	PersistencePermission_e permission;
	/// flag to indicate if a backup has already been created
   int backupCreated;
   /// flag to indicate if file must be cached
   int cacheStatus;
   /// the user id
   int userId;
   /// path to the backup file
   char backupPath[PERS_ORG_MAX_LENGTH_PATH_FILENAME];
   /// path to the checksum file
   char csumPath[PERS_ORG_MAX_LENGTH_PATH_FILENAME];
   /// the file path
   char* filePath;
} PersistenceFileHandle_s;

/// open file descriptor handle array
extern int gOpenFdArray[MaxPersHandle];

/// handle array
extern int gOpenHandleArray[MaxPersHandle];

//----------------------------------------------------------------
//----------------------------------------------------------------


/**
 * @brief delete handle trees
 */
void deleteHandleTrees(void);


/**
 * @brief get persistence handle
 *
 * @return a new handle or 0 if an error occurred or EPERS_MAXHANDLE if max no of handles is reached
 */
int get_persistence_handle_idx();


/**
 * @brief close persistence handle
 *
 * @param handle to close
 */
void set_persistence_handle_close_idx(int handle);


/**
 * @brief close open key handles
 *
 */
void close_all_persistence_handle();

//----------------------------------------------------------------
//----------------------------------------------------------------

/**
 * @brief set data to the key handle
 *
 * @param idx the index
 * @param id the resource id
 * @param ldbid the logical database id
 * @param user_no the user identifier
 * @param seat_no the seat number
 *
 * @return a positive value (0 or greather) or -1 on error
 */
int set_key_handle_data(int idx, const char* id, unsigned int ldbid,  unsigned int user_no, unsigned int seat_no);


/**
 * @brief set data to the key handle
 *
 * @param idx the index
 * @param handleStruct the handle structure
 *
 * @return 0 on success, -1 on error
 */
int get_key_handle_data(int idx, PersistenceKeyHandle_s* handleStruct);


/**
 * @brief initialize the key handle array to defined values
 */
void init_key_handle_array();


/**
 * @brief set data to the key handle
 *
 * @param idx the index
 *
 */
void clear_key_handle_array(int idx);

//----------------------------------------------------------------
//----------------------------------------------------------------

/**
 * @brief set data to the key handle
 *
 * @param idx the index
 * @param permission the permission (read/write, read only, write only)
 * @param backupCreated 0 is a backup has not been created or 1 if a backup has been created
 * @param backup path to the backup file
 * @param csumPath the path to the checksum file
 * @param filePath the path to the file
 *
 */
int set_file_handle_data(int idx, PersistencePermission_e permission, const char* backup, const char* csumPath,  char* filePath);


/**
 * @brief remove file handle from file tree
 *
 * @param idx the index
 */
int remove_file_handle_data(int idx);

/**
 * @brief set data to the key handle
 *
 * @param idx the index
 *
 * @return the file permission
 */
int get_file_permission(int idx);


/**
 * @brief set data to the key handle
 * @attention "N index check will be done"
 *
 * @param idx the index
 *
 * @return the path to the backup
 */
char* get_file_backup_path(int idx);


/**
 * @brief get the file checksum path
 * @attention "N index check will be done"
 *
 * @param idx the index
 *
 * @return the checksum path
 */
char* get_file_checksum_path(int idx);


/**
 * @brief set the file backup status of the file
 * @attention "No index check will be done"
 *
 * @param idx the index
 * @param status the backup status, 0 backup has been created,
 *                                  1 backup has not been created
 */
void set_file_backup_status(int idx, int status);


/**
 * @brief get the backup status of the file
 * @attention "No index check will be done"
 *
 * @param idx the index
 *
 * @return 0 if no backup has been created,
 *         1 if backup has been created
 */
int get_file_backup_status(int idx);


/**
 * @brief set the file cache status
 * @attention "No index check will be done"
 *
 * @param idx the index
 * @param status the cache status, 0 file must not be cached,
 *                                 1 file must be cached
 */
void set_file_cache_status(int idx, int status);


/**
 * @brief get the cache status of the file
 * @attention "No index check will be done"
 *
 * @param idx the index
 *
 * @return 0 if file must not be cached,
 *         1 if file must be cached
 */
int get_file_cache_status(int idx);


/**
 * @brief set the user id
 * @attention "No index check will be done"
 *
 * @param idx the index
 * @param userID the user id
 */
void set_file_user_id(int idx, int userID);


/**
 * @brief get the user id of the file
 * @attention "No index check will be done"
 *
 * @param idx the index
 *
 * @return the user id
 */
int get_file_user_id(int idx);
//----------------------------------------------------------------
//----------------------------------------------------------------

/**
 * @brief set data to the key handle
 *
 * @param idx the index
 * @param permission the permission (read/write, read only, write only)
 * @param backupCreated 0 is a backup has not been created or 1 if a backup has been created
 * @param backup path to the backup file
 * @param csumPath the path to the checksum file
 * @param filePath the path to the file
 *
 */
int set_ossfile_handle_data(int idx, PersistencePermission_e permission, int backupCreated,
		                   const char* backup, const char* csumPath,  char* filePath);


/**
 * @brief set data to the key handle
 *
 * @param idx the index
 *
 * @return the file permission
 */
int get_ossfile_permission(int idx);


/**
 * @brief get file backup path
 * @attention "No index check will be done"
 *
 * @param idx the index
 *
 * @return the path to the backup
 */
char* get_ossfile_backup_path(int idx);


/**
 * @brief get file path
 * @attention "No index check will be done"
 *
 * @param idx the index
 *
 * @return the path to the backup
 */
char* get_ossfile_file_path(int idx);


/**
 * @brief get the file checksum path
 * @attention "No index check will be done"
 *
 * @param idx the index
 *
 * @return the checksum path
 */
char* get_ossfile_checksum_path(int idx);

/**
 * @brief get the file checksum path
 * @attention "No index check will be done"
 *
 * @param idx the index
 * @param file pointer to the file and path
 *
 * @return the checksum path
 */
void set_ossfile_file_path(int idx, char* file);

/**
 * @brief set the file backup status of the file
 * @attention "No index check will be done"
 *
 * @param idx the index
 * @param status the backup status, 0 backup has been created,
 *                                  1 backup has not been created
 */
void set_ossfile_backup_status(int idx, int status);


/**
 * @brief get the backup status of the file
 * @attention "No index check will be done"
 *
 * @param idx the index
 *
 * @return 0 if no backup has been created,
 *         1 if backup has been created
 */
int get_ossfile_backup_status(int idx);


/**
 * @brief remove file handle from ass file tree
 *
 * @param idx the index
 */
int remove_ossfile_handle_data(int idx);

#endif /* PERSISTENCY_CLIENT_LIBRARY_HANDLE_H */

