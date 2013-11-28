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
 * @file           persistence_client_library_dbus_cmd.c
 * @ingroup        Persistence client library
 * @author         Ingo Huerner
 * @brief          Implementation of the persistence client library dbus commands.
 * @see
 */

#include "persistence_client_library_dbus_cmd.h"

#include "persistence_client_library_handle.h"
#include "persistence_client_library_itzam_errors.h"
#include "persistence_client_library_custom_loader.h"
#include "persistence_client_library_prct_access.h"

#include "../include_protected/persistence_client_library_data_organization.h"
#include "../include_protected/persistence_client_library_db_access.h"

#include <itzam.h>


// function prototype
void msg_pending_func(DBusPendingCall *call, void *data);



void process_reg_notification_signal(DBusConnection* conn)
{
   char ruleChanged[DbusMatchRuleSize] = {0};
   char ruleDeleted[DbusMatchRuleSize] = {0};
   char ruleCreated[DbusMatchRuleSize] = {0};

   // add match for  c h a n g e
   snprintf(ruleChanged, DbusMatchRuleSize,
            "type='signal',interface='org.genivi.persistence.adminconsumer',member='PersistenceResChange',path='/org/genivi/persistence/adminconsumer',arg0='%s',arg1='%u',arg2='%u',arg3='%u'",
            gNotifykey, gNotifyLdbid, gNotifyUserNo, gNotifySeatNo);

   // add match for  d e l e t e
   snprintf(ruleDeleted, DbusMatchRuleSize,
            "type='signal',interface='org.genivi.persistence.adminconsumer',member='PersistenceResDelete',path='/org/genivi/persistence/adminconsumer',arg0='%s',arg1='%u',arg2='%u',arg3='%u'",
            gNotifykey, gNotifyLdbid, gNotifyUserNo, gNotifySeatNo);

   // add match for  c r e a t e
   snprintf(ruleCreated, DbusMatchRuleSize,
            "type='signal',interface='org.genivi.persistence.adminconsumer',member='PersistenceResCreate',path='/org/genivi/persistence/adminconsumer',arg0='%s',arg1='%u',arg2='%u',arg3='%u'",
            gNotifykey, gNotifyLdbid, gNotifyUserNo, gNotifySeatNo);

   if(gNotifyPolicy == Notify_register)
   {
      dbus_bus_add_match(conn, ruleChanged, NULL);
      dbus_bus_add_match(conn, ruleDeleted, NULL);
      dbus_bus_add_match(conn, ruleCreated, NULL);
   }
   else if(gNotifyPolicy == Notify_unregister)
   {
      dbus_bus_remove_match(conn, ruleChanged, NULL);
      dbus_bus_remove_match(conn, ruleDeleted, NULL);
      dbus_bus_remove_match(conn, ruleCreated, NULL);
   }

   dbus_connection_flush(conn);  // flush the connection to add the match
}



void process_send_notification_signal(DBusConnection* conn)
{
   dbus_bool_t ret;
   DBusMessage* message;
   const char* notifyReason = NULL;

   char ldbidArray[DbusSubMatchSize] = {0};
   char userArray[DbusSubMatchSize]  = {0};
   char seatArray[DbusSubMatchSize]  = {0};
   char* pldbidArra = ldbidArray;
   char* puserArray = userArray;
   char* pseatArray = seatArray;
   char* pnotifyKey = gNotifykey;

   switch(gNotifyReason)
   {
      case pclNotifyStatus_deleted:
         notifyReason = gDeleteSignal;
         break;
      case  pclNotifyStatus_created:
         notifyReason = gCreateSignal;
         break;
      case pclNotifyStatus_changed:
         notifyReason = gChangeSignal;
         break;
      default:
         notifyReason = NULL;
         break;
   }

   if(notifyReason != NULL)
   {
      // dbus_bus_add_match is used for the notification mechanism,
      // and this works only for type DBUS_TYPE_STRING as message arguments
      // this is the reason to use string instead of integer types directly
      snprintf(ldbidArray, DbusSubMatchSize, "%u", gNotifyLdbid);
      snprintf(userArray,  DbusSubMatchSize, "%u", gNotifyUserNo);
      snprintf(seatArray,  DbusSubMatchSize, "%u", gNotifySeatNo);

      //printf("process_send_Notification_Signal => key: %s | lbid: %d | gUserNo: %d | gSeatNo: %d | gReason: %d \n", gNotifykey, gLdbid, gUserNo, gSeatNo, gReason);
      message = dbus_message_new_signal("/org/genivi/persistence/adminconsumer",    // const char *path,
                                        "org.genivi.persistence.adminconsumer",     // const char *interface,
                                        notifyReason);                                 // const char *name

      ret = dbus_message_append_args(message, DBUS_TYPE_STRING, &pnotifyKey,
                                              DBUS_TYPE_STRING, &pldbidArra,
                                              DBUS_TYPE_STRING, &puserArray,
                                              DBUS_TYPE_STRING, &pseatArray,
                                              DBUS_TYPE_INVALID);
      if(ret == TRUE)
      {
         // Send the signal
         if(conn != NULL)
         {
            if(dbus_connection_send(conn, message, 0) == TRUE)
            {
               // Free the signal now we have finished with it
               dbus_message_unref(message);
            }
            else
            {
               DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("process_send_Notification_Signal ==> failed to send dbus message!!"));
            }
         }
         else
         {
            DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("process_send_Notification_Signal ==> E R R O R  C O N E C T I O N  NULL!!"));
         }
      }
      else
      {
         DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("process_send_Notification_Signal ==> ERROR dbus_message_append_args"));
      }
   }
   else
   {
      DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("process_send_Notification_Signal ==> ERROR invalid notification reason"));
   }
}



void process_block_and_write_data_back(unsigned int requestID, unsigned int status)
{
   // lock persistence data access
   pers_lock_access();
   // sync data back to memory device
   pers_data_sync();
   // send complete notification
   //pers_admin_service_data_sync_complete(requestID, status);
}



void process_prepare_shutdown(unsigned char requestId, unsigned int status)
{
   int i = 0;
   itzam_btree* resourceTable = NULL;
   itzam_state  state = ITZAM_FAILED;

   // block write
   pers_lock_access();

   // flush open files to disk
   for(i=0; i<MaxPersHandle; i++)
   {
      int tmp = i;
      if(gOpenFdArray[tmp] == FileOpen)
      {
         fsync(tmp);
         close(tmp);
      }
   }

   // close open gvdb persistence resource configuration table
   for(i=0; i< PrctDbTableSize; i++)
   {
     resourceTable = get_resource_cfg_table_by_idx(i);
     // dereference opend database
     if(resourceTable != NULL &&  get_resource_cfg_table_status(i) == 1)
     {
        state = itzam_btree_close(resourceTable);
        invalidate_resource_cfg_table(i);
        if (state != ITZAM_OKAY)
        {
           DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("process_prepare_shutdown => itzam_btree_close: Itzam problem"), DLT_STRING(STATE_MESSAGES[state]));
        }
     }
   }

   //close opend database
   pers_db_close_all();


   // unload custom client libraries
   for(i=0; i<PersCustomLib_LastEntry; i++)
   {
      if(gPersCustomFuncs[i].custom_plugin_deinit != NULL)
      {
         // deinitialize plugin
         gPersCustomFuncs[i].custom_plugin_deinit();
         // close library handle
         dlclose(gPersCustomFuncs[i].handle);

         invalidate_custom_plugin(i);
      }
   }

   // notify lifecycle shutdown OK
   //send_prepare_shutdown_complete((int)requestId, (int)status);
}



void process_send_pas_request(DBusConnection* conn, unsigned int requestID, int status)
{
   DBusError error;
   DBusPendingCall* pending = NULL;
   dbus_error_init (&error);
   int rval = 0;

   DBusMessage* message = dbus_message_new_method_call("org.genivi.persistence.admin",       // destination
                                                      "/org/genivi/persistence/admin",       // path
                                                       "org.genivi.persistence.admin",       // interface
                                                       "PersistenceAdminRequestCompleted");  // method
   if(message != NULL)
   {
      dbus_message_append_args(message, DBUS_TYPE_UINT32, &requestID,
                                        DBUS_TYPE_INT32,  &status,
                                        DBUS_TYPE_INVALID);

      if(conn != NULL)
      {
         //replyMsg = dbus_connection_send_with_reply_and_block(conn, message, gTimeoutMs, &error);
         dbus_connection_send_with_reply(conn,        //    the connection
                                         message,       // the message to write
                                         &pending,      // pending
                                         gTimeoutMs);   // timeout in milliseconds or -1 for default

         dbus_connection_flush(conn);

         if(!dbus_pending_call_set_notify(pending, msg_pending_func, "PersistenceAdminRequestCompleted", NULL))
         {
            DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("process_send_pas_request => dbus_pending_call_set_notify: FAILED\n"));
         }
         dbus_pending_call_unref(pending);
      }
      else
      {
         DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("send_pas_request => ERROR: Invalid connection") );
      }
      dbus_message_unref(message);
   }
   else
   {
      DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("send_pas_request => ERROR: Invalid message") );
   }
}


void process_send_pas_register(DBusConnection* conn, int regType, int notificationFlag)
{
   DBusError error;
   dbus_error_init (&error);
   DBusPendingCall* pending = NULL;

   char* method = NULL;

   if(regType == 0)
      method = "UnRegisterPersAdminNotification";
   else if(regType == 1)
      method = "RegisterPersAdminNotification";

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

            dbus_connection_send_with_reply(conn,           //    the connection
                                            message,        // the message to write
                                            &pending,       // pending
                                            gTimeoutMs);    // timeout in milliseconds or -1 for default

            dbus_connection_flush(conn);

            if(!dbus_pending_call_set_notify(pending, msg_pending_func, method, NULL))
            {
               printf("process_send_pas_register => dbus_pending_call_set_notify: FAILED\n");
            }
            dbus_pending_call_unref(pending);
         }
         else
         {
            DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("send_pas_register =>  ERROR: Invalid message") );
         }
         dbus_message_unref(message);
      }
      else
      {
         DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("send_pas_register =>  ERROR: Invalid busname") );
      }
   }
   else
   {
      DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("send_pas_register =>  ERROR: Invalid connection") );
   }
}


void process_send_lifecycle_register(DBusConnection* conn, int regType, int shutdownMode)
{
   int rval = 0;

   DBusError error;
   dbus_error_init (&error);

   char* method = NULL;

   if(regType == 1)
      method = "RegisterShutdownClient";
   else if(regType == 0)
      method = "UnRegisterShutdownClient";

   if(conn != NULL)
   {
      const char* objName = "/org/genivi/NodeStateManager/LifeCycleConsumer";
      const char* busName = dbus_bus_get_unique_name(conn);

      DBusMessage* message = dbus_message_new_method_call("org.genivi.NodeStateManager",           // destination
                                                          "/org/genivi/NodeStateManager/Consumer", // path
                                                          "org.genivi.NodeStateManager.Consumer",  // interface
                                                          method);                                 // method
      if(message != NULL)
      {
         if(regType == 1)   // register
         {
            dbus_message_append_args(message, DBUS_TYPE_STRING, &busName,
                                              DBUS_TYPE_STRING, &objName,
                                              DBUS_TYPE_UINT32, &shutdownMode,
                                              DBUS_TYPE_UINT32, &gTimeoutMs, DBUS_TYPE_INVALID);
         }
         else           // unregister
         {
            dbus_message_append_args(message, DBUS_TYPE_STRING, &busName,
                                              DBUS_TYPE_STRING, &objName,
                                              DBUS_TYPE_UINT32, &shutdownMode, DBUS_TYPE_INVALID);
         }

		 if(!dbus_connection_send(conn, message, 0))
		 {
		    DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("send_lifecycle_register => Access denied"), DLT_STRING(error.message) );
		    rval = -1;
		 }
		 dbus_connection_flush(conn);
         dbus_message_unref(message);
      }
      else
      {
         DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("send_lifecycle_register => ERROR: Invalid message"));
      }
   }
   else
   {
      DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("send_lifecycle_register => ERROR: connection isn NULL"));
   }
}



void process_send_lifecycle_request(DBusConnection* conn, int requestId, int status)
{
   int rval = 0;
   DBusError error;
   dbus_error_init (&error);

   if(conn != NULL)
   {
      DBusMessage* message = dbus_message_new_method_call("org.genivi.NodeStateManager",           // destination
                                                         "/org/genivi/NodeStateManager/Consumer",  // path
                                                          "org.genivi.NodeStateManager.Consumer",  // interface
                                                          "LifecycleRequestComplete");             // method
      if(message != NULL)
      {
         dbus_message_append_args(message, DBUS_TYPE_INT32, &requestId,
                                           DBUS_TYPE_INT32, &status,
                                           DBUS_TYPE_INVALID);


		 if(!dbus_connection_send(conn, message, 0))
		 {
		    DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("send_lifecycle_request => Access denied"), DLT_STRING(error.message) );
		    rval = -1;
		 }

		 dbus_connection_flush(conn);
         dbus_message_unref(message);
      }
      else
      {
         DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("send_lifecycle_request => ERROR: Invalid message"));
         rval = -1;
      }
   }
   else
   {
      DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("send_lifecycle_request => ERROR: connection isn NULL"));
      rval = -1;
   }
}



void msg_pending_func(DBusPendingCall *call, void *data)
{
   int replyArg = -1;
   DBusError err;
   dbus_error_init(&err);

   DBusMessage *message = dbus_pending_call_steal_reply(call);

   if (dbus_set_error_from_message(&err, message))
   {
      DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("msg_pending_func ==> Access denied") );
   }
   else
   {
      //DLT_LOG(gDLTContext, DLT_LOG_INFO, DLT_STRING("msg_pending_func ==> UNlock mutex") );
      dbus_message_get_args(message, &err, DBUS_TYPE_INT32, &replyArg, DBUS_TYPE_INVALID);
   }

   gDbusPendingRvalue = replyArg;   // set the return value
   dbus_message_unref(message);

   // unlock the mutex because we have received the reply to the dbus message
   pthread_mutex_unlock(&gDbusPendingRegMtx);
}




