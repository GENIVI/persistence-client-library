#ifndef PERSISTENCE_CLIENT_LIBRARY_PAS_INTERFACE_H
#define PERSISTENCE_CLIENT_LIBRARY_PAS_INTERFACE_H

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
 * @file           persistence_client_library_pas_interface.h
 * @ingroup        Persistence client library
 * @author         Ingo Huerner
 * @brief          Definition of the persistence client library persistence
 *                 administration service interface.
 * @see
 */

#include <dbus/dbus.h>


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

//DBusHandlerResult checkPersAdminSignal(DBusConnection * connection, DBusMessage * message, void * user_data);


int signal_persModeChange(DBusConnection *connection, DBusMessage *message);

/// synchronize data back to memory device
int pers_data_sync(void);


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


/// block persistence access and write data back to device
void process_block_and_write_data_back(void);


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
