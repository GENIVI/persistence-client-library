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
 * @file           persistence_client_library_lc_interface.c
 * @ingroup        Persistence client library
 * @author         Ingo Huerner
 * @brief          Implementation of the persistence client library lifecycle interface.
 * @see
 */


#include "persistence_client_library_lc_interface.h"

#include <errno.h>


int check_lc_request(unsigned int request, unsigned int requestID)
{
   int rval = 0;

   switch(request)
   {
      case NsmShutdownNormal:
      {
      	MainLoopData_u data;
      	data.message.cmd = (uint32_t)CMD_LC_PREPARE_SHUTDOWN;
      	data.message.params[0] = request;
      	data.message.params[1] = requestID;
      	data.message.string[0] = '\0'; 	// no string parameter, set to 0

         if(-1 == deliverToMainloop_NM(&data) )
         {
            DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("lcCechkReq - failed write pipe"), DLT_INT(errno));
            rval = NsmErrorStatus_Fail;
         }
         else
         {
            rval = NsmErrorStatus_OK;
         }
         break;
      }
      default:
      {
         DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("lcCechkReq - Unknown lcm message"), DLT_INT(request));
         break;
      }
   }

   return rval;
}


int msg_lifecycleRequest(DBusConnection *connection, DBusMessage *message)
{
   int msgReturn = 0;
   unsigned int request = 0, requestID = 0;

   DBusMessage *reply;
   DBusError error;
   dbus_error_init (&error);

   if (!dbus_message_get_args (message, &error, DBUS_TYPE_UINT32, &request,
                                                DBUS_TYPE_UINT32, &requestID, DBUS_TYPE_INVALID))
   {
      reply = dbus_message_new_error(message, error.name, error.message);

      if (reply == 0)
      {
         DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("msgLcReq - DBus No mem"));
      }

      if (!dbus_connection_send(connection, reply, 0))
      {
         DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("msgLcReq - DBus No mem"));
      }

      dbus_message_unref(reply);

      return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
   }

   msgReturn = check_lc_request(request, requestID);

   reply = dbus_message_new_method_return(message);

   if (reply == 0)
   {
      DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("msgLcReq - DBus No mem"));
   }

   if (!dbus_message_append_args(reply, DBUS_TYPE_INT32, &msgReturn, DBUS_TYPE_INVALID))
   {
      DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("msgLcReq - DBus No mem"));
   }

   if (!dbus_connection_send(connection, reply, 0))
   {
      DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("msgLcReq - DBus No mem"));
   }

   dbus_connection_flush(connection);
   dbus_message_unref(reply);

   return DBUS_HANDLER_RESULT_HANDLED;
}



DBusHandlerResult checkLifecycleMsg(DBusConnection * connection, DBusMessage * message, void * user_data)
{
   DBusHandlerResult result = DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

   (void)user_data;

   if((0==strncmp(gDbusLcConsterface, dbus_message_get_interface(message), 46)))
   {
   	DLT_LOG(gPclDLTContext, DLT_LOG_DEBUG, DLT_STRING("chLcMsg - Received dbus msg: "), DLT_STRING(dbus_message_get_member(message)));
      if((0==strncmp(gDbusLcConsMsg, dbus_message_get_member(message), 16)))
      {
         result = msg_lifecycleRequest(connection, message);
      }
      else
      {
          DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("chLcMsg - unknown msg "), DLT_STRING(dbus_message_get_interface(message)));
      }
   }
   return result;
}



int register_lifecycle(int shutdownMode)
{
	MainLoopData_u data;

	data.message.cmd = (uint32_t)CMD_SEND_LC_REGISTER;
	data.message.params[0] = 1;
	data.message.params[1] = shutdownMode;
	data.message.string[0] = '\0'; 	// no string parameter, set to 0

   return deliverToMainloop(&data);
}



int unregister_lifecycle(int shutdownMode)
{
	MainLoopData_u data;

	data.message.cmd = (uint32_t)CMD_SEND_LC_REGISTER;
	data.message.params[0] = 0;
	data.message.params[1] = shutdownMode;
	data.message.string[0] = '\0'; 	// no string parameter, set to 0

   return deliverToMainloop(&data);
}
