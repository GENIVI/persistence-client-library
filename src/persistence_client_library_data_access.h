#ifndef PERSISTENCE_CLIENT_LIBRARY_DATA_ACCESS_H
#define PERSISTENCE_CLIENT_LIBRARY_DATA_ACCESS_H

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
 * @file           persistence_client_library_data_access.h
 * @ingroup        Persistence client library
 * @author         Ingo Huerner
 * @brief          Header of the persistence client library data access.
 *                 Library provides an API to access persistent data
 * @see            
 */

#define  PERSIST_DATA_ACCESS_INTERFACE_VERSION   (0x02000000U)


#include "persistence_client_library.h"



/**
 * @brief write data to a key
 *
 * @param dbPath the path to the database where the key is in 
 * @param key the database key
 * @param storage the storage identifier (local, shared or custom)
 *        (use dbShared for shared key or dbLocal if the key is local)
 *
 * @return the number of bytes written or a negative value if an error occured with the following error codes:
 *   EPERS_SETDTAFAILED  EPERS_NOPRCTABLE  EPERS_NOKEYDATA  EPERS_NOKEY
 */
int persistence_set_data(char* dbPath, char* key, PersistenceStorage_e storage, PersistencePolicy_e policy,
                         unsigned char* buffer, unsigned long buffer_size);



/**
 * @brief get data of a key
 *
 * @param dbPath the path to the database where the key is in
 * @param key the database key
 * @param storage the storage identifier (local, shared or custom)
 *
 * @return the number of bytes read or a negative value if an error occured with the following error codes:
 *  EPERS_NOPRCTABLE  EPERS_NOKEYDATA  EPERS_NOKEY
 */
int persistence_get_data(char* dbPath, char* key, PersistenceStorage_e storage, PersistencePolicy_e policy,
                         unsigned char* buffer, unsigned long buffer_size);



/**
 * @brief get the size of the data from a given key
 *
 * @param dbPath the path to the database where the key is in
 * @param key the database key
 * @param storage the storage identifier (local, shared or custom)
 *
 * @return size of data in bytes read from the key or on error a negative value with the following error codes:
 *  EPERS_NOPRCTABLE or EPERS_NOKEY
 */
int persistence_get_data_size(char* dbPath, char* key, PersistenceStorage_e storage, PersistencePolicy_e policy);



/**
 * @brief register for change notifications of a key
 *
 * @param dbPath the path to the database where the key is in
 * @param key the database key to register on
 *
 * @return 0 of registration was successfull; -1 if registration failes
 */
int persistence_reg_notify_on_change(char* dbPath, char* key);


/**
 * @brief delete data
 */
int persistence_delete_data(char* dbPath, char* dbKey, PersistenceStorage_e storage, PersistencePolicy_e policy);

/**
 * @brief close the database for the given storage type
 *
 * @param storage the storage type of the database to close
 */
void database_close(PersistenceStorage_e storage, PersistencePolicy_e policy);




//---------------------------------------------------------------------------------------------
// C U R S O R    F U N C T I O N S
//---------------------------------------------------------------------------------------------

/**
 * @brief create a cursor to a DB ; if success, the cursor points to (-1)
 * to access the first entry in DB, call persistence_db_cursor_next
 *
 * @param dbPath[in] absolute path to the database
 * @param storage[in] the storage identifier (local, shared or custom)
 *
 * @return handler to the DB (to be used in successive calls) or error code (< 0)
 */
int persistence_db_cursor_create(char* dbPath, PersistenceStorage_e storage, PersistencePolicy_e policy);

/**
 * @brief move cursor to the next position
 *
 * @param handlerDB[in] handler to DB (obtained with persistence_db_cursor_create())
 *
 * @return 0 for success, negative value in case of error (check against EPERS_LAST_ENTRY_IN_DB)
 */
int persistence_db_cursor_next(int handlerDB);

/**
 * @brief get the name of the key pointed by the cursor associated with the database
 *
 * @param handlerDB[in] handler to DB (obtained with persistence_db_cursor_create())
 * @param bufKeyName_out[out] buffer where to pass the name of the key
 * @param bufSize[out] size of bufKeyName_out
 *
 * @return read size (if >= 0), error other way
 */
int persistence_db_cursor_get_key(int handlerDB, char * bufKeyName_out, int bufSize) ;

/**
 * @brief get the data of the key pointed by the cursor associated with the database
 *
 * @param handlerDB[in] handler to DB (obtained with persistence_db_cursor_create())
 * @param bufKeyData_out[out] buffer where to pass the data of the key
 * @param bufSize[out] size of bufKeyData_out
 *
 * @return read size (if >= 0), error other way
 */
int persistence_db_cursor_get_data(int handlerDB, char * bufData_out, int bufSize) ;

/**
 * @brief get the data size of the key pointed by the cursor associated with the database
 *
 * @param handlerDB[in] handler to DB (obtained with persistence_db_cursor_create())
 *
 * @return positive value for data size, negative value for error
 */
int persistence_db_cursor_get_data_size(int handlerDB) ;


/**
 * @brief remove the cursor
 *
 * @param handlerDB[in] handler to DB (obtained with persistence_db_cursor_create())
 *
 * @return 0 for success, negative value in case of error
 */
int persistence_db_cursor_destroy(int handlerDB) ;


#endif /* PERSISTENCY_CLIENT_LIBRARY_DATA_ACCESS_H */
