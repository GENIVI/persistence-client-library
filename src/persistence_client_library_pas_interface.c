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
 * @file           persistence_client_library_pas_interface.c
 * @ingroup        Persistence client library
 * @author         Ingo Huerner
 * @brief          Implementation of the persistence client library persistence
 *                 administration service interface.
 * @see
 */

#include "persistence_client_library_pas_interface.h"
#include "persistence_client_library_dbus_service.h"

#include "persistence_client_library_data_organization.h"

#include <errno.h>
#include <unistd.h>
#include <string.h>

/// flag if access is locked
static int gLockAccess = 0;

void pers_lock_access(void)
{
   __sync_fetch_and_add(&gLockAccess,1);
}

void pers_unlock_access(void)
{
   __sync_fetch_and_sub(&gLockAccess,1);
}

int isAccessLocked(void)
{
   return gLockAccess;
}


int check_pas_request(unsigned int request, unsigned int requestID)
{
   int rval = 0;
   switch(request)
   {
      case (PasMsg_Block|PasMsg_WriteBack):
      {
      	tMainLoopData data;

      	data.message.cmd = (uint32_t)CMD_PAS_BLOCK_AND_WRITE_BACK;
      	data.message.params[0] = request;
      	data.message.params[1] = requestID;
      	data.message.string[0] = '\0'; 	// no string parameter, set to 0

         DLT_LOG(gPclDLTContext, DLT_LOG_INFO, DLT_STRING("PCL: check_pas_request; case PasMsg_Block o. PasMsg_WriteBack"));
         if(-1 == deliverToMainloop_NM(&data))
         {
            DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("write failed w/ errno "), DLT_INT(errno), DLT_STRING(strerror(errno)));
            rval = PasErrorStatus_FAIL;
         }
         else
         {
            rval = PasErrorStatus_RespPend;
         }
         break;
      }
      case PasMsg_Unblock:
      {
         DLT_LOG(gPclDLTContext, DLT_LOG_INFO, DLT_STRING("PCL: check_pas_request; case PasMsg_Unblock"));
         pers_unlock_access();
         rval = PasErrorStatus_OK;
         break;
      }
      default:
      {
         rval = PasErrorStatus_FAIL;
         break;
      }
   }
   return rval;
}





DBusHandlerResult msg_persAdminRequest(DBusConnection *connection, DBusMessage *message)
{
   int request = 0, requestID = 0;
   int errorReturn = 0;

   DBusMessage *reply;
   DBusError error;
   dbus_error_init (&error);


   if (!dbus_message_get_args(message, &error, DBUS_TYPE_INT32 , &request,
                                               DBUS_TYPE_INT32 , &requestID,
                                               DBUS_TYPE_INVALID))
   {
      reply = dbus_message_new_error(message, error.name, error.message);

      if(reply == 0)
      {
         DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("msg_persAdminRequest => DBus No memory"));
      }

      if (!dbus_connection_send(connection, reply, 0))
      {
         DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("msg_persAdminRequest => DBus No memory"));
      }

      dbus_message_unref(reply);

      return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
   }
   errorReturn = check_pas_request(request, requestID);


   reply = dbus_message_new_method_return(message);

   if (reply == 0)
   {
      DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("msg_persAdminRequest => DBus No memory"));
   }

   if (!dbus_message_append_args(reply, DBUS_TYPE_INT32, &errorReturn, DBUS_TYPE_INVALID))
   {
      DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("msg_persAdminRequest => DBus No memory"));
   }

   if (!dbus_connection_send(connection, reply, 0))
   {
      DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("msg_persAdminRequest => DBus No memory"));
   }

   dbus_connection_flush(connection);
   dbus_message_unref(reply);

   return DBUS_HANDLER_RESULT_HANDLED;
}


int signal_persModeChange(DBusConnection *connection, DBusMessage *message)
{
   int persistenceMode = 0;
   int errorCode = 0;

   DBusMessage *reply;
   DBusError error;
   dbus_error_init (&error);

   if (!dbus_message_get_args (message, &error, DBUS_TYPE_INT32 , &persistenceMode,
                                                DBUS_TYPE_INVALID))
   {
      reply = dbus_message_new_error(message, error.name, error.message);

      if(reply == 0)
      {
         DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("signal_persModeChange => DBus No memory"));
      }

      if (!dbus_connection_send(connection, reply, 0))
      {
         DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("signal_persModeChange => DBus No memory"));
      }

      dbus_message_unref(reply);

      return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
   }

   reply = dbus_message_new_method_return(message);

   if (reply == 0)
   {
      DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("signal_persModeChange => DBus No memory"));
   }

   if (!dbus_message_append_args(reply, DBUS_TYPE_INT32, &errorCode, DBUS_TYPE_INVALID))
   {
      DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("signal_persModeChange => DBus No memory"));
   }

   if (!dbus_connection_send(connection, reply, 0))
   {
      DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("signal_persModeChange => DBus No memory"));
   }

   dbus_connection_flush(connection);
   dbus_message_unref(reply);

   return DBUS_HANDLER_RESULT_HANDLED;
}


DBusHandlerResult checkPersAdminMsg(DBusConnection * connection, DBusMessage * message, void * user_data)
{
   DBusHandlerResult result = DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

   (void)user_data;

   printf("checkPersAdminMsg => \n  interface: %s\n  Message: %s \n", dbus_message_get_interface(message), dbus_message_get_member(message));

   if((0==strcmp(gDbusPersAdminConsInterface, dbus_message_get_interface(message))))
   {
      if((0==strcmp(gDbusPersAdminConsMsg, dbus_message_get_member(message))))
      {
         result = msg_persAdminRequest(connection, message);
      }
      else
      {
         DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("checkPersAdminMsg => unknown message"), DLT_STRING(dbus_message_get_member(message)));
      }
   }
   else
   {
      DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("checkPersAdminMsg => unknown message"), DLT_STRING(dbus_message_get_interface(message)));
   }
   return result;
}




int register_pers_admin_service(void)
{
   int rval =  0;

	tMainLoopData data;

	data.message.cmd = (uint32_t)CMD_SEND_PAS_REGISTER;
	data.message.params[0] = 1;
	data.message.params[1] = (PasMsg_Block | PasMsg_WriteBack | PasMsg_Unblock);
	data.message.string[0] = '\0'; 	// no string parameter, set to 0

   if(-1 == deliverToMainloop(&data))
   {
    DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("register_pers_admin_service => failed to write to pipe"), DLT_INT(errno));
    rval = -1;
   }
   else
   {
      pthread_mutex_lock(&gDbusPendingRegMtx);   // block until pending received
      rval = gDbusPendingRvalue;
   }
   return rval;
}



int unregister_pers_admin_service(void)
{
   int rval =  0;

	tMainLoopData data;
	data.message.cmd = (uint32_t)CMD_SEND_PAS_REGISTER;
	data.message.params[0] = 0;
	data.message.params[1] = (PasMsg_Block | PasMsg_WriteBack | PasMsg_Unblock);
	data.message.string[0] = '\0'; 	// no string parameter, set to 0

   if(-1 == deliverToMainloop(&data))
   {
     DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("unregister_pers_admin_service => failed to write to pipe"), DLT_INT(errno));
     rval = -1;
   }
   else
   {
      pthread_mutex_lock(&gDbusPendingRegMtx);   // block until pending received
      rval = gDbusPendingRvalue;
   }
   return rval;
}


