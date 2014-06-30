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

#include <errno.h>
#include <dlfcn.h>																/* For dlclose() */
#include "persistence_client_library_dbus_cmd.h"

#include "persistence_client_library_handle.h"
#include "persistence_client_library_custom_loader.h"
#include "persistence_client_library_prct_access.h"
#include "persistence_client_library_pas_interface.h"
#include "persistence_client_library_data_organization.h"
#include "persistence_client_library_db_access.h"


// function prototype
void msg_pending_func(DBusPendingCall *call, void *data);



void process_reg_notification_signal(DBusConnection* conn, unsigned int notifyLdbid, unsigned int notifyUserNo,
                                                           unsigned int notifySeatNo, unsigned int notifyPolicy, const char* notifyKey)
{
   char ruleChanged[DbusMatchRuleSize] = {[0 ... DbusMatchRuleSize-1] = 0};
   char ruleDeleted[DbusMatchRuleSize] = {[0 ... DbusMatchRuleSize-1] = 0};
   char ruleCreated[DbusMatchRuleSize] = {[0 ... DbusMatchRuleSize-1] = 0};

   // add match for  c h a n g e
   snprintf(ruleChanged, DbusMatchRuleSize,
            "type='signal',interface='org.genivi.persistence.adminconsumer',member='PersistenceResChange',path='/org/genivi/persistence/adminconsumer',arg0='%s',arg1='%u',arg2='%u',arg3='%u'",
            notifyKey, notifyLdbid, notifyUserNo, notifySeatNo);

   // add match for  d e l e t e
   snprintf(ruleDeleted, DbusMatchRuleSize,
            "type='signal',interface='org.genivi.persistence.adminconsumer',member='PersistenceResDelete',path='/org/genivi/persistence/adminconsumer',arg0='%s',arg1='%u',arg2='%u',arg3='%u'",
            notifyKey, notifyLdbid, notifyUserNo, notifySeatNo);

   // add match for  c r e a t e
   snprintf(ruleCreated, DbusMatchRuleSize,
            "type='signal',interface='org.genivi.persistence.adminconsumer',member='PersistenceResCreate',path='/org/genivi/persistence/adminconsumer',arg0='%s',arg1='%u',arg2='%u',arg3='%u'",
            notifyKey, notifyLdbid, notifyUserNo, notifySeatNo);

   if(notifyPolicy == Notify_register)
   {
      dbus_bus_add_match(conn, ruleChanged, NULL);
      dbus_bus_add_match(conn, ruleDeleted, NULL);
      dbus_bus_add_match(conn, ruleCreated, NULL);
      DLT_LOG(gPclDLTContext, DLT_LOG_VERBOSE, DLT_STRING("Registered for change notifications:"), DLT_STRING(ruleChanged));
   }
   else if(notifyPolicy == Notify_unregister)
   {
      dbus_bus_remove_match(conn, ruleChanged, NULL);
      dbus_bus_remove_match(conn, ruleDeleted, NULL);
      dbus_bus_remove_match(conn, ruleCreated, NULL);
      DLT_LOG(gPclDLTContext, DLT_LOG_VERBOSE, DLT_STRING("Unregistered for change notifications:"), DLT_STRING(ruleChanged));
   }

   dbus_connection_flush(conn);  // flush the connection to add the match
}



void process_send_notification_signal(DBusConnection* conn, unsigned int notifyLdbid, unsigned int notifyUserNo,
                                                            unsigned int notifySeatNo, unsigned int notifyReason, const char* notifyKey)
{
   dbus_bool_t ret;
   DBusMessage* message;
   const char* notifyReasonString = NULL;

   char ldbidArray[DbusSubMatchSize] = {[0 ... DbusSubMatchSize-1] = 0};
   char userArray[DbusSubMatchSize]  = {[0 ... DbusSubMatchSize-1] = 0};
   char seatArray[DbusSubMatchSize]  = {[0 ... DbusSubMatchSize-1] = 0};
   char* pldbidArra = ldbidArray;
   char* puserArray = userArray;
   char* pseatArray = seatArray;
   char* pnotifyKey = (char*)notifyKey;

   switch(notifyReason)
   {
      case pclNotifyStatus_deleted:
      	notifyReasonString = gDeleteSignal;
         break;
      case  pclNotifyStatus_created:
      	notifyReasonString = gCreateSignal;
         break;
      case pclNotifyStatus_changed:
      	notifyReasonString = gChangeSignal;
         break;
      default:
      	notifyReasonString = NULL;
         break;
   }

   if(notifyReasonString != NULL)
   {
      // dbus_bus_add_match is used for the notification mechanism,
      // and this works only for type DBUS_TYPE_STRING as message arguments
      // this is the reason to use string instead of integer types directly
      snprintf(ldbidArray, DbusSubMatchSize, "%u", notifyLdbid);
      snprintf(userArray,  DbusSubMatchSize, "%u", notifyUserNo);
      snprintf(seatArray,  DbusSubMatchSize, "%u", notifySeatNo);

      //printf("process_send_Notification_Signal => key: %s | lbid: %d | gUserNo: %d | gSeatNo: %d | gReason: %d \n", gNotifykey, gLdbid, gUserNo, gSeatNo, gReason);
      message = dbus_message_new_signal("/org/genivi/persistence/adminconsumer",    // const char *path,
                                        "org.genivi.persistence.adminconsumer",     // const char *interface,
                                        notifyReasonString);                                 // const char *name

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
               DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("process_send_Notification_Signal ==> failed to send dbus message!!"));
            }
         }
         else
         {
            DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("process_send_Notification_Signal ==> E R R O R  C O N E C T I O N  NULL!!"));
         }
      }
      else
      {
         DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("process_send_Notification_Signal ==> ERROR dbus_message_append_args"));
      }
   }
   else
   {
      DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("process_send_Notification_Signal ==> ERROR invalid notification reason"));
   }
}



void process_block_and_write_data_back(unsigned int requestID, unsigned int status)
{
   (void)requestID;
   (void)status;
   // lock persistence data access
   pers_lock_access();
   // sync data back to memory device
}



void process_prepare_shutdown(int complete)
{
   int i = 0, rval = 0;

   // block write
   pers_lock_access();

   // flush open files to disk
   for(i=0; i<MaxPersHandle; i++)
   {
      if(gOpenFdArray[i] == FileOpen)
      {

#if USE_FILECACHE
         if(complete == Shutdown_Full)
         {
         	rval = pfcCloseFile(i);
         }
         else if(complete == Shutdown_Partial)
         {
         	pfcWriteBackAndSync(i);
         }
#else
         if(complete == Shutdown_Full)
         {
         	rval = close(i);
         }
         else if(complete == Shutdown_Partial)
         {
         	fsync(i);
         }
#endif
         if(rval == -1)
         {
         	DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("process_prepare_shutdown => failed to close file: "), DLT_STRING(strerror(errno)) );
         }

      }
   }

   // close all opend rct
   pers_rct_close_all();

   // close opend database
   database_close_all();

   if(complete > 0)
   {
   	close_all_persistence_handle();
   }


   if(complete > 0)
   {
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
   }
}



void process_send_pas_request(DBusConnection* conn, unsigned int requestID, int status)
{
   DBusError error;
   dbus_error_init (&error);

   DBusMessage* message = dbus_message_new_method_call("org.genivi.persistence.admin",       // destination
                                                      "/org/genivi/persistence/admin",       // path
                                                       "org.genivi.persistence.admin",       // interface
                                                       "PersistenceAdminRequestCompleted");  // method
   if(conn != NULL)
   {
      if(message != NULL)
      {
         dbus_message_append_args(message, DBUS_TYPE_UINT32, &requestID,
                                           DBUS_TYPE_INT32,  &status,
                                           DBUS_TYPE_INVALID);

         if(!dbus_connection_send(conn, message, 0))
         {
            DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("send_lifecycle_register => Access denied"), DLT_STRING(error.message) );
         }

         dbus_connection_flush(conn);
         dbus_message_unref(message);
      }
      else
      {
         DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("send_pas_request => ERROR: Invalid message") );
      }
   }
   else
   {
      DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("send_pas_request => ERROR: Invalid connection") );
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
               DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("process_send_pas_register => dbus_pending_call_set_notify: FAILED\n") );
            }
            dbus_pending_call_unref(pending);
         }
         else
         {
            DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("send_pas_register =>  ERROR: Invalid message") );
         }
         dbus_message_unref(message);
      }
      else
      {
         DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("send_pas_register =>  ERROR: Invalid busname") );
      }
   }
   else
   {
      DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("send_pas_register =>  ERROR: Invalid connection") );
   }
}


void process_send_lifecycle_register(DBusConnection* conn, int regType, int shutdownMode)
{
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
		      DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("send_lifecycle_register => Access denied"), DLT_STRING(error.message) );
		   }
		   dbus_connection_flush(conn);
         dbus_message_unref(message);
      }
      else
      {
         DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("send_lifecycle_register => ERROR: Invalid message"));
      }
   }
   else
   {
      DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("send_lifecycle_register => ERROR: connection isn NULL"));
   }
}



void process_send_lifecycle_request(DBusConnection* conn, unsigned int requestId, unsigned int status)
{
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
            DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("send_lifecycle_request => Access denied"), DLT_STRING(error.message) );
          }

          dbus_connection_flush(conn);
          dbus_message_unref(message);
      }
      else
      {
         DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("send_lifecycle_request => ERROR: Invalid message"));
      }
   }
   else
   {
      DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("send_lifecycle_request => ERROR: connection isn NULL"));
   }
}



void msg_pending_func(DBusPendingCall *call, void *data)
{
   int replyArg = -1;
   DBusError err;
   dbus_error_init(&err);

   (void)data;

   DBusMessage *message = dbus_pending_call_steal_reply(call);

   if (dbus_set_error_from_message(&err, message))
   {
      DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("msg_pending_func ==> Access denied") );
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




