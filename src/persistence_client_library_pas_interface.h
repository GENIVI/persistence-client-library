#ifndef PERSISTENCE_CLIENT_LIBRARY_PAS_INTERFACE_H
#define PERSISTENCE_CLIENT_LIBRARY_PAS_INTERFACE_H

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
 * @file           persistence_client_library_pas_interface.h
 * @ingroup        Persistence client library
 * @author         Ingo Huerner
 * @brief          Definition of the persistence client library persistence
 *                 administration service interface.
 * @see
 */

#include "persistence_client_library_dbus_service.h"


/**
 * @brief Check if a org.genivi.persistence.admin message has been received
 *
 * @param connection the debus connection
 * @param message the dbus message
 * @param user_data data handed over to this function
 *
 * @return DBUS_HANDLER_RESULT_HANDLED or DBUS_HANDLER_RESULT_NOT_YET_HANDLED
 */
DBusHandlerResult checkPersAdminMsg(DBusConnection * connection, DBusMessage * message, void * user_data);


/// lock access to persistence data
void pers_lock_access(void);


/// unlock access to persistent data
void pers_unlock_access(void);


/**
 * @brief check if access to persistent data is locked
 *
 * @return 1 if access is locked, 0 if access is possible
 */
int isAccessLocked(void);



/**
 * @brief send registration message 'RegisterPersAdminNotification' to org.genivi.persistence.admin
 *
 * @return 0 on success or -1 on error
 */
int register_pers_admin_service(void);


/**
 * @brief send registration message 'UnRegisterPersAdminNotification' to org.genivi.persistence.admin
 *
 * @return 0 on success or -1 on error
 */
int unregister_pers_admin_service(void);


#endif /* PERSISTENCE_CLIENT_LIBRARY_PAS_INTERFACE_H */
