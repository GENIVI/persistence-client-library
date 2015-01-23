#ifndef PERSISTENCE_CLIENT_LIBRARY_CUSTOM_LOADER_H
#define PERSISTENCE_CLIENT_LIBRARY_CUSTOM_LOADER_H

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
 * @file           persistence_client_library_custom_loader.h
 * @ingroup        Persistence client library
 * @author         Ingo Huerner
 * @brief          Header of the persistence client library custom library_loader.
 * @see
 */

#include "../include/persistence_client_custom.h"

#include "persistence_client_library_data_organization.h"

/// enumerator used to identify the policy to manage the data
typedef enum _PersistenceCustomLibs_e
{
   /// used for the default library
   PersCustomLib_default     = 0,
   /// predefined custom library for early persistence
   PersCustomLib_early       = 1,
   /// predefined custom library for secure persistence
   PersCustomLib_secure      = 2,
   /// predefined custom library for emengency persistence
   PersCustomLib_emergency   = 4,
   /// predefined custom library for hw information
   PersCustomLib_HWinfo      = 5,
   /// custom library 1
   PersCustomLib_Custom1     = 6,
   /// custom library 2
   PersCustomLib_Custom2     = 7,
   /// custom library 3
   PersCustomLib_Custom3     = 8,

   // insert new entries here ...

   /// last entry
   PersCustomLib_LastEntry

} PersistenceCustomLibs_e;


/// enumerator for custom library defines
typedef enum _PersCustomLibDefines_e
{
   /// the custom library path size
   PersCustomPathSize = 24

} PersCustomLibDefines_e;


/// indicates the init method type
typedef enum PersInitType_e_
{
   /// initialize the plugin with the synchronous init function
   Init_Synchronous  = 0,
   /// initialize the plugin with the asynchronous init function
   Init_Asynchronous = 1,
   /// undefined
   Init_Undefined
} PersInitType_e;


/// indicates the plugin loading type
typedef enum PersLoadingType_e_
{
   /// load plugin during pclInitLibrary function
   LoadType_PclInit  = 0,
   /// load the pluing on demand, when a plugin function will be requested the first time.
   LoadType_OnDemand = 1,
   /// undefined
   LoadType_Undefined
} PersLoadingType_e;


/// directory structure seat name definition
char* plugin_gSeat;
/// path prefix for local write through database /Data/mnt-wt/\<appId\>/\<database_name\>
char* plugin_gLocalWt;
///directory structure user name definition
char* plugin_gUser;
/// local factory-default database
char* plugin_gLocalFactoryDefault;
/// local cached default database
char* plugin_gLocalCached;
/// directory structure node name definition
char* plugin_gNode;
/// local configurable-default database
char* plugin_gLocalConfigurableDefault;
/// resource configuration table name
char* plugin_gResTableCfg;


/// Obtain a handler to DB indicated by dbPathname
signed int (*plugin_persComDbOpen)(char const * dbPathname, unsigned char bForceCreationIfNotPresent) ;

/// Close handler to DB
signed int (*plugin_persComDbClose)(signed int handlerDB) ;

/// write a key-value pair into local/shared database
signed int (*plugin_persComDbWriteKey)(signed int handlerDB, char const * key, char const * data, signed int dataSize) ;

/// read a key's value from local/shared database
signed int (*plugin_persComDbReadKey)(signed int handlerDB, char const * key, char* dataBuffer_out, signed int dataBufferSize) ;

/// read a key's value from local/shared database
signed int (*plugin_persComDbGetKeySize)(signed int handlerDB, char const * key) ;

/// delete key from local/shared database
signed int (*plugin_persComDbDeleteKey)(signed int handlerDB, char const * key) ;

/// Find the buffer's size needed to accomodate the list of keys' names in local/shared database
signed int (*plugin_persComDbGetSizeKeysList)(signed int handlerDB) ;

/// Obtain the list of the keys' names in local/shared database
signed int (*plugin_persComDbGetKeysList)(signed int handlerDB, char* listBuffer_out, signed int listBufferSize) ;


/**
 * \brief Obtain a handler to RCT indicated by rctPathname
 * \note : RCT is created if it does not exist and (bForceCreationIfNotPresent != 0)
 */
signed int (*plugin_persComRctOpen)(char const * rctPathname, unsigned char bForceCreationIfNotPresent) ;

/// Close handler to RCT
signed int (*plugin_persComRctClose)(signed int handlerRCT) ;

/// read a resourceID's configuration from RCT
signed int (*plugin_persComRctRead)(signed int handlerRCT, char const * resourceID, PersistenceConfigurationKey_s const * psConfig_out) ;


/**
 * @brief definition of async init callback function.
 *        This function will be called when the asynchronous
 *        init function (custom_plugin_init_async) has finished
 *
 * @param error the error code occured while calling init
 */
extern int(* gPlugin_callback_async_t)(int errcode);


/// structure definition for custom library functions
typedef struct _Pers_custom_functs_s
{
   /// custom library handle
   void* handle;

   /// create backup
   int (*custom_plugin_create_backup)(const char* backup_id);

   /// custom library deinit function
   int (*custom_plugin_deinit)();

   /// custom delete function
   int (*custom_plugin_delete_data)(const char* path);

   /// get backup
   int (*custom_plugin_get_backup)(char* backup_id, int size);

   // get the size
   int (*custom_plugin_get_size)(const char* path);

   /// custom get data function
   int (*custom_plugin_get_data)(const char* path, char* buffer, int size);

   /// custom close function
   int (*custom_plugin_handle_close)(int handle);

   /// custom get data function
   int (*custom_plugin_handle_get_data)(int handle, char* buffer, int size);

   /// custom open function
   int (*custom_plugin_handle_open)(const char* path, int flag, int mode);

   /// custom set data function
   int (*custom_plugin_handle_set_data)(int handle, char* buffer, int size);

   /// custom library init function
   int (*custom_plugin_init)();

   /// restore backup
   int (*custom_plugin_restore_backup)(const char* backup_id);

   /// custom set data function
   int (*custom_plugin_set_data)(const char* path, char* buffer, int size);

   /// custom status notification function
   int (*custom_plugin_get_status_notification_clbk)(plugin_callback_t pFunct);

   // get the size
   int (*custom_plugin_handle_get_size)(int handle);

   /// initialize plugin (non blocking)
   int (*custom_plugin_init_async)(plugin_callback_async_t pfInitCompletedCB);

   /// clear all data
   int (*custom_plugin_clear_all_data)(void);

   /// sync all data
   int (*custom_plugin_sync)(void);


}Pers_custom_functs_s;


/// custom library functions array
Pers_custom_functs_s gPersCustomFuncs[PersCustomLib_LastEntry];


/**
 * @brief Translate a client library name into a id
 *
 * @param lib_name the library name
 * @param substring indicator if a substring search is neccessary
 *
 * @return the library id or PersCustomLib_LastEntry if nothing found
 */
PersistenceCustomLibs_e custom_client_name_to_id(const char* lib_name, int substring);

/**
 * @brief get the names of the custom libraries to load
 *
 * @return 0 for success or a negative value with the following errors:
 * EPERS_OUTOFBOUNDS
 */
int get_custom_libraries();


/**
 * @brief load default plugin
 *
 * handle the library handle
 *
 * @return default library
 */
int load_default_library(void* handle);

/**
 * @brief get the names of the custom libraries to load
 *
 * @param customLib the enumerator id identifying the custom library
 * @param customFuncts function pointer array of loaded custom library functions
 *
 * @return 0 for success or a negative value with one of the following errors:
 *  EPERS_NOPLUGINFCNT   EPERS_DLOPENERROR
 */
int load_custom_library(PersistenceCustomLibs_e customLib, Pers_custom_functs_s *customFuncts);


/**
 * @brief get the position in the array
 *
 * @param idx the index to identifying the custom library
 *
 * @return the array position or -1 if the position can't be found
 */
int check_valid_idx(int idx);



/**
 * @brief get the custom library name form an index
 *
 * @param idx the index to identifying the custom library
 *
 * @return the name of the custom library ot NULL if invalid
 */
char* get_custom_client_lib_name(int idx);


/**
 * @brief invalidate customer plugin function
 *
 * @param idx the plugin index
 */
void invalidate_custom_plugin(int idx);


/**
 * @brief load the custom plugins.
 *        The custom library configuration file will be loaded to see
 *        if there a re plugins that must be loaded in the pclInitLibrary function.
 *        The other plugins will be loaded on demand.
 *
 *
 * @param pfInitCompletedCB the callback function to be called when
 *        a plugin with asyncnonous init function will be laoded
 */
int load_custom_plugins(plugin_callback_async_t pfInitCompletedCB);


/**
 * @brief Get the custom loading type.
 *        The loading type is
 *        ::LoadType_PclInit or ::LoadType_OnDemand
 *
 * @param i the custom id, see ::PersistenceCustomLibs_e
 */
PersLoadingType_e getCustomLoadingType(int i);


/**
 * @brief Get the custom init type.
 *        The init type is
 *        ::Init_Synchronous or ::Init_Asynchronous
 *
 * @param i the custom id, see ::PersistenceCustomLibs_e
 */
PersInitType_e getCustomInitType(int i);


#endif /* PERSISTENCE_CLIENT_LIBRARY_CUSTOM_LOADER_H */
