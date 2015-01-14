#ifndef PERSISTENCE_CLIENT_LIBRARY_CUSTOM_H
#define PERSISTENCE_CLIENT_LIBRARY_CUSTOM_H

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
 * @file           persistence_client_custom.h
 * @ingroup        Persistence client library
 * @author         Ingo Huerner (XSe) / Guy Sagnes (Continental) / Ionut Ieremie (Continental)
 * @brief          Header of the persistence client library custom plugin.
 *                 Library provides an plugin API to extend persistence client library
 * @par change history
 *    Date       Author    Version  Description
 *  - 2015.01.14 ihuerner  1.6.0.0  Extended header documentation for function plugin_init_async.
 *  - 2014.01.20 iieremie  1.6.0.0  multiple extensions:
 *                                  - error codes
 *                                  - asynchronous init/deinit
 *                                  - sync(flush) data
 *                                  - clear all data
 *                                  - get info
 *                                  - added explanations
 *  - 2013.06.26 ihuerner  1.5.0.0  added description of parameters
 *  - 2013.01.06 ihuerner  1.4.0.0  plugin_handle_open and plugin_set_data changed from char* to const char*
 *  - 2012.11.22 gsagnes   1.3.0.0  add the handle_get_size, correct the type to int
 *  - 2012.10.16 gsagnes   1.2.0.0  add get_size, create_backup, restore_backup
 *  - 2012.10.04 gsagnes   1.1.0.0  add deinitialisation functionality (call during shutdown)
 *  - 2012.07.14 ihuerner  1.0.0.0  Initial version of the interface
 */


/** \ingroup GEN_PERS_CLIENTLIB_INTERFACE API document
 *  \{
 */

/** \defgroup PCCL_INTERFACE_VERSION persistence_client_custom version
 *  \{
 */

/** Module version
The lower significant byte is equal 0 for released version only
*/
#define     PERSIST_CUSTOMER_INTERFACE_VERSION            (0x01060000U)

/** \} */ /* End of Errors */

/**
 * <b>Plugin interface:</b>
 * All plugins in a specual location will be loaded, and according
 * to the plugin name  functions will created.
 * Example:
 * function name: plugin_open
 * Loaded plugin name: hwi
 * Generated function: hwi_plugin_open
 */


/** \defgroup PCCL_RETURNS persistence_client_custom Return Values
 * ::PCCL_SUCCESS, ::PCCL_ERROR_CODE..::PCCL_FAILURE_INVALID_PARAMETER
 
 * These defines are used to define the return values of the interface functions
 *   - ::PCCL_SUCCESS: the function call succeded
 *   - ::PCCL_ERROR_CODE..::PCCL_FAILURE: the function call failed
 *  \{
 */
/* Error code return by the SW Package, related to SW_PackageID. */
#define PCCL_PACKAGEID                           0x013                       /*!< Software package identifier, use for return value base */
#define PCCL_BASERETURN_CODE                    (PCCL_PACKAGEID << 16)       /*!< Basis of the return value containing SW PackageID */

#define PCCL_SUCCESS                             0x00000000                  /*!< the function call succeded */

#define PCCL_ERROR_CODE                       (-(PCCL_BASERETURN_CODE))      /*!< basis of the error (negative values) */
#define PCCL_FAILURE_NOT_INITIALIZED            (PCCL_ERROR_CODE - 1)        /*!< try to access functionality before initialization  */
#define PCCL_FAILURE_INVALID_PARAMETER          (PCCL_ERROR_CODE - 2)        /*!< Invalid parameter in the API call  */
#define PCCL_FAILURE_BUFFER_TOO_SMALL           (PCCL_ERROR_CODE - 3)        /*!< The provided buffer can not accommodate the available data size */
#define PCCL_FAILURE_OUT_OF_MEMORY              (PCCL_ERROR_CODE - 4)        /*!< not enough memory, malloc failed, no handler available */
#define PCCL_FAILURE_INVALID_FORMAT             (PCCL_ERROR_CODE - 5)        /*!< format of the path is not as expected */
#define PCCL_FAILURE_NOT_FOUND                  (PCCL_ERROR_CODE - 6)        /*!< e.g. a folder/file is not found */
#define PCCL_FAILURE_NO_DATA                    (PCCL_ERROR_CODE - 7)        /*!< no data available for the key */
#define PCCL_FAILURE_ACCESS_DENIED              (PCCL_ERROR_CODE - 8)        /*!< tried to access a file without having the right */
#define PCCL_FAILURE_OPERATION_NOT_SUPPORTED    (PCCL_ERROR_CODE - 9)        /*!< operation not (yet) supported */

#define PCCL_FAILURE                            (PCCL_ERROR_CODE - 0xFFFF)   /*!< should be the max. value for error */

#define PCCL_WARNING_CODE                       (PCCL_BASERETURN_CODE)       /*!< basis of the warning (positive values) */

/** \} */ /* End of Errors */


/** \defgroup PCCL_FUNCTIONS persistence_client_custom Functions
 *  \{
 */

/**
 * @brief typdef of callback function prototype for asynchronous init/deinit
 *
 * @param errcode - pass \ref PCCL_SUCCESS or an error code (\ref PCCL_RETURNS)
 */
typedef int (*plugin_callback_async_t) (int errcode);

/**
 * @brief initialize plugin (blocking)
 *
 * @return positive value: init success; negative value: error code (\ref PCCL_RETURNS)
 *
 * @note
 *       - This (or \ref plugin_init_async) is the first function to call from this interface
 *       - The call is blocking
 */
int plugin_init();

/**
 * @brief initialize plugin (non blocking)
 *
 * @param pfInitCompletedCB the callback to be called when the initialization is complete
 *
 * @return positive value: init success; negative value: error code (\ref PCCL_RETURNS)
 *
 * @note
 *       - This (or \ref plugin_init) is the first function to call from this interface
 *       - The call returns immediately.
 *       - Later, pfInitCompletedCB will be called when the initialization is complete.
 *       - If a plugin function will be called and the plugin initialization is not
 *         finished jet, the plugin function must return PCCL_FAILURE_NOT_INITIALIZED error code.
 */
int plugin_init_async(plugin_callback_async_t pfInitCompletedCB);

 /**
 * @brief deinitialize plugin (during shutdown)
 *
 * @return positive value: deinit success; negative value: error code (\ref PCCL_RETURNS)
 *
 * @note
 *       - This is the last function to call from this interface
 *       - The call is blocking.
 */
int plugin_deinit();


/**
 * @brief delete data
 *
 * @param path the path to the data to delete
 *
 * @return positive value: delete success; negative value: error code (\ref PCCL_RETURNS)
 */
int plugin_delete_data(const char* path);


/**
 * @brief gets the size of persistent data in bytes
 *
 * @param path the path to the data
 *
 * @return positive value: the size; negative value: error code (\ref PCCL_RETURNS)
 */
int plugin_get_size(const char* path);

/**
 * @brief get data
 *
 * @param path the path to the resource to get
 * @param buffer the buffer to store data
 * @param size the number of bytes to get data
 *
 * @return positive value: size data read in bytes; negative value: error code (\ref PCCL_RETURNS)
 */
int plugin_get_data(const char* path, char* buffer, int size);
 
/**
 * @brief close the given handle
 *
 * @param handle the handle to close
 *
 * @return positive value: successfully closed; negative value: error code (\ref PCCL_RETURNS)
 */
int plugin_handle_close(int handle);

/**
 * @brief get data
 *
 * @param handle the handle returned from open
 * @param buffer the buffer to store data
 * @param size the number of bytes to get data
 *
 * @return positive value: size data read in bytes; negative value: error code (\ref PCCL_RETURNS)
 */
int plugin_handle_get_data(int handle, char* buffer, int size);

/**
 * @brief open a resource
 *
 * @param path the path to the resource to open
 * @param flag open flags
 * @param mode the open mode
 *
 * @return positive value: handle; negative value: error code (\ref PCCL_RETURNS)
 */
int plugin_handle_open(const char* path, int flag, int mode);

/**
 * @brief set data
 *
 * @param handle the handle given by open
 * @param buffer the data to write
 * @param size the number of bytes to write
 *
 * @return positive size data set; negative value: error code (\ref PCCL_RETURNS)
 */
int plugin_handle_set_data(int handle, char* buffer, int size);

/**
 * @brief create backup of the container's data
 *
 * @param backup_id backup destination's pathname (e.g. /tmp/bkup/mybackup.bk)
 *
 * @return positive value: backup success (size of backup, bytes); negative value: error code (\ref PCCL_RETURNS)
 *
 * @note
 *       - The path (e.g. /tmp/bkup/ for our example) has to exist and to be accessible
 *       - The format of backup file is in responsibility of the customer plugin
 *       - The backup shall be created using the data in persistent storage (obtained after last \ref plugin_sync)
 *       - To be used only by Persistence Administrator Service !
 */
int plugin_create_backup(const char* backup_id);

/**
 * @brief get backup name
 *
 * @param backup_id Name of the backup / identifier
 * @param size size of the buffer to return the identifier
 *
 * @return positive value: success, length of identifier; negative value: error code (\ref PCCL_RETURNS)
 * TO DO: What is the idea ?
 */
int plugin_get_backup(char* backup_id, int size);

/**
 * @brief restore backup
 *
 * @param backup_id backup source's pathname (e.g. /tmp/bkup/mybackup.bk)
 *
 * @return positive value: backup success (size of backup, bytes); negative value: error code (\ref PCCL_RETURNS)
 *
 * @note
 *       - The source has to be compatible (i.e. obtained with \ref plugin_create_backup)
 *       - The backup's data has first to be stored persistent and then reloaded in cache (if used)
 *       - To be used only by Persistence Administrator Service !
 */
int plugin_restore_backup(const char* backup_id);

/**
 * @brief clear data (delete values for all the resources)
 *
 * @return positive value: success; negative value: error code (\ref PCCL_RETURNS)
 *
 * @note
 *       - the new (empty) data content is flushed to non-volatile storage
 *       - To be used only by Persistence Administrator Service !
 */
int plugin_clear_all_data(void);

/**
 * @brief set data
 *
 * @param path the path to the resource to set
 * @param buffer the data to write
 * @param size the number of bytes to write
 *
 * @return positive size data set; negative value: error code (\ref PCCL_RETURNS)
 */
int plugin_set_data(const char* path, char* buffer, int size);

/**
 * @brief typdef of callback function prototype
 *
 * @param int pass a statusId to the function
 * @param void* pass an argument to the function
 */
typedef int (*plugin_callback_t) (int, void*);

/**
 * @brief registercallback for status notifications
 *
 * @param pFunct the callback
 *
 * @return positive value: register success; negative value error code (\ref PCCL_RETURNS)
 */
int plugin_get_status_notification_clbk(plugin_callback_t pFunct);

/**
 * @brief get size
 *
 * @param handle the handle given by open
 *
 * @return positive value: the size; negative value: error error code (\ref PCCL_RETURNS)
 */
int plugin_handle_get_size(int handle);

/**
 * @brief flush the cache (if used) into the non-volatile storage
 *
 * @return positive value for success; negative value: error code (\ref PCCL_RETURNS)
 *
 * @note
 *       - To be used only by Persistence Administrator Service !
 */
int plugin_sync(void);

/**
 * @brief information to be provided as status
 */
typedef struct _plugin_info_s
{
    long     used_size ;         /*!< The current total size in bytes of the data */
    long     available_size ;    /*!< The current available size in bytes */
    long     number_of_items ;   /*!< The current number of resources inside the plugin */
}plugin_info_s ;

/**
 * @brief get the status information for the plugin's data
 *
 * @param pInfo_out the status information
 *
 * @return positive value for success; negative value: error code (\ref PCCL_RETURNS)
 *
 * @note
 *       - To be used only by Persistence Administrator Service !
 */
int plugin_get_info(plugin_info_s* pInfo_out);

/** \} */
/** \} */

#endif /* PERSISTENCE_CLIENT_LIBRARY_CUSTOM_H */
/** \} */ /* End of INTERFACE */

