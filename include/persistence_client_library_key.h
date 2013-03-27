#ifndef PERSISTENCE_CLIENT_LIBRARY_KEY_H
#define PERSISTENCE_CLIENT_LIBRARY_KEY_H

/******************************************************************************
 * Project         Persistency
 * (c) copyright   2011
 * Company         XS Embedded GmbH
 *****************************************************************************/
/******************************************************************************
 * This Source Code Form is subject to the terms of the
 * Mozilla Public License, v. 2.0. If a  copy of the MPL was not distributed
 * with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
******************************************************************************/
 /**
 * @file           persistence_client_library_key.h
 * @ingroup        Persistence client library
 * @author         Ingo Huerner (XSe) / Guy Sagnes (Continental)
 * @brief          Header of the persistence client library.
 *                 Library provides an API to access persistent data
 * @see            
 */
/** \ingroup SSW_PERS */
/** \defgroup SSW_PERS_KEYVALUE Client: Key-value access
 *  \{
 */
/** \defgroup SSW_PERS_KEYVALUE_INTERFACE API document
 *  \{
 */

#ifdef __cplusplus
extern "C" {
#endif


#define 	PERSIST_KEYVALUEAPI_INTERFACE_VERSION   (0x03000000U)

/**
* status returned in notification structure
*/
typedef enum _PersistenceNotifyStatus_e
{
   pclNotifyStatus_no_changed = 0,
   pclNotifyStatus_created,
   pclNotifyStatus_changed,
   pclNotifyStatus_deleted,
   /* insert new_ entries here .. */
   pclNotifyStatus_lastEntry
} PersistenceNotifyStatus_e;


/**
* structure to return in case of notification
*/
typedef struct _PersistenceNotification_s
{
   PersistenceNotifyStatus_e pclKeyNotify_Status;
   unsigned int ldbid;
   const char * resource_id;
   unsigned int user_no;
   unsigned int seat_no;
} PersistenceNotification_s;


typedef int(* changeNotifyCallback_t)(PersistenceNotification_s * notifyStruct);

/**
 * @brief delete persistent data
 *
 * @param ldbid logical database ID
 * @param resource_id the resource ID
 * @param user_no  the user ID; user_no=0 can not be used as user-ID beacause ‘0’ is defined as System/node
 * @param seat_no  the seat number
 *
 * @return positive value: success; On error a negative value will be returned with th follwoing error codes:
 * EPERS_LOCKFS
 */
int pclKeyDelete(unsigned int ldbid, const char* resource_id, unsigned int user_no, unsigned int seat_no);



/**
 * @brief gets the size of persistent data in bytes
 *
 * @param ldbid logical database ID
 * @param resource_id the resource ID
 * @param user_no  the user ID; user_no=0 can not be used as user-ID beacause ‘0’ is defined as System/node
 * @param seat_no  the seat number
 *
 * @return positive value: the size; On error a negative value will be returned with th follwoing error codes:
 * EPERS_LOCKFS, EPERS_BADPOL, EPERS_NOKEY, EPERS_NOKEYDATA or EPERS_NOPRCTABLE
 */
int pclKeyGetSize(unsigned int ldbid, const char* resource_id, unsigned int user_no, unsigned int seat_no);

/**
 * @brief close the access to a key-value identified by key handle
 *
 * @param key_handle key value handle return by key_handle_open()
 *
 * @return positive value: success; On error a negative value will be returned with th follwoing error codes:
 * EPERS_LOCKFS
 */
int pclKeyHandleClose(int key_handle);



/**
 * @brief gets the size of persistent data in bytes identified by key handle
 *
 * @param key_handle key value handle return by key_handle_open()
 *
 * @return positive value: the size; On error a negative value will be returned with th follwoing error codes:
 */
int pclKeyHandleGetSize(int key_handle);



/**
 * @brief open a key-value
 *
 * @param ldbid logical database ID
 * @param resource_id the resource ID
 * @param user_no  the user ID; user_no=0 can not be used as user-ID beacause ‘0’ is defined as System/node
 * @param seat_no  the seat number
 *
 * @return positive value: the key handle to access the value;
 * On error a negative value will be returned with th follwoing error codes:
 */
int pclKeyHandleOpen(unsigned int ldbid, const char* resource_id, unsigned int user_no, unsigned int seat_no);



/**
 * @brief reads persistent data identified by key handle
 *
 * @param key_handle key value handle return by key_handle_open()
 * @param buffer the buffer for persistent data
 * @param buffer_size size of buffer for reading
 *
 * @return positive value: the bytes read; On error a negative value will be returned with th follwoing error codes:
 *
 */
int pclKeyHandleReadData(int key_handle, unsigned char* buffer, int buffer_size);



/**
 * @brief register a change notification for persistent data
 *
 * @param key_handle key value handle return by key_handle_open()
 * @param callback notification callback
 *
 * @return positive value: registration OK; On error a negative value will be returned with th follwoing error codes:
 */
int pclKeyHandleRegisterNotifyOnChange(int key_handle, changeNotifyCallback_t callback);



/**
 * @brief writes persistent data identified by key handle
 *
 * @param key_handle key value handle return by key_handle_open()
 * @param buffer the buffer containing the persistent data to write
 * @param buffer_size the number of bytes to write (default max size is set to 16kB)
 *                    use environment variable PERS_MAX_KEY_VAL_DATA_SIZE to modify default size in bytes
 *
 * @return positive value: the bytes written; On error a negative value will be returned with th follwoing error codes:
 */
int pclKeyHandleWriteData(int key_handle, unsigned char* buffer, int buffer_size);



/**
 * @brief reads persistent data identified by ldbid and resource_id
 *
 * @param ldbid logical database ID
 * @param resource_id the resource ID
 * @param user_no  the user ID; user_no=0 can not be used as user-ID beacause ‘0’ is defined as System/node
 * @param seat_no  the seat number
 * @param buffer the buffer to read the persistent data
 * @param buffer_size size of buffer for reading
 *
 * @return positive value: the bytes read; On error a negative value will be returned with th follwoing error codes:
 */
int pclKeyReadData(unsigned int ldbid, const char* resource_id, unsigned int user_no, unsigned int seat_no, unsigned char* buffer, int buffer_size);



/**
 * @brief register a change notification for persistent data
 *
 * @param ldbid logical database ID of the resource to monitor
 * @param resource_id the resource ID
 * @param user_no  the user ID; user_no=0 can not be used as user-ID beacause ‘0’ is defined as System/node
 * @param seat_no  the seat number
 * @param callback notification callback
 *
 * @return positive value: registration OK; On error a negative value will be returned with th follwoing error codes:
 */
int pclKeyRegisterNotifyOnChange(unsigned int ldbid, const char* resource_id, unsigned int user_no, unsigned int seat_no, changeNotifyCallback_t callback);



/**
 * @brief writes persistent data identified by ldbid and resource_id
 *
 * @param ldbid logical database ID
 * @param resource_id the resource ID
 * @param user_no  the user ID; user_no=0 can not be used as user-ID beacause ‘0’ is defined as System/node
 * @param seat_no  the seat number
 * @param buffer the buffer containing the persistent data to write
 * @param buffer_size the number of bytes to write (default max size is set to 16kB)
 *                    use environment variable PERS_MAX_KEY_VAL_DATA_SIZE to modify default size in bytes
 *
 * @return positive value: the bytes written; On error a negative value will be returned with th follwoing error codes:
 */
int pclKeyWriteData(unsigned int ldbid, const char* resource_id, unsigned int user_no, unsigned int seat_no, unsigned char* buffer, int buffer_size);


#ifdef __cplusplus
}
#endif

/** \} */ /* End of API */
/** \} */ /* End of MODULE */

#endif /* PERSISTENCY_CLIENT_LIBRARY_KEY_H */

