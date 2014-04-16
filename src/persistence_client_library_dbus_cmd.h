#ifndef PERSISTENCE_CLIENT_LIBRARY_DBUS_CMD_H_
#define PERSISTENCE_CLIENT_LIBRARY_DBUS_CMD_H_

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
 * @file           persistence_client_library_dbus_cmd.h
 * @ingroup        Persistence client library
 * @author         Ingo Huerner
 * @brief          Header of the persistence client library dbus command functions.
 * @see
 */

#include <dbus/dbus.h>

#include "persistence_client_library_dbus_service.h"

/**
 * @brief process a shutdown message (close all open files, open databases, ...
 */
void process_prepare_shutdown(int complete);


/**
 * @brief block persistence access and write data back to device
 *
 * @param requestId the requestID
 * @param status the status
 */
void process_block_and_write_data_back(unsigned int requestID, unsigned int status);


/**
 * @brief send notification signal
 *
 * @param conn the dbus connection
 */
void process_send_notification_signal(DBusConnection* conn);


/**
 * @brief register for notification signal
 *
 * @param conn the dbus connection
 */
void process_reg_notification_signal(DBusConnection* conn);



/**
 * @brief process a request of the persistence admin service
 *
 * @param requestId the requestID
 * @param status the status
 */
void process_send_pas_request(DBusConnection* conn, unsigned int requestID, int status);


/**
 * @brief process a request of the persistence admin service
 *
 * @param regType the registration type (1 for register; 0 for unregister)
 * @param notification flag the notificatin flag
 */
void process_send_pas_register(DBusConnection* conn, int regType, int notificationFlag);



void process_send_lifecycle_request(DBusConnection* conn, int requestId, int status);


void process_send_lifecycle_register(DBusConnection* conn, int regType, int shutdownMode);



#endif /* PERSISTENCE_CLIENT_LIBRARY_DBUS_CMD_H_ */
