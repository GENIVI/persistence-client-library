#ifndef PERSISTENCE_CLIENT_LIBRARY_DBUS_SERVICE_H_
#define PERSISTENCE_CLIENT_LIBRARY_DBUS_SERVICE_H_

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
 * @file           persistence_client_library_dbus_service.h
 * @ingroup        Persistence client library
 * @author         Ingo Huerner
 * @brief          Header of the persistence client library dbus service.
 * @see
 */

#include <dbus/dbus.h>
#include <poll.h>
#include <pthread.h>
#include <sys/eventfd.h>
#include <sys/timerfd.h>

/// mutex to make sure main loop is running
extern pthread_mutex_t gDbusInitializedMtx;
extern pthread_cond_t  gDbusInitializedCond;
extern pthread_mutex_t gDbusPendingRegMtx;

extern pthread_mutex_t gMainLoopMtx;

// declared in persistence_client_library_dbus_service.c
// used to end dbus library
extern int bContinue;

extern pthread_t gMainLoopThread;


/// command definitions for main loop
typedef enum ECmd
{
   CMD_NONE = 0,                    /// command none
   CMD_PAS_BLOCK_AND_WRITE_BACK,    /// command block access and write data back
   CMD_LC_PREPARE_SHUTDOWN,         /// command to prepare shutdown
   CMD_SEND_NOTIFY_SIGNAL,          /// command send changed notification signal
   CMD_REG_NOTIFY_SIGNAL,           /// command send register/unregister command
   //CMD_SEND_PAS_REQUEST,            /// command send admin request
   CMD_SEND_PAS_REGISTER,           /// command send admin register/unregister
   //CMD_SEND_LC_REQUEST,             /// command send lifecycle request
   CMD_SEND_LC_REGISTER,            /// command send lifecycle register/unregister
   CMD_QUIT                         /// quit command
} tCmd;


/// pipe file descriptors
extern int gEfds;


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


int deliverToMainloop(tCmd mainloopCmd, unsigned int param1, unsigned int param2);


int deliverToMainloop_NM(tCmd mainloopCmd, unsigned int param1, unsigned int param2);


#endif /* PERSISTENCE_CLIENT_LIBRARY_DBUS_SERVICE_H_ */
