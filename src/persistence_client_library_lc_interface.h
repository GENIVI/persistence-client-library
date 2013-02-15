#ifndef PERSISTENCE_CLIENT_LIBRARY_LC_INTERFACE_H
#define PERSISTENCE_CLIENT_LIBRARY_LC_INTERFACE_H

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
 * @file           persistence_client_library_lc_interface.h
 * @ingroup        Persistence client library
 * @author         Ingo Huerner
 * @brief          Implementation of the persistence client library lifecycle interface.
 * @see
 */

#include <dbus/dbus.h>


/**
 * @brief Check if a com.contiautomotive.NodeStateManager.LifecycleConsumer message has been received
 *
 * @param connection the debus connection
 * @param message the dbus message
 * @param user_data data handed over to this function
 *
 * @return DBUS_HANDLER_RESULT_HANDLED or DBUS_HANDLER_RESULT_NOT_YET_HANDLED
 */
DBusHandlerResult checkLifecycleMsg(DBusConnection * connection, DBusMessage * message, void * user_data);


/**
 * @brief send register message 'RegisterShutdownClient' to com.contiautomotive.NodeStateManager.Consumer
 *
 * @return 0 on success or -1 on error
 */
int register_lifecycle();


/**
 * @brief send register message 'UnRegisterShutdownClient' to com.contiautomotive.NodeStateManager.Consumer
 *
 * @return 0 on success or -1 on error
 */
int unregister_lifecycle();


/**
 * @brief process a shutdown message (close all open files, open databases, ...
 *
 * @param requestId the requestID
 */
void process_prepare_shutdown(unsigned char requestId, unsigned int status);



#endif /* PERSISTENCE_CLIENT_LIBRARY_LC_INTERFACE_H */
