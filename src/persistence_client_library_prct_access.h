#ifndef PERSISTENCE_CLIENT_LIBRARY_ACCESS_HELPER_H
#define PERSISTENCE_CLIENT_LIBRARY_ACCESS_HELPER_H

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
 * @file           persistence_client_library_prct_access.h
 * @ingroup        Persistence client library
 * @author         Ingo Huerner
 * @brief          Header of persistence resource configuration config
 *                 table access functions
 * @see
 */

#include "persistence_client_library_data_organization.h"

/**
 * @brief Create database search key and database location path
 *
 * @param dbContext the database context
 * @param resource_id the resource id
 * @param dbKey the array where the database key will be stored
 * @param dbPath the array where the database location path will be stored
 *
 * @return 1 if shared database and 0 if local database or PersistenceStoragePolicy_LastEntry
 *         when no valid database has been found
 */
int get_db_path_and_key(PersistenceInfo_s* dbContext, const char* resource_id, char dbKey[], char dbPath[]);



/**
 * @brief Create database search key and database location path
 *
 * @param dbContext the database context
 * @param resource_id the resource id
 * @param isFile identifier if this resource is a file (used for file/key creation if resource does not exist)
 * @param dbKey the array where the database key will be stored
 * @param dbPath the array where the database location path will be stored
 *
 * @return 0 or a negative value with one of the following errors: EPERS_NOKEYDATA or EPERS_NOPRCTABLE
 */
int get_db_context(PersistenceInfo_s* dbContext, const char* resource_id, unsigned int isFile, char dbKey[], char dbPath[]);



/**
 * @brief get the resource configuration table database by id
 *
 * @return i Handle to the database table or negative value if no valid database has been found
 */
int get_resource_cfg_table_by_idx(int i);


/**
 * @brief mark the resource configuration table as closed
 *
 * @param i the index
 */
void invalidate_resource_cfg_table(int i);



#endif /* PERSISTENCE_CLIENT_LIBRARY_ACCESS_HELPER_H */
