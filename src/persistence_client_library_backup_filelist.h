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


/**
 * @brief Read the blacklist configuration file
 *
 * @param filename the filename and path to the configuration fiel
 *
 * @return 1 success, 0 error
 */
int readBlacklistConfigFile(const char* filename);


int need_backup_key(unsigned int key);


#endif /* PERS_BACKUP_BLACKLIST_H */
