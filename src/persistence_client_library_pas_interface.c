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

#include "../include_protected/persistence_client_library_data_organization.h"

#include <errno.h>
#include <unistd.h>


static int gTimeoutMs = 50000;

/// flag if access is locked
static int gLockAccess = 0;


int pers_data_sync(void)
{
   sync();
   return 1;
}

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
         uint64_t cmd;
         // add command and data to queue
         cmd = ( ((uint64_t)requestID << 32) | ((uint64_t)request << 16) | CMD_PAS_BLOCK_AND_WRITE_BACK);

         if(-1 == write(gEfds, &cmd, (sizeof(uint64_t))))
         {
            DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("write failed w/ errno "), DLT_INT(errno), DLT_STRING(strerror(errno)));
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
         DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("msg_persAdminRequest => DBus No memory"));
      }

      if (!dbus_connection_send(connection, reply, 0))
      {
         DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("msg_persAdminRequest => DBus No memory"));
      }

      dbus_message_unref(reply);

      return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
   }
   errorReturn = check_pas_request(request, requestID);


   reply = dbus_message_new_method_return(message);

   if (reply == 0)
   {
      DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("msg_persAdminRequest => DBus No memory"));
   }

   if (!dbus_message_append_args(reply, DBUS_TYPE_INT32, &errorReturn, DBUS_TYPE_INVALID))
   {
      DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("msg_persAdminRequest => DBus No memory"));
   }

   if (!dbus_connection_send(connection, reply, 0))
   {
      DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("msg_persAdminRequest => DBus No memory"));
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
         DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("signal_persModeChange => DBus No memory"));
      }

      if (!dbus_connection_send(connection, reply, 0))
      {
         DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("signal_persModeChange => DBus No memory"));
      }

      dbus_message_unref (reply);

      return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
   }

   reply = dbus_message_new_method_return(message);

   if (reply == 0)
   {
      DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("signal_persModeChange => DBus No memory"));
   }

   if (!dbus_message_append_args(reply, DBUS_TYPE_INT32, &errorCode, DBUS_TYPE_INVALID))
   {
      DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("signal_persModeChange => DBus No memory"));
   }

   if (!dbus_connection_send(connection, reply, 0))
   {
      DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("signal_persModeChange => DBus No memory"));
   }

   dbus_connection_flush(connection);
   dbus_message_unref(reply);

   return DBUS_HANDLER_RESULT_HANDLED;
}


DBusHandlerResult checkPersAdminMsg(DBusConnection * connection, DBusMessage * message, void * user_data)
{
   DBusHandlerResult result = DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

   if((0==strcmp("org.genivi.persistence.adminconsumer", dbus_message_get_interface(message))))
   {
      if((0==strcmp("PersistenceAdminRequest", dbus_message_get_member(message))))
      {
         result = msg_persAdminRequest(connection, message);
      }
      else
      {
         DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("checkPersAdminMsg => unknown message"), DLT_STRING(dbus_message_get_member(message)));
      }
   }
   else
   {
      DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("checkPersAdminMsg => unknown message"), DLT_STRING(dbus_message_get_interface(message)));
   }
   return result;
}



int send_pas_register(const char* method, int notificationFlag)
{
   int rval = 0;

   DBusError error;
   dbus_error_init (&error);
   DBusMessage *replyMsg = NULL;
   DBusConnection* conn = get_dbus_connection();

   if(conn != NULL)
   {
      const char* objName = "/org/genivi/persistence/adminconsumer";
      const char* busName = dbus_bus_get_unique_name(conn);

      if(busName != NULL)
      {
         DBusMessage* message = dbus_message_new_method_call("org.genivi.persistence.admin",    // destination
                                                            "/org/genivi/persistence/admin",    // path
                                                             "org.genivi.persistence.admin",    // interface
                                                             method);                           // method

         if(message != NULL)
         {
            dbus_message_append_args(message, DBUS_TYPE_STRING, &busName,  // bus name
                                              DBUS_TYPE_STRING, &objName,
                                              DBUS_TYPE_INT32,  &notificationFlag,
                                              DBUS_TYPE_UINT32, &gTimeoutMs,
                                              DBUS_TYPE_INVALID);

            replyMsg = dbus_connection_send_with_reply_and_block(conn, message, gTimeoutMs, &error);

            if(replyMsg != NULL)
            {
               if(dbus_set_error_from_message(&error, replyMsg))
               {
                  DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("send_pas_register => Access denied"), DLT_STRING(error.message) );
               }
               else
               {
                  dbus_message_get_args(replyMsg, &error, DBUS_TYPE_INT32, &rval, DBUS_TYPE_INVALID);
               }
               dbus_message_unref(replyMsg);
            }
            else
            {
               DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("send_pas_register => reply message is NULL!"), DLT_STRING(error.message) );
            }

            dbus_message_unref(message);
         }
         else
         {
            DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("send_pas_register =>  ERROR: Invalid message") );
            rval = -1;
         }
      }
      else
      {
         DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("send_pas_register =>  ERROR: Invalid busname") );
         rval = -1;
      }
   }
   else
   {
      DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("send_pas_register =>  ERROR: Invalid connection") );
      rval = -1;
   }
   return rval;
}



int send_pas_request(const char* method, unsigned int requestID, int status)
{
   int rval = 0;

   DBusError error;
   dbus_error_init (&error);
   DBusMessage *replyMsg = NULL;
   DBusConnection* conn = get_dbus_connection();

   DBusMessage* message = dbus_message_new_method_call("org.genivi.persistence.admin",    // destination
                                                      "/org/genivi/persistence/admin",    // path
                                                       "org.genivi.persistence.admin",    // interface
                                                       method);                  // method
   if(message != NULL)
   {
      dbus_message_append_args(message, DBUS_TYPE_UINT32, &requestID,
                                        DBUS_TYPE_INT32,  &status,
                                        DBUS_TYPE_INVALID);

      if(conn != NULL)
      {
         replyMsg = dbus_connection_send_with_reply_and_block(conn, message, gTimeoutMs, &error);
         if(replyMsg != NULL)
         {
            if(dbus_set_error_from_message(&error, replyMsg))
            {
               DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("send_pas_request => Access denied"), DLT_STRING(error.message) );
            }
            else
            {
               dbus_message_get_args(replyMsg, &error, DBUS_TYPE_INT32, &rval, DBUS_TYPE_INVALID);
            }
            dbus_message_unref(replyMsg );
         }
         else
         {
            DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("send_pas_request => reply messgae is NULL"), DLT_STRING(error.message) );
         }
      }
      else
      {
         DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("send_pas_request => ERROR: Invalid connection") );
         rval = -1;
      }
      dbus_message_unref(message);
   }
   else
   {
      DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("send_pas_request => ERROR: Invalid message") );
      rval = -1;
   }

   return rval;
}



int register_pers_admin_service(void)
{
   // register for everything
   int notificationFlag = PasMsg_Block | PasMsg_WriteBack | PasMsg_Unblock;

   return send_pas_register("RegisterPersAdminNotification", notificationFlag);
}



int unregister_pers_admin_service(void)
{
   // register for everything
   int notificationFlag = PasMsg_Block | PasMsg_WriteBack | PasMsg_Unblock;

   return send_pas_register("UnRegisterPersAdminNotification", notificationFlag);
}



int pers_admin_service_data_sync_complete(unsigned int requestID, unsigned int status)
{
   return send_pas_request("PersistenceAdminRequestCompleted", requestID, status);
}



void process_block_and_write_data_back(unsigned int requestID, unsigned int status)
{
   // lock persistence data access
   pers_lock_access();
   // sync data back to memory device
   pers_data_sync();
   // send complete notification
   pers_admin_service_data_sync_complete(requestID, status);
}


