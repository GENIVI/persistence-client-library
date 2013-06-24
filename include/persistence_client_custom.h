#ifndef PERSISTENCE_CLIENT_LIBRARY_CUSTOM_H
#define PERSISTENCE_CLIENT_LIBRARY_CUSTOM_H

/****************************************************
 *  persistence_custom.h                                         
 *  Created on: 09-Jul-2012 11:38:03                      
 *  Implementation of the Interface PersCustom       
 *  Original author: ihuerner, G.Sagnes
 *
 *  \file   persistence_client_custom.h
 *  \brief  Implementation of the Interface PersCustom
 *
 *  \par Responsibility
 *   - SW-Subsystem:         EG-SI
 *   - SW-Domain:            Persistence
 *   - Interface Visibility: Protected
 *  \par change history
 *  \verbatim
 *  Date       Author    Version  Description 
 *  2013.06.26 ihuerner  1.5.0.0  added description of parameters
 *  2013.01.06 ihuerner  1.4.0.0  plugin_handle_open and plugin_set_data changed from char* to const char*
 *  2012.11.22 gsagnes   1.3.0.0  add the handle_get_size, correct the type to int
 *  2012.10.16 gsagnes   1.2.0.0  add get_size, create_backup, restore_backup
 *  2012.10.04 gsagnes   1.1.0.0  add deinitialisation functionality (call during shutdown)
 *  2012.07.14 ihuerner  1.0.0.0  Initial version of the interface
 *  \endverbatim
 *
 ****************************************************/
 
/** \ingroup GEN_PERS_CLIENTLIB_INTERFACE API document
 *  \{
 */

/** Module version
The lower significant byte is equal 0 for released version only
*/
#define     PERSIST_CUSTOMER_INTERFACE_VERSION            (0x01050000U)

/**
 * <b>Plugin interface:</b>
 * All plugins in a specual location will be loaded, and according
 * to the plugin name  functions will created.
 * Example:
 * function name: plugin_open
 * Loaded plugin name: hwi
 * Generated function: hwi_plugin_open
 */
 
/**
 * @brief create backup
 *
 * @param backup_id Name of the backup / identifier
 *
 * @return positive value: backup success (size of backup, bytes); negative value: error
 */
int plugin_create_backup(const char* backup_id);

 /**
 * @brief deinitialize plugin (during shutdown)
 *
 * @return positive value: init success; negative value: error
 */
int plugin_deinit();

/**
 * @brief delete data
 *
 * @param path the path to the data to delete
 *
 * @return positive value: delete success; negative value: error
 */
int plugin_delete_data(const char* path);

/**
 * @brief get backup name
 *
 * @param backup_id Name of the backup / identifier
 * @param size size of the buffer to return the identifier
 *
 * @return positive value: success, length of identifier; negative value: error
 */
int plugin_get_backup(char* backup_id, int size);

/**
 * @brief gets the size of persistent data in bytes
 *
 * @param path the path to the data
 *
 * @return positive value: the size; negative value: error code
 */
int plugin_get_size(const char* path);

/**
 * @brief get data
 *
 * @param path the path to the resource to get
 * @param buffer the buffer to store data
 * @param size the number of bytes to get data
 *
 * @return positive value: size data read in bytes; negative value: error
 */
int plugin_get_data(const char* path, char* buffer, int size);
 
/**
 * @brief close the given handle
 *
 * @param handle the handle to close
 *
 * @return positive value: successfully closed; negative value: error
 */
int plugin_handle_close(int handle);

/**
 * @brief get data
 *
 * @param handle the handle returned from open
 * @param buffer the buffer to store data
 * @param size the number of bytes to get data
 *
 * @return positive value: size data read in bytes; negative value: error
 */
int plugin_handle_get_data(int handle, char* buffer, int size);

/**
 * @brief open a resource
 *
 * @param path the path to the resource to open
 * @param flag open flags
 * @param mode the open mode
 *
 * @return positive value: handle; negative value: error
 */
int plugin_handle_open(const char* path, int flag, int mode);

/**
 * @brief set data
 *
 * @param handle the handle given by open
 * @param buffer the data to write
 * @param size the number of bytes to write
 *
 * @return positive size data set; negative value: error
 */
int plugin_handle_set_data(int handle, char* buffer, int size);

/**
 * @brief initialize plugin
 *
 * @return positive value: init success; negative value: error
 */
int plugin_init();

/**
 * @brief restore backup
 *
 * @param backup_id Name of the backup / identifier
 *
 * @return positive value: backup success (size of backup, bytes); negative value: error
 */
int plugin_restore_backup(const char* backup_id);

/**
 * @brief set data
 *
 * @param path the path to the resource to set
 * @param buffer the data to write
 * @param size the number of bytes to write
 *
 * @return positive size data set; negative value: error
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
 * @return positive value: register success; negative value error
 */
int plugin_get_status_notification_clbk(plugin_callback_t pFunct);

/**
 * @brief get size
 *
 * @param handle the handle given by open
 *
 * @return positive value: the size; negative value: error code
 */
int plugin_handle_get_size(int handle);

#endif /* PERSISTENCE_CLIENT_LIBRARY_CUSTOM_H */
/** \} */ /* End of INTERFACE */

