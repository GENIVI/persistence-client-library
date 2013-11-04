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
 * vauthor         Ingo Huerner (XSe) / Guy Sagnes (Continental)
 * @brief          Header of the persistence client library.
 *                 Library provides an API to access persistent data
 * @par change history
 * Date     Author          Version
 * 27/03/13 Ingo Hürner     4.0.0 - Add registration for callback notification
 * 28/05/13 Ingo Hürner     5.0.0 - Add pclInitLibrary(), pcl DeInitLibrary() incl. shutdown notification
 * 05/06/13 Oliver Bach     6.0.0 - Rework of Init functions
 */
/** \ingroup GEN_PERS */
/** \defgroup PERS_KEYVALUE Client: Key-value access
 *  \{
 */
/** \defgroup PERS_KEYVALUE_INTERFACE API document
 *  \{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** \defgroup PCL_DEFINES_KEYVALUE Defines, Struct, Enum
 * \{
 */

#define  PERSIST_KEYVALUEAPI_INTERFACE_VERSION   (0x06010000U)

#include "persistence_client_library.h"


/**
* status returned in notification structure
*/
typedef enum _pclNotifyStatus_e
{
   pclNotifyStatus_no_changed = 0,
   pclNotifyStatus_created,
   pclNotifyStatus_changed,
   pclNotifyStatus_deleted,
   /* insert new_ entries here .. */
   pclNotifyStatus_lastEntry
} pclNotifyStatus_e;


/**
* structure to return in case of notification
*/
typedef struct _pclNotification_s
{
   pclNotifyStatus_e pclKeyNotify_Status;    /// notification status
   unsigned int ldbid;                       /// logical db id
   const char * resource_id;                 /// resource id
   unsigned int user_no;                     /// user id
   unsigned int seat_no;                     /// seat id
} pclNotification_s;

/** \defgroup SHUTDOWN_TYPE shutdown notification type definitions
 *  according to Node State Manager component
 * \{
 */


/** \} */

/** \} */

/** definition of the change callback
 *
 * @param notifyStruct structure for notifcation
 *
 * @return positive value: success;
 *   On error a negative value will be returned with the following error codes: ::EPERS_LOCKFS
*/
typedef int(* pclChangeNotifyCallback_t)(pclNotification_s * notifyStruct);


/** \defgroup PCL_KEYVALUE functions Key-Value access
 * \{
 */

/**
 * @brief delete persistent data
 *
 * @param ldbid logical database ID
 * @param resource_id the resource ID
 * @param user_no  the user ID; user_no=0 can not be used as user-ID beacause ‘0’ is defined as System/node
 * @param seat_no  the seat number
 *
 * @return positive value: success; On error a negative value will be returned with the following error codes:
 * ::EPERS_LOCKFS ::EPERS_NOTIFY_SIG
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
 * @return positive value: the size; On error a negative value will be returned with the following error codes:
 * ::EPERS_LOCKFS, ::EPERS_BADPOL, ::EPERS_NOKEY, ::EPERS_NOKEYDATA or ::EPERS_NOPRCTABLE
 */
int pclKeyGetSize(unsigned int ldbid, const char* resource_id, unsigned int user_no, unsigned int seat_no);

/**
 * @brief close the access to a key-value identified by key handle
 *
 * @param key_handle key value handle return by key_handle_open()
 *
 * @return positive value: success; On error a negative value will be returned with the following error codes:
 * ::EPERS_LOCKFS
 */
int pclKeyHandleClose(int key_handle);



/**
 * @brief gets the size of persistent data in bytes identified by key handle
 *
 * @param key_handle key value handle return by key_handle_open()
 *
 * @return positive value: the size; On error a negative value will be returned with the following error codes:
 * ::EPERS_LOCKFS
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
 * On error a negative value will be returned with the following error codes:
 * ::EPERS_LOCKFS
 */
int pclKeyHandleOpen(unsigned int ldbid, const char* resource_id, unsigned int user_no, unsigned int seat_no);



/**
 * @brief reads persistent data identified by key handle
 *
 * @param key_handle key value handle return by key_handle_open()
 * @param buffer the buffer for persistent data
 * @param buffer_size size of buffer for reading
 *
 * @return positive value: the bytes read; On error a negative value will be returned with the following error codes:
 * ::EPERS_LOCKFS
 */
int pclKeyHandleReadData(int key_handle, unsigned char* buffer, int buffer_size);



/**
 * @brief register a change notification for persistent data
 *
 * @param key_handle key value handle return by key_handle_open()
 * @param callback notification callback
 *
 * @return positive value: registration OK; On error a negative value will be returned with the following error codes:
 * ::EPERS_LOCKFS ::EPERS_MAXHANDLE ::EPERS_NOTIFY_NOT_ALLOWED
 */
int pclKeyHandleRegisterNotifyOnChange(int key_handle, pclChangeNotifyCallback_t callback);

/**
 * @brief unregister a change notification for persistent data
 *
 * @param key_handle key value handle return by key_handle_open()
 * @param callback notification callback
 *
 * @return positive value: registration OK; On error a negative value will be returned with the following error codes:
 * ::EPERS_LOCKFS ::EPERS_MAXHANDLE ::EPERS_NOTIFY_NOT_ALLOWED
 */
int pclKeyHandleUnRegisterNotifyOnChange(int key_handle, pclChangeNotifyCallback_t callback);

/**
 * @brief writes persistent data identified by key handle
 *
 * @param key_handle key value handle return by key_handle_open()
 * @param buffer the buffer containing the persistent data to write
 * @param buffer_size the number of bytes to write (default max size is set to 16kB)
 *                    use environment variable PERS_MAX_KEY_VAL_DATA_SIZE to modify default size in bytes
 *
 * @return positive value: the bytes written; On error a negative value will be returned with the following error codes:
 * ::EPERS_LOCKFS ::EPERS_MAX_BUFF_SIZE ::EPERS_NOTIFY_SIG
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
 * ::EPERS_LOCKFS
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
 * @return positive value: registration OK; On error a negative value will be returned with the following error codes:
 *                         ::EPERS_RES_NO_KEY ::EPERS_NOKEYDATA  ::EPERS_NOPRCTABLE ::EPERS_NOTIFY_NOT_ALLOWED
 */
int pclKeyRegisterNotifyOnChange(unsigned int ldbid, const char* resource_id, unsigned int user_no, unsigned int seat_no, pclChangeNotifyCallback_t callback);



/**
 * @brief unregister a change notification for persistent data
 *
 * @param ldbid logical database ID of the resource to monitor
 * @param resource_id the resource ID
 * @param user_no  the user ID; user_no=0 can not be used as user-ID beacause ‘0’ is defined as System/node
 * @param seat_no  the seat number
 * @param callback notification callback
 *
 * @return positive value: registration OK; On error a negative value will be returned with the following error codes:
 *                         ::EPERS_RES_NO_KEY ::EPERS_NOKEYDATA  ::EPERS_NOPRCTABLE ::EPERS_NOTIFY_NOT_ALLOWED
 */
int pclKeyUnRegisterNotifyOnChange( unsigned int  ldbid, const char *  resource_id, unsigned int  user_no, unsigned int  seat_no, pclChangeNotifyCallback_t  callback);

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
 * @return positive value: the bytes written; On error a negative value will be returned with the following error codes:
 * ::EPERS_LOCKFS ::EPERS_NOTIFY_NOT_ALLOWED
 */
int pclKeyWriteData(unsigned int ldbid, const char* resource_id, unsigned int user_no, unsigned int seat_no, unsigned char* buffer, int buffer_size);

/** \} */

#ifdef __cplusplus
}
#endif

/** \} */ /* End of API */
/** \} */ /* End of MODULE */

#endif /* PERSISTENCY_CLIENT_LIBRARY_KEY_H */
