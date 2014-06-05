#ifndef PERSISTENCE_CLIENT_LIBRARY_BACKUP_FILELIST_H
#define PERSISTENCE_CLIENT_LIBRARY_BACKUP_FILELIST_H

/******************************************************************************
 * Project         Persistency
 * (c) copyright   2013
 * Company         XS Embedded GmbH
 *****************************************************************************/
/******************************************************************************
 * This Source Code Form is subject to the terms of the
 * Mozilla Public License, v. 2.0. If a  copy of the MPL was not distributed
 * with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
******************************************************************************/
 /**
 * @file           persistence_client_library_backup_filelist.h
 * @ingroup        Persistence client library
 * @author         Ingo Huerner
 * @brief          Header of the persistence client library backup file list
 * @see
 */


//#include "../include_protected/persistence_client_library_rc_table.h"
#include <persComRct.h>


/**
 * @brief Read the blacklist configuration file
 *
 * @param filename the filename and path to the configuration fiel
 *
 * @return 1 success, 0 error
 */
int readBlacklistConfigFile(const char* filename);


/**
 * @brief create the file
 *
 * @param path of the file to be created
 *
 * @return the handle to his file
 */
int pclCreateFile(const char* path);


/**
 * @brief create a backup copy of a file
 *
 * @param srcPath the path of the file
 * @param srcfd the file descriptor of the file
 * @param csumpath the path where to checksum will be stored
 * @param csumBuf the checksum string
 *
 * @return -1 on error or a positive value indicating number of bytes of the backup file created
 */
int pclCreateBackup(const char* srcPath, int srcfd, const char* csumPath, const char* csumBuf);


/**
 * @brief recover file form backup
 *
 * @param backupfd the file descriptor of the backup file
 * @param original the path of the file to be recovered
 *
 * @return 0 on success -1 on error
 */
int pclRecoverFromBackup(int backupFd, const char* original);


/**
 * @brief calculate crc32 checksum
 *
 * @param fd the file descriptor to create the checksum from
 * @param crc32sum the array to store the checksum
 *
 * @return -1 on error or 1 if succeeded
 */
int pclCalcCrc32Csum(int fd, char crc32sum[]);


/**
 * @brief verify file for consistency
 *
 * @param origPath the path of the file to verify
 * @param backupPath the path of the backup file
 * @param csumPath the path to the checksum file
 * @param openFlag the file open flags
 *
 * @return
 */
int pclVerifyConsistency(const char* origPath, const char* backupPath, const char* csumPath, int openFlags);


/**
 * @brief check if file needs a backup
 *
 * @param path the path of the file
 *
 * @return 1 if a backup will shall be created,
 *         0 if a backup shall be not created or -1 for an error
 */
inline int pclBackupNeeded(const char* path);


/**
 * @brief translate persistence permission into POSIX file open permissions
 *
 * @param permission the permission enumerator ::PersistencePermission_e
 *
 * @return the POSIX file permission will be returned of -1 in an error case.
 *         If an unknown PersistencePermission_e will be detected the
 *         default permission O_RDONLY will be returned
 */
int pclGetPosixPermission(PersistencePermission_e permission);


#endif /* PERS_BACKUP_BLACKLIST_H */
