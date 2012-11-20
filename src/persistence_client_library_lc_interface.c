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
 * @file           persistence_client_library_lc_interface.c
 * @ingroup        Persistence client library
 * @author         Ingo Huerner
 * @brief          Implementation of the persistence client library lifecycle interface.
 * @see
 */


#include "persistence_client_library_lc_interface.h"

#include "persistence_client_library.h"
#include "persistence_client_library_handle.h"
#include "persistence_client_library_pas_interface.h"
#include "persistence_client_library_dbus_service.h"
#include "persistence_client_library_custom_loader.h"
#include "persistence_client_library_access_helper.h"
#include "persistence_client_library_data_access.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dlfcn.h>

static int gTimeoutMs = 500;

int check_lc_request(int request)
{
   int rval = 0;

   switch(request)
   {
      case NsmShutdownNormal:
      {
         // add command and data to queue
         unsigned long cmd = ( (request << 8) | CMD_LC_PREPARE_SHUTDOWN);

         if(sizeof(int)!=write(gPipefds[1], &cmd, sizeof(unsigned long)))
         {
            printf("write failed w/ errno %d\n", errno);
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
         printf("Unknown lifecycle message!\n");
         break;
      }
   }

   return rval;
}


int msg_lifecycleRequest(DBusConnection *connection, DBusMessage *message)
{
   int request   = 0,
       requestId = 0,
       msgReturn = 0;

   DBusMessage *reply;
   DBusError error;
   dbus_error_init (&error);

   if (!dbus_message_get_args (message, &error, DBUS_TYPE_UINT32, &request,
                                                DBUS_TYPE_UINT32, &requestId,
                                                DBUS_TYPE_INVALID))
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

   msgReturn = check_lc_request(request);

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




DBusHandlerResult checkLifecycleMsg(DBusConnection * connection, DBusMessage * message, void * user_data)
{
   DBusHandlerResult result = DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

   //printf("handleObjectPathMessage '%s' -> '%s'\n", dbus_message_get_interface(message), dbus_message_get_member(message));
   if((0==strncmp("com.contiautomotive.NodeStateManager.LifecycleConsumer", dbus_message_get_interface(message), 20)))
   {
      if((0==strncmp("NSMLifecycleRequest", dbus_message_get_member(message), 18)))
      {
         result = msg_lifecycleRequest(connection, message);
      }
      else
      {
          printf("checkLifecycleMsg -> unknown message '%s'\n", dbus_message_get_interface(message));
      }
   }
   return result;
}



int send_lifecycle_register(const char* method, int shutdownMode)
{
   int rval = 0;

   DBusError error;
   dbus_error_init (&error);
   DBusConnection* conn = get_dbus_connection();

   const char* objName = "/com/contiautomotive/NodeStateManager/LifecycleConsumer";
   const char* busName = dbus_bus_get_unique_name(conn);

   DBusMessage* message = dbus_message_new_method_call("com.contiautomotive.NodeStateManager.Consumer",  // destination
                                                       "/com/contiautomotive/NodeStateManager/Consumer",  // path
                                                       "com.contiautomotive.NodeStateManager.Consumer",  // interface
                                                       method);                  // method
   if(message != NULL)
   {
      dbus_message_append_args(message, DBUS_TYPE_STRING, &busName,
                                        DBUS_TYPE_STRING, &objName,
                                        DBUS_TYPE_INT32, &shutdownMode,
                                        DBUS_TYPE_INT32, &gTimeoutMs,
                                        DBUS_TYPE_INVALID);

      if(conn != NULL)
      {
         if(!dbus_connection_send(conn, message, 0))
         {
            fprintf(stderr, "send_lifecycle ==> Access denied: %s \n", error.message);
         }

         dbus_connection_flush(conn);
      }
      else
      {
         fprintf(stderr, "send_lifecycle ==> ERROR: Invalid connection!! \n");
      }
      dbus_message_unref(message);
   }
   else
   {
      fprintf(stderr, "send_lifecycle ==> ERROR: Invalid message!! \n");
   }

   return rval;
}



int send_lifecycle_request(const char* method, int requestId, int status)
{
   int rval = 0;

   DBusError error;
   dbus_error_init (&error);

   DBusConnection* conn = get_dbus_connection();

   DBusMessage* message = dbus_message_new_method_call("com.contiautomotive.NodeStateManager.Consumer",  // destination
                                                      "/com/contiautomotive/NodeStateManager/Consumer",  // path
                                                       "com.contiautomotive.NodeStateManager.Consumer",  // interface
                                                       method);                  // method
   if(message != NULL)
   {
      dbus_message_append_args(message, DBUS_TYPE_INT32, &requestId,
                                        DBUS_TYPE_INT32, &status,
                                        DBUS_TYPE_INVALID);

      if(conn != NULL)
      {
         if(!dbus_connection_send(conn, message, 0))
         {
            fprintf(stderr, "send_lifecycle ==> Access denied: %s \n", error.message);
         }

         dbus_connection_flush(conn);
      }
      else
      {
         fprintf(stderr, "send_lifecycle ==> ERROR: Invalid connection!! \n");
      }
      dbus_message_unref(message);
   }
   else
   {
      fprintf(stderr, "send_lifecycle ==> ERROR: Invalid message!! \n");
   }

   return rval;
}



int register_lifecycle()
{
   int shutdownMode = 88;  // TODO send correct mode

   return send_lifecycle_register("RegisterShutdownClient", shutdownMode);
}



int unregister_lifecycle()
{
   int shutdownMode = 88;     // TODO send correct mode

   return send_lifecycle_register("UnRegisterShutdownClient", shutdownMode);
}


int send_prepare_shutdown_complete(int requestId)
{
   int status    = 1;   // TODO send correct status

   return send_lifecycle_request("LifecycleRequestComplete", requestId, status);
}




void process_prepare_shutdown(unsigned char requestId)
{
   int i = 0;
   GvdbTable* resourceTable = NULL;

   // block write
   pers_lock_access();

   // flush open files to disk
   for(i=0; i<maxPersHandle; i++)
   {
      if(gOpenFdArray[i] == FileOpen)
      {
         fsync(i);
         close(i);
      }
   }

   // close open gvdb persistence resource configuration table
   for(i=0; i< PersistenceRCT_LastEntry; i++)
   {
     resourceTable = get_resource_cfg_table_by_idx(i);
     // dereference opend database
     if(resourceTable != NULL)
     {
        gvdb_table_unref(resourceTable);
     }
   }

   //close opend database
   database_close(PersistenceStorage_local, PersistencePolicy_wc);
   database_close(PersistenceStorage_local, PersistencePolicy_wt);
   database_close(PersistenceStorage_shared, PersistencePolicy_wc);
   database_close(PersistenceStorage_shared, PersistencePolicy_wt);


   // unload custom client libraries
   for(i=0; i<get_num_custom_client_libs(); i++)
   {
      // deinitialize plugin
      gPersCustomFuncs[i].custom_plugin_deinit();
      // close library handle
      dlclose(gPersCustomFuncs[i].handle);
   }

   // notify lifecycle shutdown OK
   send_prepare_shutdown_complete((int)requestId);
}

