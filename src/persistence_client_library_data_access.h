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

#include "persistence_client_library.h"
#ifdef USE_GVDB
#include "gvdb-builder.h"
#endif

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
int persistence_set_data(char* dbPath, char* key, PersistenceStorage_e storage, unsigned char* buffer, unsigned long buffer_size);



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
int persistence_get_data(char* dbPath, char* key, PersistenceStorage_e storage, unsigned char* buffer, unsigned long buffer_size);



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
int persistence_get_data_size(char* dbPath, char* key, PersistenceStorage_e storage);



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
int persistence_delete_data(char* dbPath, char* dbKey, PersistenceStorage_e storePolicy);

/**
 * @brief read data from a key
 *
 * @param database pointer to the database
 * @param key the database key to read data from
 * @param buffer the data
 * @param buffer_size the size of the buffer in bytes
 *
 * @return size of data in bytes read from the key or a negative value on error with the following error codes:
 *  EPERS_NOKEYDATA   EPERS_NOKEY
 */
//int get_value_from_table(const char* dbPath, char* key, unsigned char* buffer, unsigned long buffer_size);
//int get_value_from_table(GvdbTable* database, char* key, unsigned char* buffer, unsigned long buffer_size);



/**
 * @brief write data to a key
 *
 * @param database pointer to the database
 * @param key the database key to write data
 * @param buffer the data
 * @param buffer_size the size of the buffer in bytes
 *
 * @return size of data in bytes written to the key
 */
//int set_value_to_table(const char* dbPath, char* key, unsigned char* buffer, unsigned long buffer_size);
//int set_value_to_table(GHashTable* database, char* key, unsigned char* buffer, unsigned long buffer_size);



/**
 * @brief get the size of the data from a key
 *
 * @param database pointer to the database
 * @param key the database key to get the size form
 *
 * @return size of data or a negative value on error with the following errors codes:
 * EPERS_NOKEY
 */
//int get_size_from_table(const char* dbPath, char* key);
//int get_size_from_table(GvdbTable* database, char* key);




#endif /* PERSISTENCY_CLIENT_LIBRARY_DATA_ACCESS_H */
