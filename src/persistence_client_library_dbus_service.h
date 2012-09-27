#ifndef PERSISTENCE_CLIENT_LIBRARY_DBUS_SERVICE_H_
#define PERSISTENCE_CLIENT_LIBRARY_DBUS_SERVICE_H_

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
 * @file           persistence_client_library_dbus_service.h
 * @ingroup        Persistence client library
 * @author         Ingo Huerner
 * @brief          Header of the persistence client library dbus service.
 * @see
 */

#include <dbus/dbus.h>
#include <poll.h>
#include <pthread.h>

/// mutex to make sure main loop is running
pthread_mutex_t gDbusInitializedMtx;


/// command definitions for main loop
typedef enum ECmd
{
   CMD_NONE = 0,                    /// command none
   CMD_REQUEST_NAME,                /// command request dbus name
   CMD_PAS_BLOCK_AND_WRITE_BACK,    /// command block access and write data back
   CMD_LC_PREPARE_SHUTDOWN,         /// command to prepare shutdown
   CMD_QUIT                         /// quit command
} tCmd;


/// pipe file descriptors
int gPipefds[2];


/// returns the dbus connection
DBusConnection* get_dbus_connection(void);



/**
 * @brief DBus main loop to dispatch events and dbus messages
 *
 * @param vtable the function pointer tables for '/org/genivi/persistence/adminconsumer' messages
 * @param vtable2 the function pointer tables for '/com/contiautomotive/NodeStateManager/LifecycleConsumer' messages
 * @param vtableFallback the fallback function pointer tables
 * @param userData data to pass to the main loop
 *
 * @return 0
 */
int mainLoop(DBusObjectPathVTable vtable, DBusObjectPathVTable vtable2,
             DBusObjectPathVTable vtableFallback, void* userData);



/**
 * @brief Setup the dbus main dispatching loop
 *
 * @return 0
 */
int setup_dbus_mainloop(void);


#endif /* PERSISTENCE_CLIENT_LIBRARY_DBUS_SERVICE_H_ */
