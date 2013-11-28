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


/// enumerator used to identify the policy to manage the data
typedef enum _PersistenceCustomLibs_e
{
   PersCustomLib_early       = 0,  /// predefined custom library for early persistence
   PersCustomLib_secure      = 1,  /// predefined custom library for secure persistence
   PersCustomLib_emergency   = 2,  /// predefined custom library for emengency persistence
   PersCustomLib_HWinfo      = 3,  /// predefined custom library for hw information
   PersCustomLib_Custom1     = 4,  /// custom library 1
   PersCustomLib_Custom2     = 5,  /// custom library 2
   PersCustomLib_Custom3     = 6,  /// custom library 3

   // insert new entries here ...

   PersCustomLib_LastEntry         /// last entry

} PersistenceCustomLibs_e;


/// enumerator fo custom library defines
enum _PersCustomLibDefines_e
{
   PersCustomPathSize = 12

} PersCustomLibDefines_e;


/// callback definition for custom_plugin_get_status_notification_clbk function
typedef int (*plugin_callback_t) (int status, void* dataPtr);


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
 * @brief get the names of the custom libraries to load
 *
 * @param customLib the enumerator id identifying the custom library
 * @param customFuncts function pointer array of loaded custom library functions
 *
 * @return 0 for success or a negative value with one of the following errors:
 *  EPERS_NOPLUGINFCNT   EPERS_DLOPENERROR
 *
 */
int load_custom_library(PersistenceCustomLibs_e customLib, Pers_custom_functs_s *customFuncts);



/**
 * @brief get the names of the custom libraries to load
 *
 * @return 0 for success orr a negative value with one of the following errors:
 *  EPERS_NOPLUGINFCNT   EPERS_DLOPENERROR
 */
int load_all_custom_libraries();


/**
 * @brief get the position in the array
 *
 * @param customLib the enumerator id identifying the custom library
 *
 * @return the array position or -1 if the position can't be found
 */
int check_valid_idx(int idx);



/**
 * @brief get the custom library name form an index
 *
 * @return the name of the custom library ot NULL if invalid
 */
char* get_custom_client_lib_name(int idx);


/**
 * @brief invalidate customer plugin function
 */
void invalidate_custom_plugin(int idx);


#endif /* PERSISTENCE_CLIENT_LIBRARY_CUSTOM_LOADER_H */
