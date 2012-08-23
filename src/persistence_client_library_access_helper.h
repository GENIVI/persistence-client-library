#ifndef PERSISTENCE_CLIENT_LIBRARY_ACCESS_HELPER_H
#define PERSISTENCE_CLIENT_LIBRARY_ACCESS_HELPER_H

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
 * @file           persistence_client_library_access_helper.h
 * @ingroup        Persistence client library
 * @author         Ingo Huerner
 * @brief          Header of the persistence client library access helper.
 * @see
 */

#include "persistence_client_library.h"


/**
 * @brief Create database search key and database location path
 *
 * @param ldbid logical database id
 * @param resource_id the resource id
 * @param user_no user identification
 * @param seat_no seat identifier
 * @param isFile identifier if this resource is a file
 * @param dbKey the array where the database key will be stored
 * @param dbPath the array where the database location path will be stored
 * @param cached_resource flag to identify if the resource is cached (value 1)or write through (value 0)
 *
 * @return -1 if error : 1 if shared database and 0 if local database
 */
int get_db_path_and_key(unsigned char ldbid, char* resource_id, unsigned char user_no, unsigned char seat_no,
                        unsigned int isFile, char dbKey[], char dbPath[], unsigned char cached_resource);



/**
 * Create database search key and database location path
 *
 * @param ldbid logical database id
 * @param resource_id the resource id
 * @param user_no user identification
 * @param seat_no seat identifier
 * @param isFile identifier if this resource is a file
 * @param dbKey the array where the database key will be stored
 * @param dbPath the array where the database location path will be stored
 * @param cached_resource flag to identify if the resource is cached (value 1)or write through (value 0)
 *
 * @return -1 if error : or PersistenceStorage_e
 */
int get_db_context(unsigned char ldbid, char* resource_id, unsigned char user_no, unsigned char seat_no,
                                    unsigned int isFile, char dbKey[], char dbPath[]);



/**
 * @brief get the resource configuration table gvbd database by id
 *
 * @return pointer to the gvdb database table
 */
GvdbTable* get_resource_cfg_table_by_idx(int i);


/**
 * @brief serialize data to store to database
 */
int serialize_data(PersistenceConfigurationKey_s pc, char* buffer);


/**
 * @brief deserialize data read from database
 */
int de_serialize_data(char* buffer, PersistenceConfigurationKey_s* pc);


/**
 * @brief free allocated data of a persistence configuration key
 *
 * @param pc the configuration key
 */
int free_pers_conf_key(PersistenceConfigurationKey_s* pc);



#endif /* PERSISTENCE_CLIENT_LIBRARY_ACCESS_HELPER_H */
