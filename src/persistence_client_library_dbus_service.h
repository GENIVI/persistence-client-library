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

#include "persistence_client_library_data_organization.h"

/// mutex to make sure main loop is running
extern pthread_mutex_t gDbusInitializedMtx;
extern pthread_cond_t  gDbusInitializedCond;
extern pthread_mutex_t gDbusPendingRegMtx;

extern pthread_mutex_t gMainCondMtx;

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
   CMD_SEND_PAS_REGISTER,           /// command send admin register/unregister
   CMD_SEND_LC_REGISTER,            /// command send lifecycle register/unregister
   CMD_QUIT                         /// quit command
} tCmd;



/// lifecycle consumer interface dbus name
extern const char* gDbusLcConsterface;
/// lifecycle consumer dbus interface
extern const char* gDbusLcCons;
/// lifecycle consumer dbus destination
extern const char* gDbusLcConsDest;
/// lifecycle consumer dbus path
extern const char* gDbusLcConsPath;
/// lifecycle consumer debus message
extern const char* gDbusLcConsMsg;
/// lifecycle consumer dbus interface
extern const char* gDbusLcInterface;

/// persistence administrator consumer dbus interface
extern const char* gDbusPersAdminConsInterface;
/// persistence administrator consumer dbus
extern const char* gDbusPersAdminPath;
/// persistence administrator consumer dbus interface message
extern const char* gDbusPersAdminConsMsg;
/// persistence administrator dbus
extern const char* gDbusPersAdminInterface;
/// persistence administrator dbus path
extern const char* gPersAdminConsumerPath;


/// command data union definition
typedef union MainLoopData{
	struct {
		uint32_t cmd;				/// dbus mainloop command
		uint32_t params[4];	/// unsignet int parameters
		char string[DbKeyMaxLen];			/// char parameter
	} message;
	char payload[128];
} tMainLoopData;


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


int deliverToMainloop(tMainLoopData* payload);


int deliverToMainloop_NM(tMainLoopData* payload);


#endif /* PERSISTENCE_CLIENT_LIBRARY_DBUS_SERVICE_H_ */
