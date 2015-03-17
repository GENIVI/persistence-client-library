#ifndef PERSISTENCE_CLIENT_LIBRARY_DB_ACCESS_H
#define PERSISTENCE_CLIENT_LIBRARY_DB_ACCESS_H

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
 * @file           persistence_client_library_db_access.h
 * @ingroup        Persistence client library
 * @author         Ingo Huerner
 * @brief          Header of the persistence client library database access.
 *                 Library provides an API to access persistent data
 * @see            
 */

#ifdef __cplusplus
extern "C" {
#endif

#define  PERSIST_DATA_ACCESS_INTERFACE_VERSION   (0x05000000U)


#include "persistence_client_library_data_organization.h"
#include "../include/persistence_client_library_key.h"

#include <persComRct.h>



/// default database definitions
typedef enum PersistenceDB_e_
{
	/// configurable default database
	PersistenceDB_confdefault = PersistencePolicy_LastEntry,
	/// default database
	PersistenceDB_default,

	PersistenceDB_LastEntry

} PersistenceDefaultDB_e;


/**
 * @brief get the raw key without prefixed '/node/', '/user/3/' etc
 *
 * @param key the ptr. to the key which should be stripped
 *
 * @return the pointer to the stripped 'raw key'
 */
char* pers_get_raw_key(char *key);



/**
 * @brief open the default value database specified by the 'DefaultType'
 *
 * @param dbPath path to the directory were the databases are included in.
 * @param DefaultType the default type
 *
 * @return >= 0 for valid handler; if an error occured the following error code:
 *   EPERS_COMMON
 */
int pers_db_open_default(const char* dbPath, PersDefaultType_e DefaultType);



/**
 * @brief tries to get default values for a key from the configurable and factory default databases.
 *
 * @param dbPath the path to the directory where the default databases are in 
 * @param key the database key
 * @param info the persistence context information
 * @param buffer the buffer holding the data
 * @param buffer_size the size of the buffer
 * @param job the info to specify what to do. Get Data or the DataSize.
 *
 * @return the number of bytes read or the size of the key (depends on parameter 'job').
           negative value if an error occured and the following error code:
 *         EPERS_NOKEY
 */
int pers_get_defaults(char* dbPath, char* key, PersistenceInfo_s* info, unsigned char* buffer, unsigned int buffer_size, PersGetDefault_e job);



/**
 * @brief write data to a key
 *
 * @param dbPath the path to the database where the key is in 
 * @param key the database key
 * @param resource_id the resource identifier
 * @param info persistence information
 * @param buffer the buffer holding the data
 * @param buffer_size the size of the buffer
 *
 * @return the number of bytes written or a negative value if an error occured with the following error codes:
 *   EPERS_SETDTAFAILED  EPERS_NOPRCTABLE  EPERS_NOKEYDATA  EPERS_NOKEY
 */
int persistence_set_data(char* dbPath, char* key, const char* resource_id, PersistenceInfo_s* info, unsigned char* buffer, unsigned int buffer_size);



/**
 * @brief get data of a key
 *
 * @param dbPath the path to the database where the key is in
 * @param key the database key
 * @param resourceID the resource id
 * @param info persistence information
 * @param buffer the buffer holding the data
 * @param buffer_size the size of the buffer
 *
 * @return the number of bytes read or a negative value if an error occured with the following error codes:
 *  EPERS_NOPRCTABLE  EPERS_NOKEYDATA  EPERS_NOKEY
 */
int persistence_get_data(char* dbPath, char* key, const char* resourceID, PersistenceInfo_s* info, unsigned char* buffer, unsigned int buffer_size);



/**
 * @brief get the size of the data from a given key
 *
 * @param dbPath the path to the database where the key is in
 * @param key the database key
 * @param resourceID the resource id
 * @param info persistence information
 *
 * @return size of data in bytes read from the key or on error a negative value with the following error codes:
 *  EPERS_NOPRCTABLE or EPERS_NOKEY
 */
int persistence_get_data_size(char* dbPath, char* key, const char* resourceID, PersistenceInfo_s* info);



/**
 * @brief delete data
 *
 * @param dbPath the path to the database where the key is in
 * @param key the database key to register on
 * @param resource_id the resource identifier
 * @param info persistence information
 *
 * @return 0 if deletion was successfull;
 *         or an error code: EPERS_DB_KEY_SIZE, EPERS_NOPRCTABLE, EPERS_DB_ERROR_INTERNAL or EPERS_NOPLUGINFUNCT
 */
int persistence_delete_data(char* dbPath, char* key, const char* resource_id, PersistenceInfo_s* info);



/**
 * @brief close all databases
 */
void database_close_all();



/**
 * @brief register or unregister for change notifications of a key
 *
 * @param resource_id the database resource_id to register on
 * @param ldbid logical database ID of the resource to monitor
 * @param user_no  the user ID; user_no=0 can not be used as user-ID beacause '0' is defined as System/node
 * @param seat_no  the seat number
 * @param callback the function callback to be called
 * @param regPolicy ::Notify_register to register; ::Notify_unregister to unregister
 *
 * @return 0 of registration was successful; -1 if registration fails
 */
int persistence_notify_on_change(const char* resource_id, const char* dbKey, unsigned int ldbid, unsigned int user_no, unsigned int seat_no,
                                     pclChangeNotifyCallback_t callback, PersNotifyRegPolicy_e regPolicy);



/**
 * @brief send a notification signal
 *
 * @param key the database key to register on
 * @param context the database context
 * @param reason the reason of the signal, values see pclNotifyStatus_e.
 *
 * @return 0 of registration was successful; -1 if registration failes
 */
int pers_send_Notification_Signal(const char* key, PersistenceDbContext_s* context, pclNotifyStatus_e reason);


/**
 * @brief close all open persistence resource configuration tables
 */
void pers_rct_close_all();


/**
 * @brief delete notification tree
 */
void deleteNotifyTree(void);


#ifdef __cplusplus
}
#endif

#endif /* PERSISTENCY_CLIENT_LIBRARY_DB_ACCESS_H */
