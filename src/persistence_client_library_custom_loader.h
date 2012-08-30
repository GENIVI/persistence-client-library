#ifndef PERSISTENCE_CLIENT_LIBRARY_CUSTOM_LOADER_H
#define PERSISTENCE_CLIENT_LIBRARY_CUSTOM_LOADER_H

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

enum _PersCustomLibDefines_e
{
   PersCustomPathSize = 12

} PersCustomLibDefines_e;

/// callback definition for custom_plugin_get_status_notification_clbk function
typedef int (*plugin_callback_t) (int status, void* dataPtr);


/// structure definition for custom library functions
typedef struct _Pers_custom_functs_s
{
   /// custom library init function
   int (*custom_plugin_init)();

   /// custom open function
   long (*custom_plugin_open)(char* path, int flag, int mode);

   /// custom close function
   int (*custom_plugin_close)(int handle);

   /// custom get data function
   long (*custom_plugin_get_data)(long handle, char* buffer, long size);

   /// custom set data function
   long (*custom_plugin_set_data)(long handle, char* buffer, long size);

   /// custom delete function
   int (*custom_plugin_delete_data)(const char* path);

   /// custom status notification function
   int (*custom_plugin_get_status_notification_clbk)(plugin_callback_t pFunct);

}Pers_custom_functs_s;



/// custom library functions array
Pers_custom_functs_s gPersCustomFuncs[PersCustomLib_LastEntry];



PersistenceCustomLibs_e custom_client_name_to_id(const char* lib_name, int substring);

/**
 * @brief get the names of the custom libraries to load
 *
 * @return 0 for success or -1 if an error occurred
 */
int get_custom_libraries();



/**
 * @brief get the names of the custom libraries to load
 *
 * @param customLib the enumerator id identifying the custom library
 * @param customFuncts function pointer array of loaded custom library functions
 *
 * @return 0 for success or -1 if an error occurred
 */
int load_custom_library(PersistenceCustomLibs_e customLib, Pers_custom_functs_s *customFuncts);



/**
 * @brief get the names of the custom libraries to load
 *
 * @return 0 for success or -1 if an error occurred
 */
int load_all_custom_libraries();


/**
 * @brief get the position in the array
 *
 * @param customLib the enumerator id identifying the custom library
 *
 * @return the array position
 */
int get_custom_client_position_in_array(PersistenceCustomLibs_e customLib);


/// get the number of available custom client libraries
int get_num_custom_client_libs();


char* get_custom_client_lib_name(int id);


#endif /* PERSISTENCE_CLIENT_LIBRARY_CUSTOM_LOADER_H */
