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

#include "../include_protected/persistence_client_library_data_organization.h"
#include "../include_protected/persistence_client_library_db_access.h"

#include "persistence_client_library_handle.h"
#include "persistence_client_library_pas_interface.h"
#include "persistence_client_library_dbus_service.h"
#include "persistence_client_library_custom_loader.h"
#include "persistence_client_library_prct_access.h"
#include "persistence_client_library_itzam_errors.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dlfcn.h>


int check_lc_request(int request, int requestID)
{
   int rval = 0;

   switch(request)
   {
      case NsmShutdownNormal:
      {
         if(-1 == deliverToMainloop_NM(CMD_LC_PREPARE_SHUTDOWN, request, requestID) )
         {
            DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("check_lc_request => failed to write to pipe"), DLT_INT(errno));
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
         DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("check_lc_request => Unknown lifecycle message"), DLT_INT(request));
         break;
      }
   }

   return rval;
}


int msg_lifecycleRequest(DBusConnection *connection, DBusMessage *message)
{
   int request   = 0,
       requestID = 0,
       msgReturn = 0;

   DBusMessage *reply;
   DBusError error;
   dbus_error_init (&error);

   if (!dbus_message_get_args (message, &error, DBUS_TYPE_UINT32, &request,
                                                DBUS_TYPE_UINT32, &requestID,
                                                DBUS_TYPE_INVALID))
   {
      reply = dbus_message_new_error(message, error.name, error.message);

      if (reply == 0)
      {
         DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("msg_lifecycleRequest => DBus No memory"));
      }

      if (!dbus_connection_send(connection, reply, 0))
      {
         DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("msg_lifecycleRequest => DBus No memory"));
      }

      dbus_message_unref(reply);

      return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
   }

   msgReturn = check_lc_request(request, requestID);

   reply = dbus_message_new_method_return(message);

   if (reply == 0)
   {
      DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("msg_lifecycleRequest => DBus No memory"));
   }

   if (!dbus_message_append_args(reply, DBUS_TYPE_INT32, &msgReturn, DBUS_TYPE_INVALID))
   {
      DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("msg_lifecycleRequest => DBus No memory"));
   }

   if (!dbus_connection_send(connection, reply, 0))
   {
      DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("msg_lifecycleRequest => DBus No memory"));
   }

   dbus_connection_flush(connection);
   dbus_message_unref(reply);

   return DBUS_HANDLER_RESULT_HANDLED;
}



DBusHandlerResult checkLifecycleMsg(DBusConnection * connection, DBusMessage * message, void * user_data)
{
   DBusHandlerResult result = DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

   if((0==strncmp("org.genivi.NodeStateManager.LifeCycleConsumer", dbus_message_get_interface(message), 46)))
   {
      if((0==strncmp("LifecycleRequest", dbus_message_get_member(message), 16)))
      {
         result = msg_lifecycleRequest(connection, message);
      }
      else
      {
          DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("checkLifecycleMsg -> unknown message "), DLT_STRING(dbus_message_get_interface(message)));
      }
   }
   return result;
}



int register_lifecycle(int shutdownMode)
{
   return deliverToMainloop(CMD_SEND_LC_REGISTER, 1, shutdownMode);
}



int unregister_lifecycle(int shutdownMode)
{
   return deliverToMainloop(CMD_SEND_LC_REGISTER, 0, shutdownMode);
}



int send_prepare_shutdown_complete(int requestId, int status)
{
   return deliverToMainloop_NM(CMD_SEND_LC_REQUEST, status, requestId);
}

