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
 * @file           persistence_client_library_pas_interface.c
 * @ingroup        Persistence client library
 * @author         Ingo Huerner
 * @brief          Implementation of the persistence client library persistence
 *                 administration service interface.
 * @see
 */

#include "persistence_client_library_pas_interface.h"
#include "persistence_client_library_dbus_service.h"
#include "persistence_client_library.h"

#include <errno.h>
#include <unistd.h>


/// flag if access is locked
static int gLockAccess = 0;


int pers_data_sync()
{
   return 1;
}

void pers_lock_access()
{
   __sync_fetch_and_add(&gLockAccess,1);
}

void pers_unlock_access()
{
   __sync_fetch_and_sub(&gLockAccess,1);
}

int isAccessLocked()
{
   return gLockAccess;
}


int check_pas_request(int request)
{
   int rval = 0;

   switch(request)
   {
      case (PasMsg_Block|PasMsg_WriteBack):
      {
         // add command to queue
         static const int cmd = CMD_PAS_BLOCK_AND_WRITE_BACK;
         if(sizeof(int)!=write(gPipefds[1], &cmd, sizeof(int)))
         {
            printf("write failed w/ errno %d\n", errno);
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
         rval = PasErrorStatus_FAIL;
   }
   return rval;
}


int msg_persAdminRequest(DBusConnection *connection, DBusMessage *message)
{
   int request = 0,
       msgReturn = 0;

   DBusMessage *reply;
   DBusError error;
   dbus_error_init (&error);

   if (!dbus_message_get_args (message, &error, DBUS_TYPE_UINT16 , &request, DBUS_TYPE_INVALID))
   {
      reply = dbus_message_new_error(message, error.name, error.message);

      if (reply == 0)
      {
         //DLT_LOG(mgrContext, DLT_LOG_ERROR, DLT_STRING("DBus No memory"));
         printf("DBus No memory\n");
      }

      if (!dbus_connection_send(connection, reply, 0))
      {
         //DLT_LOG(mgrContext, DLT_LOG_ERROR, DLT_STRING("DBus No memory"));
         printf("DBus No memory\n");
      }

      dbus_message_unref (reply);

      return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
   }

   msgReturn = check_pas_request(request);

   reply = dbus_message_new_method_return(message);

   if (reply == 0)
   {
     //DLT_LOG(mgrContext, DLT_LOG_ERROR, DLT_STRING("DBus No memory"));
      printf("DBus No memory\n");
   }

   if (!dbus_message_append_args(reply, DBUS_TYPE_INT32, &msgReturn, DBUS_TYPE_INVALID))
   {
     //DLT_LOG(mgrContext, DLT_LOG_ERROR, DLT_STRING("DBus No memory"));
      printf("DBus No memory\n");
   }

   if (!dbus_connection_send(connection, reply, 0))
   {
     //DLT_LOG(mgrContext, DLT_LOG_ERROR, DLT_STRING("DBus No memory"));
      printf("DBus No memory\n");
   }

   dbus_connection_flush(connection);
   dbus_message_unref(reply);

   return DBUS_HANDLER_RESULT_HANDLED;
}





DBusHandlerResult checkPersAdminMsg(DBusConnection * connection, DBusMessage * message, void * user_data)
{
   DBusHandlerResult result = DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

   //printf("handleObjectPathMessage '%s' -> '%s'\n", dbus_message_get_interface(message), dbus_message_get_member(message));
   if((0==strncmp("org.genivi.persistence.admin", dbus_message_get_interface(message), 20)))
   {
      if((0==strncmp("PersistenceAdminRequest", dbus_message_get_member(message), 14)))
      {
         result = msg_persAdminRequest(connection, message);
      }
      else
      {
          printf("checkPersAdminMsg -> unknown message '%s'\n", dbus_message_get_interface(message));
      }
   }
   return result;
}




int send_pas_register(const char* method, const char* appname)
{
   int rval = 0;

   DBusError error;
   dbus_error_init (&error);
   DBusConnection* conn = get_dbus_connection();

   DBusMessage* message = dbus_message_new_method_call("org.genivi.persistence.admin",    // destination
                                                      "/org/genivi/persistence/admin",    // path
                                                       "org.genivi.persistence.admin",    // interface
                                                       method);                  // method
   if(message != NULL)
   {
      dbus_message_append_args(message, DBUS_TYPE_STRING, &appname, DBUS_TYPE_INVALID);


      if(conn != NULL)
      {
         if(!dbus_connection_send(conn, message, 0))
         {
            fprintf(stderr, "send_pers_admin_service ==> Access denied: %s \n", error.message);
         }

         dbus_connection_flush(conn);
      }
      else
      {
         fprintf(stderr, "send_pers_admin_service ==> ERROR: Invalid connection!! \n");
      }
      dbus_message_unref(message);
   }
   else
   {
      fprintf(stderr, "send_pers_admin_service ==> ERROR: Invalid message!! \n");
   }

   return rval;
}



int send_pas_request(const char* method, int blockStatus)
{
   int rval = 0;

   DBusError error;
   dbus_error_init (&error);
   DBusConnection* conn = get_dbus_connection();

   DBusMessage* message = dbus_message_new_method_call("org.genivi.persistence.admin",    // destination
                                                      "/org/genivi/persistence/admin",    // path
                                                       "org.genivi.persistence.admin",    // interface
                                                       method);                  // method
   if(message != NULL)
   {
      dbus_message_append_args(message, DBUS_TYPE_UINT32, &blockStatus, DBUS_TYPE_INVALID);


      if(conn != NULL)
      {
         if(!dbus_connection_send(conn, message, 0))
         {
            fprintf(stderr, "send_pers_admin_service ==> Access denied: %s \n", error.message);
         }

         dbus_connection_flush(conn);
      }
      else
      {
         fprintf(stderr, "send_pers_admin_service ==> ERROR: Invalid connection!! \n");
      }
      dbus_message_unref(message);
   }
   else
   {
      fprintf(stderr, "send_pers_admin_service ==> ERROR: Invalid message!! \n");
   }

   return rval;
}




int register_pers_admin_service()
{
   return send_pas_register("RegisterPersAdminNotification", gAppId);
}


int unregister_pers_admin_service()
{
   return send_pas_register("UnRegisterPersAdminNotification", gAppId);
}


int pers_admin_service_data_sync_complete()
{
   return send_pas_request("PersistenceAdminRequestCompleted", 1);
}


void process_block_and_write_data_back()
{
   // lock persistence data access
   pers_lock_access();
   // sync data back to memory device
   pers_data_sync();
   // send complete notification
   pers_admin_service_data_sync_complete();
}


