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
 * @file           persistenceCustomLib_Demo.c
 * @ingroup        Persistency client library
 * @author         Ingo Huerner
 * @brief          Demo implementation of the persistence custom client library
 * @see            
 */


#include <stdio.h>
#include <string.h>

#include "../include/persistence_client_custom.h"

/**
 * @brief close the given handle
 *
 * @param handle the handle to close
 *
 * @return positive value: successfully closed; negative value: error
 */
// OK
int plugin_handle_close(int handle)
{
   int rval = 99;

   (void)handle;

   //printf("* * * * * plugin_close- handle: %d!\n", handle);

   return rval;
}


/**
 * @brief delete data
 *
 * @param path the path to the data to delete
 *
 * @return positive value: delete success; negative value: error
 */
// OK
int plugin_delete_data(const char* path)
{
   int rval = 13579;
   (void)path;

   //printf("* * * * * plugin_delete_data - path: %s!\n", path);

   return rval;
}


/**
 * @brief get data
 *
 * @param handle the handle returned from open
 * @param buffer the buffer to store data
 * @param size the number of bytes to get data
 *
 * @return positive value: size data read in bytes; negative value: error
 */
// OK
int plugin_handle_get_data(int handle, char* buffer, int size)
{
   //int strSize = 99;
   //printf("plugin_handle_get_data: %s\n", LIBIDENT);
   (void)handle;

   return snprintf(buffer, size, "Custom plugin -> plugin_get_data_handle: %s!", LIBIDENT);
}
/**
 * @brief get data
 *
 * @param buffer the buffer to store data
 * @param size the number of bytes to get data
 *
 * @return positive value: size data read in bytes; negative value: error
 */
// OK
int plugin_get_data(const char* path, char* buffer, int size)
{
   //int strSize = 99;

   //printf("Custom plugin -> plugin_get_data: %s!\n", LIBIDENT);
   (void)path;
   (void)buffer;
   (void)size;

   return snprintf(buffer, size, "Custom plugin -> plugin_get_data: %s!", LIBIDENT);
}



/**
 * @brief initialize plugin
 *
 * @return positive value: init success; negative value: error
 */
// OK
int plugin_init()
{
   //int rval = 99;

   //printf("* * * * * plugin_init sync  => %s!\n", LIBIDENT);

   return 1;
}

int plugin_init_async(plugin_callback_async_t pfInitCompletedCB)
{
   //int rval = -1;

	//printf("* * * * * plugin_init_async => %s!\n", LIBIDENT);
   (void)pfInitCompletedCB;

	return 1;
}


/**
 * @brief deinitialize plugin
 *
 * @return positive value: init success; negative value: error
 */
// OK
int plugin_deinit()
{
   int rval = 99;

   //printf("* * * * * plugin_deinit: %s!\n", LIBIDENT);

   return rval;
}




/**
 * @brief open a resource
 *
 * @param path the path to the resource to open
 * @param flag open flags
 * @param mode the open mode
 *
 * @return positive value: handle; negative value: error
 */
// OK
int plugin_handle_open(const char* path, int flag, int mode)
{
   int rval = 100;

   //printf("* * * * * plugin_open - path: %s | flag: %d | mode: %d | plugin: %s!\n", path, flag, mode, LIBIDENT);
   (void)path;
   (void)flag;
   (void)mode;

   return rval;
}


/**
 * @brief set data
 *
 * @param handle the handle given by open
 * @param buffer the data to write
 * @param size the number of bytes to write
 *
 * @return positive size data set; negative value: error
 */
// OK
int plugin_handle_set_data(int handle, char* buffer, int size)
{
   int rval = 123654;

   //printf("* * * * * plugin_handle_set_data: %s!\n", LIBIDENT);
   (void)handle;
   (void)buffer;
   (void)size;

   return rval;
}
/**
 * @brief set data
 *
 * @param buffer the data to write
 * @param size the number of bytes to write
 *
 * @return positive size data set; negative value: error
 */
// OK
int plugin_set_data(const char* path, char* buffer, int size)
{
   int rval = 321456;

   //printf("* * * * * plugin_set_data: %s!\n", LIBIDENT);
   (void)path;
   (void)buffer;
   (void)size;

   return rval;
}


/**
 * @brief typdef of callback function prototype
 *
 * @param status status identifier
 * @param dataPtr data
 */
typedef int (*plugin_callback_t) (int status, void* dataPtr);


/**
 * @brief registercallback for status notifications
 *
 * @param pFunct the callback
 *
 * @return positive value: register success; negative value error
 */
int plugin_get_status_notification_clbk(plugin_callback_t pFunct)
{
   int rval = 99;

   //printf("* * * * * plugin_get_status_notification_clbk: %s!\n", LIBIDENT);
   (void)pFunct;

   return rval;
}


int plugin_handle_get_size(int handle)
{
   int rval = 11223344;

   //printf("* * * * * plugin_get_size_handle: %d | %s!\n", handle, LIBIDENT);
   (void)handle;

   return rval;
}

// OK
int plugin_get_size(const char* path)
{
   int rval = 44332211;

   //printf("* * * * * plugin_get_size: %s | %s!\n", path, LIBIDENT);
   (void)path;

   return rval;
}


// OK
int plugin_create_backup(const char* backup_id)
{
   int rval = -1;

   printf("* * * * * plugin_create_backup: backup_id %s | %s!\n", backup_id, LIBIDENT);
   (void)backup_id;

   return rval;
}

// OK
int plugin_restore_backup(const char* backup_id)
{
   int rval = -1;

   printf("* * * * * plugin_restore_backup: backup_id %s | %s!\n", backup_id, LIBIDENT);
   (void)backup_id;

   return rval;

}

// OK
int plugin_get_backup(char* backup_id, int size)
{
   int rval = -1;

   printf("* * * * * plugin_get_backup: backup_id %s\n", backup_id);
   (void)backup_id;
   (void)size;

   return rval;
}


int plugin_clear_all_data(void)
{
   printf("plugin_clear_all_data\n");
   return 1;
}


int plugin_sync(void)
{
   printf("plugin_sync\n");
   return 1;
}



