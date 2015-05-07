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
 * @file           persistence_client_library_dbus_service.c
 * @ingroup        Persistence client library
 * @author         Ingo Huerner
 * @brief          Implementation of the persistence client library dbus service.
 * @see
 */


#include "persistence_client_library_dbus_service.h"
#include "persistence_client_library_lc_interface.h"
#include "persistence_client_library_pas_interface.h"
#include "persistence_client_library_dbus_cmd.h"

#include <errno.h>
#include <stdlib.h>

pthread_mutex_t gDbusPendingRegMtx   = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t gDeliverpMtx         = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t gMainCondMtx         = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  gMainLoopCond        = PTHREAD_COND_INITIALIZER;

pthread_t gMainLoopThread;

int gMainLoopCondValue = 0;

const char* gDbusLcConsDest    = "org.genivi.NodeStateManager";

const char* gDbusLcConsterface = "org.genivi.NodeStateManager.LifeCycleConsumer";
const char* gDbusLcConsPath    = "/org/genivi/NodeStateManager/LifeCycleConsumer";
const char* gDbusLcInterface   = "org.genivi.NodeStateManager.Consumer";
const char* gDbusLcCons        = "/org/genivi/NodeStateManager/Consumer";
const char* gDbusLcConsMsg     = "LifecycleRequest";

const char* gDbusPersAdminConsInterface = "org.genivi.persistence.adminconsumer";
const char* gPersAdminConsumerPath      = "/org/genivi/persistence/adminconsumer";
const char* gDbusPersAdminPath          = "/org/genivi/persistence/admin";
const char* gDbusPersAdminInterface     = "org.genivi.persistence.admin";
const char* gDbusPersAdminConsMsg       = "PersistenceAdminRequest";

/// communication channel into the dbus mainloop
static int gPipeFd[2] = {-1};


typedef enum EDBusObjectType
{
   OT_NONE = 0,
   OT_WATCH,
   OT_TIMEOUT
} tDBusObjectType;



/// object entry
typedef struct SObjectEntry
{
   tDBusObjectType objtype;	/// libdbus' object
   union
   {
      DBusWatch * watch;		/// watch "object"
      DBusTimeout * timeout;	/// timeout "object"
   };
} tObjectEntry;



/// polling structure
typedef struct SPollInfo
{
   int nfds;						/// number of polls
   struct pollfd fds[10];		/// poll file descriptors array
   tObjectEntry objects[10];	/// poll object
} tPollInfo;


/// polling information
static tPollInfo gPollInfo;

#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))


/* function to unregister ojbect path message handler */
static void unregisterMessageHandler(DBusConnection *connection, void *user_data)
{
   (void)connection;
   (void)user_data;
   DLT_LOG(gPclDLTContext, DLT_LOG_DEBUG, DLT_STRING("unregisterObjectPath\n"));
}


/* catches messages not directed to any registered object path ("garbage collector") */
static DBusHandlerResult handleObjectPathMessageFallback(DBusConnection * connection, DBusMessage * message, void * user_data)
{
   DBusHandlerResult result = DBUS_HANDLER_RESULT_HANDLED;
   (void)user_data;

   if((0==strcmp(gDbusPersAdminConsInterface, dbus_message_get_interface(message))))
   {
      if(dbus_message_get_type(message) == DBUS_MESSAGE_TYPE_SIGNAL)
      {
         pclNotification_s notifyStruct;
         int validMessage = 0;

         if((0==strcmp("PersistenceResChange", dbus_message_get_member(message))))
         {
            notifyStruct.pclKeyNotify_Status = pclNotifyStatus_changed;
            validMessage = 1;
         }
         else if((0==strcmp("PersistenceResDelete", dbus_message_get_member(message))))
         {
            notifyStruct.pclKeyNotify_Status = pclNotifyStatus_deleted;
            validMessage = 1;
         }
         else if((0==strcmp("PersistenceRes", dbus_message_get_member(message))))
         {
            notifyStruct.pclKeyNotify_Status = pclNotifyStatus_created;
            validMessage = 1;
         }

         if(validMessage == 1)
         {
            DBusError error;
            DBusMessage *reply;
            dbus_error_init (&error);
            char *ldbid, *user_no, *seat_no;

            if (!dbus_message_get_args(message, &error, DBUS_TYPE_STRING, &notifyStruct.resource_id,
                                                         DBUS_TYPE_STRING, &ldbid,
                                                         DBUS_TYPE_STRING, &user_no,
                                                         DBUS_TYPE_STRING, &seat_no,
                                                         DBUS_TYPE_INVALID))
            {
               reply = dbus_message_new_error(message, error.name, error.message);

               if (reply == 0)
               {
                  DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("handleObjPathMsgFback - DBus No mem"), DLT_STRING(dbus_message_get_interface(message)) );
               }

               if (!dbus_connection_send(connection, reply, 0))
               {
                  DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("handleObjPathMsgFback - DBus No mem"), DLT_STRING(dbus_message_get_interface(message)) );
               }

               result = DBUS_HANDLER_RESULT_NOT_YET_HANDLED;;
               dbus_message_unref(reply);
            }
            else
            {
               notifyStruct.ldbid       = atoi(ldbid);
               notifyStruct.user_no     = atoi(user_no);
               notifyStruct.seat_no     = atoi(seat_no);

               if(gChangeNotifyCallback != NULL )  // call the registered callback function
               {
                  gChangeNotifyCallback(&notifyStruct);
               }
               else
               {
                  DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("handleObjPathMsgFback - gChangeNotifyCallback not set (NULL?)") );
               }
               result = DBUS_HANDLER_RESULT_HANDLED;
            }
            dbus_connection_flush(connection);
         }
      }
   }
   return result;
}



static void  unregisterObjectPathFallback(DBusConnection *connection, void *user_data)
{
   (void)connection;
   (void)user_data;
   DLT_LOG(gPclDLTContext, DLT_LOG_DEBUG, DLT_STRING("unregObjPathFback"));
}



static dbus_bool_t addWatch(DBusWatch *watch, void *data)
{
   dbus_bool_t result = FALSE;
   (void)data;

   if (ARRAY_SIZE(gPollInfo.fds) > (unsigned int)(gPollInfo.nfds))
   {
      int flags = dbus_watch_get_flags(watch);

      tObjectEntry * const pEntry = &gPollInfo.objects[gPollInfo.nfds];
      pEntry->objtype = OT_WATCH;
      pEntry->watch = watch;

      gPollInfo.fds[gPollInfo.nfds].fd = dbus_watch_get_unix_fd(watch);

      if (TRUE==dbus_watch_get_enabled(watch))
      {
         if (flags&DBUS_WATCH_READABLE)
         {
            gPollInfo.fds[gPollInfo.nfds].events |= POLLIN;
         }
         if (flags&DBUS_WATCH_WRITABLE)
         {
            gPollInfo.fds[gPollInfo.nfds].events |= POLLOUT;
         }

         ++gPollInfo.nfds;
      }
      result = TRUE;
   }

   return result;
}



static void removeWatch(DBusWatch *watch, void *data)
{
   void* w_data = dbus_watch_get_data(watch);

   (void)data;

   DLT_LOG(gPclDLTContext, DLT_LOG_INFO, DLT_STRING("removeWatch called "), DLT_INT( (long)watch) );

   if(w_data)
      free(w_data);

   dbus_watch_set_data(watch, NULL, NULL);
}



static void watchToggled(DBusWatch *watch, void *data)
{
   (void)data;
   DLT_LOG(gPclDLTContext, DLT_LOG_INFO, DLT_STRING("watchToggled called "), DLT_INT( (long)watch) );

   if(dbus_watch_get_enabled(watch))
      addWatch(watch, data);
   else
      removeWatch(watch, data);
}



static dbus_bool_t addTimeout(DBusTimeout *timeout, void *data)
{
   (void)data;
   dbus_bool_t ret = FALSE;

   if(ARRAY_SIZE(gPollInfo.fds) > (unsigned int)(gPollInfo.nfds))
   {
      const int interval = dbus_timeout_get_interval(timeout);
      if ((0<interval)&&(TRUE==dbus_timeout_get_enabled(timeout)))
      {
         const int tfd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC);
         if (-1!=tfd)
         {
            const struct itimerspec its = { .it_value= {interval/1000, interval%1000} };
            if (-1!=timerfd_settime(tfd, 0, &its, NULL))
            {
               tObjectEntry * const pEntry = &gPollInfo.objects[gPollInfo.nfds];
               pEntry->objtype = OT_TIMEOUT;
               pEntry->timeout = timeout;
               gPollInfo.fds[gPollInfo.nfds].fd = tfd;
               gPollInfo.fds[gPollInfo.nfds].events |= POLLIN;
               ++gPollInfo.nfds;
               ret = TRUE;
            }
            else
            {
               DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("addTimeout - _settime() failed"), DLT_STRING(strerror(errno)) );
            }
         }
         else
         {
            DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("addTimeout - _create() failed"), DLT_STRING(strerror(errno)) );
         }
      }
   }
   else
   {
      DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("addTimeout - cannot create another fd to be poll()'ed"));
   }
   return ret;
}



static void removeTimeout(DBusTimeout *timeout, void *data)
{
   int i = gPollInfo.nfds;
   (void)data;

   while ((0<i--)&&(timeout!=gPollInfo.objects[i].timeout));

   if (0<i)
   {
      if (-1==close(gPollInfo.fds[i].fd))
      {
         DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("removeTimeout - close() timerfd"), DLT_STRING(strerror(errno)) );
      }

      --gPollInfo.nfds;
      while (gPollInfo.nfds>i)
      {
         gPollInfo.fds[i] = gPollInfo.fds[i+1];
         gPollInfo.objects[i] = gPollInfo.objects[i+1];
         ++i;
      }

      gPollInfo.fds[gPollInfo.nfds].fd = -1;
      gPollInfo.objects[gPollInfo.nfds].objtype = OT_NONE;
   }
}



// callback for libdbus' when timeout changed
static void timeoutToggled(DBusTimeout *timeout, void *data)
{
   int i = gPollInfo.nfds;
   (void)data;

   while ((0<i--)&&(timeout!=gPollInfo.objects[i].timeout));
   DLT_LOG(gPclDLTContext, DLT_LOG_DEBUG, DLT_STRING("timeoutToggled") );
   if (0<i)
   {
      const int interval = (TRUE==dbus_timeout_get_enabled(timeout))?dbus_timeout_get_interval(timeout):0;
      const struct itimerspec its = { .it_value= {interval/1000, interval%1000} };
      if (-1!=timerfd_settime(gPollInfo.fds[i].fd, 0, &its, NULL))
      {
         DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("timeoutToggled - timerfd_settime()"), DLT_STRING(strerror(errno)) );
      }
   }
}



int setup_dbus_mainloop(void)
{
   int rval = 0, doCleanup = 0;
   DBusError err;
   DBusConnection* conn = NULL;
   const char *pAddress = getenv("PERS_CLIENT_DBUS_ADDRESS");

   dbus_error_init(&err);

   if(!dbus_threads_init_default())
   {
      DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("setupMainLoop - initDefaultFailed() :"), DLT_STRING(err.message) );
      dbus_error_free (&err);
      return EPERS_COMMON;
   }

   if(pAddress != NULL)    // Connect to the bus and check for errors
   {
      DLT_LOG(gPclDLTContext, DLT_LOG_DEBUG, DLT_STRING("setupMainLoop - specific dbus address:"), DLT_STRING(pAddress) );

      conn = dbus_connection_open_private(pAddress, &err);

      if(conn != NULL)
      {
         if(!dbus_bus_register(conn, &err))
         {
            DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("setupMainLoop - _register() :"), DLT_STRING(err.message) );
            dbus_error_free (&err);
            return EPERS_COMMON;
         }
      }
      else
      {
         DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("setupMainLoop - open_private() :"), DLT_STRING(err.message) );
         dbus_error_free(&err);
         return EPERS_COMMON;
      }
   }
   else
   {
      DLT_LOG(gPclDLTContext, DLT_LOG_DEBUG, DLT_STRING("setupMainLoop - Use def bus (DBUS_BUS_SYSTEM)"));
      conn = dbus_bus_get_private(DBUS_BUS_SYSTEM, &err);

      if(conn == NULL)
      {
         DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("setupMainLoop - get_private() :"), DLT_STRING(err.message) );
         dbus_error_free(&err);
         return EPERS_COMMON;
      }
   }

   if (-1 == (pipe(gPipeFd)))    // create communication pipe with the dbus mainloop
   {
      DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("mainLoop - eventfd() failed w/ errno:"), DLT_INT(errno) );
      rval = EPERS_COMMON;
   }
   else
   {
      // persistence administrator message
      const struct DBusObjectPathVTable vtablePersAdmin = {unregisterMessageHandler, checkPersAdminMsg, NULL, NULL, NULL, NULL};
      // lifecycle message
      const struct DBusObjectPathVTable vtableLifecycle = {unregisterMessageHandler, checkLifecycleMsg, NULL, NULL, NULL, NULL};
      // fallback
      const struct DBusObjectPathVTable vtableFallback  = {unregisterObjectPathFallback, handleObjectPathMessageFallback, NULL, NULL, NULL, NULL};

#if USE_PASINTERFACE != 1
      (void)vtablePersAdmin;
#endif

      memset(&gPollInfo, 0 , sizeof(gPollInfo));
      gPollInfo.nfds = 1;
      gPollInfo.fds[0].fd = gPipeFd[0];
      gPollInfo.fds[0].events = POLLIN;

      dbus_bus_add_match(conn, "type='signal',interface='org.genivi.persistence.admin',member='PersistenceModeChanged',path='/org/genivi/persistence/admin'", &err);

      // register for messages
      if (   (TRUE==dbus_connection_register_object_path(conn, gDbusLcConsPath, &vtableLifecycle, conn))
   #if USE_PASINTERFACE == 1
          && (TRUE==dbus_connection_register_object_path(conn, gPersAdminConsumerPath, &vtablePersAdmin, conn))
   #endif
          && (TRUE==dbus_connection_register_fallback(conn, "/", &vtableFallback, conn)) )
      {
         if(   (TRUE!=dbus_connection_set_watch_functions(conn, addWatch, removeWatch, watchToggled, NULL, NULL))
            || (TRUE!=dbus_connection_set_timeout_functions(conn, addTimeout, removeTimeout, timeoutToggled, NULL, NULL)) )
         {
            DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("mainLoop - set_watch_functions() failed"));
            doCleanup = 1;
            rval = EPERS_COMMON;
         }
         else
         {
            dbus_connection_set_exit_on_disconnect(conn, FALSE);

            if(pthread_create(&gMainLoopThread, NULL, mainLoop, conn) != -1)
            {
               (void)pthread_setname_np(gMainLoopThread, "pclDbusLoop");
            }
            else
            {
               DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("pthread_create( DBUS run_mainloop ) ret err:"), DLT_INT(rval) );
               doCleanup = 1;
               rval = EPERS_COMMON;
            }
         }
      }
      else
      {
         doCleanup = 1;
      }
   }

   if(doCleanup)     // close pipe and close dbus connection if anything goes wrong setting up
   {
      if(gPipeFd[0] != -1)
      {
         close(gPipeFd[0]);
         close(gPipeFd[1]);
      }

#if USE_PASINTERFACE == 1
      dbus_connection_unregister_object_path(conn, gPersAdminConsumerPath);
#endif
      dbus_connection_unregister_object_path(conn, gDbusLcConsPath);
      dbus_connection_unregister_object_path(conn, "/");

      dbus_connection_close(conn);
      dbus_connection_unref(conn);
      dbus_shutdown();

      rval = EPERS_COMMON;
   }

   return rval;
}



int dispatchInternalCommand(DBusConnection* conn, MainLoopData_u* readData, int* quit)
{
   int rval = 1;

   //DLT_LOG(gPclDLTContext, DLT_LOG_DEBUG, DLT_STRING("mainLoop - receive cmd:"), DLT_INT(readData.message.cmd));
   switch (readData->message.cmd)
   {
      case CMD_PAS_BLOCK_AND_WRITE_BACK:
         process_block_and_write_data_back(readData->message.params[1] /*requestID*/, readData->message.params[0] /*status*/);
         process_send_pas_request(conn,    readData->message.params[1] /*request*/,   readData->message.params[0] /*status*/);
         break;
      case CMD_LC_PREPARE_SHUTDOWN:
         process_prepare_shutdown(Shutdown_Full);
         process_send_lifecycle_request(conn, readData->message.params[1] /*requestID*/, readData->message.params[0] /*status*/);
         break;
      case CMD_SEND_NOTIFY_SIGNAL:
         process_send_notification_signal(conn, readData->message.params[0] /*ldbid*/, readData->message.params[1], /*user*/
                                                readData->message.params[2] /*seat*/,  readData->message.params[3], /*reason*/
                                                readData->message.string);
         break;
      case CMD_REG_NOTIFY_SIGNAL:
         process_reg_notification_signal(conn, readData->message.params[0] /*ldbid*/, readData->message.params[1], /*user*/
                                               readData->message.params[2] /*seat*/,  readData->message.params[3], /*,policy*/
                                               readData->message.string);
         break;
      case CMD_SEND_PAS_REGISTER:
         process_send_pas_register(conn, readData->message.params[0] /*regType*/, readData->message.params[1] /*notifyFlag*/);
         break;
      case CMD_SEND_LC_REGISTER:
         process_send_lifecycle_register(conn, readData->message.params[0] /*regType*/, readData->message.params[1] /*mode*/);
         break;
      case CMD_QUIT:
         rval = 0;
         *quit = TRUE;
         break;
      default:
         DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("mainLoop - cmd not handled"), DLT_INT(readData->message.cmd) );
         break;
   }

   return rval;
}



void* mainLoop(void* userData)
{
   int ret, bContinue = 0;   /// indicator if dbus mainloop shall continue

   DBusConnection* conn = (DBusConnection*)userData;

   do
   {
      while(DBUS_DISPATCH_DATA_REMAINS==dbus_connection_dispatch(conn));

      while ((-1==(ret=poll(gPollInfo.fds, gPollInfo.nfds, -1)))&&(EINTR==errno));

      if (0>ret)
      {
         DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("mainLoop - poll() failed w/ errno "), DLT_INT(errno) );
      }
      else if (0==ret)
      {
         /* poll time-out */
      }
      else
      {
         int i, bQuit = FALSE;

         for (i=0; gPollInfo.nfds>i && !bQuit; ++i)
         {
            if (0!=gPollInfo.fds[i].revents)    // anything to do
            {
               if (OT_TIMEOUT==gPollInfo.objects[i].objtype)
               {
                  unsigned long long nExpCount = 0;   // time-out occured

                  if ((ssize_t)sizeof(nExpCount)!=read(gPollInfo.fds[i].fd, &nExpCount, sizeof(nExpCount)))
                  {
                     DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("mainLoop - read failed"));
                  }
                  DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("mainLoop - timeout"));

                  if (FALSE==dbus_timeout_handle(gPollInfo.objects[i].timeout))
                  {
                     DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("mainLoop - _timeout_handle() failed!?"));
                  }
                  bContinue = TRUE;
               }
               else if (gPollInfo.fds[i].fd == gPipeFd[0])
               {
                  if (0!=(gPollInfo.fds[i].revents & POLLIN))  // dispatch internal command
                  {
                     MainLoopData_u readData;
                     bContinue = TRUE;
                     while ((-1==(ret = read(gPollInfo.fds[i].fd, readData.payload, sizeof(struct message_))))&&(EINTR == errno));
                     if(ret < 0)
                     {
                        DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("mainLoop - read() failed"), DLT_STRING(strerror(errno)) );
                     }
                     else
                     {
                        pthread_mutex_lock(&gMainCondMtx);

                        bContinue = dispatchInternalCommand(conn, &readData, &bQuit);

                        gMainLoopCondValue = 1;
                        pthread_cond_signal(&gMainLoopCond);
                        pthread_mutex_unlock(&gMainCondMtx);
                     }
                  }
               }
               else
               {
                  int flags = 0;

                  if (0!=(gPollInfo.fds[i].revents & POLLIN))
                  {
                     flags |= DBUS_WATCH_READABLE;
                  }
                  if (0!=(gPollInfo.fds[i].revents & POLLOUT))
                  {
                     flags |= DBUS_WATCH_WRITABLE;
                  }
                  if (0!=(gPollInfo.fds[i].revents & POLLERR))
                  {
                     flags |= DBUS_WATCH_ERROR;
                  }
                  if (0!=(gPollInfo.fds[i].revents & POLLHUP))
                  {
                     flags |= DBUS_WATCH_HANGUP;
                  }
                  bContinue = dbus_watch_handle(gPollInfo.objects[i].watch, flags);
               }
            }
         }
      }
   }
   while (0 != bContinue);

   // do some cleanup
   close(gPipeFd[0]);
   close(gPipeFd[1]);

#if USE_PASINTERFACE == 1
   dbus_connection_unregister_object_path(conn, gPersAdminConsumerPath);
#endif
   dbus_connection_unregister_object_path(conn, gDbusLcConsPath);
   dbus_connection_unregister_object_path(conn, "/");

   dbus_connection_close(conn);
   dbus_connection_unref(conn);
   dbus_shutdown();

   return NULL;
}



int deliverToMainloop(MainLoopData_u* payload)
{
   int rval = 0;

   pthread_mutex_lock(&gDeliverpMtx);     // make sure  deliverToMainloop will be used exclusively
   rval = deliverToMainloop_NM(payload);


   pthread_mutex_lock(&gMainCondMtx);     // mutex needed for pthread condition used to wait on other thread (mainloop)
   while(0 == gMainLoopCondValue)
      pthread_cond_wait(&gMainLoopCond, &gMainCondMtx);
   pthread_mutex_unlock(&gMainCondMtx);


   gMainLoopCondValue = 0;
   pthread_mutex_unlock(&gDeliverpMtx);

   return rval;
}



int deliverToMainloop_NM(MainLoopData_u* payload)
{
   int rval = 0;

   if(-1 == write(gPipeFd[1], payload->payload, sizeof(struct message_)))
   {
     DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("toMainloop => failed write pipe"), DLT_INT(errno));
     rval = -1;
   }
   return rval;
}

