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
#include "../include_protected/persistence_client_library_data_organization.h"

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>


pthread_mutex_t gDbusInitializedMtx  = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  gDbusInitializedCond = PTHREAD_COND_INITIALIZER;

typedef enum EDBusObjectType
{
   OT_NONE = 0,
   OT_WATCH,
   OT_TIMEOUT
} tDBusObjectType;


typedef struct SObjectEntry
{
   tDBusObjectType objtype; /** libdbus' object */
   union
   {
      DBusWatch * watch;      /** watch "object" */
      DBusTimeout * timeout;  /** timeout "object" */
   };
} tObjectEntry;



/// polling structure
typedef struct SPollInfo
{
   int nfds;
   struct pollfd fds[10];
   tObjectEntry objects[10];
} tPollInfo;


/// polling information
static tPollInfo gPollInfo;

/// dbus connection
DBusConnection* gDbusConn = NULL;


DBusConnection* get_dbus_connection(void)
{
   return gDbusConn;
}

//------------------------------------------------------------------------
// debugging only until "correct" exit of main loop is possible!!!!!
//------------------------------------------------------------------------
#include "signal.h"
static int endLoop = 0;

void sigHandler(int signo)
{
   endLoop = 1;
}
//------------------------------------------------------------------------



#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))


/* function to unregister ojbect path message handler */
static void unregisterMessageHandler(DBusConnection *connection, void *user_data)
{
   printf("unregisterObjectPath\n");
}

/* catches messages not directed to any registered object path ("garbage collector") */
static DBusHandlerResult handleObjectPathMessageFallback(DBusConnection * connection, DBusMessage * message, void * user_data)
{
   DBusHandlerResult result = DBUS_HANDLER_RESULT_HANDLED;

   printf("handleObjectPathMessageFallback Object: '%s' -> Interface: '%s' -> Message: '%s'\n",
          dbus_message_get_sender(message), dbus_message_get_interface(message), dbus_message_get_member(message) );

   // org.genivi.persistence.admin  S I G N A L
   if((0==strcmp("org.genivi.persistence.admin", dbus_message_get_interface(message))))
   {
      printf("checkPersAdminSignalInterface '%s' -> '%s'\n", dbus_message_get_interface(message), dbus_message_get_member(message));
      if(dbus_message_get_type(message) == DBUS_MESSAGE_TYPE_SIGNAL)
      {
         printf("  checkPersAdminSignal signal\n");
         if((0==strcmp("PersistenceModeChanged", dbus_message_get_member(message))))
         {
            printf("  checkPersAdminSignal message\n");
            // to do handle signal
            result = signal_persModeChange(connection, message);
         }
         else
         {
            printf("handleObjectPathMessageFallback -> unknown signal '%s'\n", dbus_message_get_interface(message));
         }
      }
   }
   // org.genivi.persistence.admin  S I G N A L
   else if((0==strcmp("org.genivi.persistence.adminconsumer", dbus_message_get_interface(message))))
   {
      printf("checkPersAdminconsumerSignalInterface '%s' -> '%s'\n", dbus_message_get_interface(message), dbus_message_get_member(message));
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
            char* ldbid;
            char* user_no;
            char* seat_no;

            if (!dbus_message_get_args (message, &error, DBUS_TYPE_STRING, &notifyStruct.resource_id,
                                                         DBUS_TYPE_STRING, &ldbid,
                                                         DBUS_TYPE_STRING, &user_no,
                                                         DBUS_TYPE_STRING, &seat_no,
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

               result = DBUS_HANDLER_RESULT_NOT_YET_HANDLED;;
               dbus_message_unref(reply);
            }
            else
            {
               notifyStruct.ldbid       = atoi(ldbid);
               notifyStruct.user_no     = atoi(user_no);
               notifyStruct.seat_no     = atoi(seat_no);

               // call the registered callback function
               gChangeNotifyCallback(&notifyStruct);

               result = DBUS_HANDLER_RESULT_HANDLED;
            }
            dbus_connection_flush(connection);
         }
      }
   }
   // org.genivi.persistence.admin  P R O P E R T Y
   else  if((0==strcmp("org.freedesktop.DBus.Properties", dbus_message_get_interface(message))))
   {
      if(dbus_message_get_type(message) == DBUS_MESSAGE_TYPE_SIGNAL)
      {
         if((0==strcmp("EggDBusChanged", dbus_message_get_member(message))))
         {
            DBusMessageIter array;
            DBusMessageIter dict;
            DBusMessageIter variant;

            char* dictString = NULL;
            int value = 0;

            dbus_message_iter_open_container(&array, DBUS_TYPE_DICT_ENTRY, 0, &dict);
            dbus_message_iter_get_basic(&dict, &dictString);

            dbus_message_iter_open_container(&dict,DBUS_TYPE_VARIANT, NULL, &variant);
            dbus_message_iter_get_basic(&dict, &value);

            dbus_message_iter_close_container(&dict, &variant);
            dbus_message_iter_close_container(&array, &dict);

            printf("handleObjectPathMessageFallback ==> value: %d \n", value);
            // to do handle signal
            result = DBUS_HANDLER_RESULT_HANDLED;
         }
         else
         {
            printf("handleObjectPathMessageFallback -> unknown property '%s'\n", dbus_message_get_interface(message));
         }
      }
      else
      {
         printf("handleObjectPathMessageFallback -> not a signal '%s'\n", dbus_message_get_member(message));
      }
   }

   return result;
}



static void  unregisterObjectPathFallback(DBusConnection *connection, void *user_data)
{
   printf("unregisterObjectPathFallback\n");
}



void* run_mainloop(void* dataPtr)
{
   // persistence admin message
   static const struct DBusObjectPathVTable vtablePersAdmin
      = {unregisterMessageHandler, checkPersAdminMsg, NULL, };

   // lifecycle message
   static const struct DBusObjectPathVTable vtableLifecycle
      = {unregisterMessageHandler, checkLifecycleMsg, NULL, };

   // fallback
   static const struct DBusObjectPathVTable vtableFallback
      = {unregisterObjectPathFallback, handleObjectPathMessageFallback, NULL, };

   // setup the dbus
   mainLoop(vtablePersAdmin, vtableLifecycle, vtableFallback, dataPtr);

   printf("Exit dbus main loop!!!!\n");

   return NULL;
}



int setup_dbus_mainloop(void)
{
   int rval = 0;
   pthread_t thread;
   DBusError err;
   const char *pAddress = getenv("PERS_CLIENT_DBUS_ADDRESS");

   // enable locking of data structures in the D-Bus library for multi threading.
   dbus_threads_init_default();

   dbus_error_init(&err);

   // Connect to the bus and check for errors
   if(pAddress != NULL)
   {
      printf("Use specific dbus address: %s\n !", pAddress);
      gDbusConn = dbus_connection_open(pAddress, &err);

      if(gDbusConn != NULL)
      {
         if(!dbus_bus_register(gDbusConn, &err))
         {
            printf("dbus_bus_register() Error %s\n", err.message);
            dbus_error_free (&err);
            return -1;
         }
         else
         {
            printf("Registered connection successfully !\n");
         }
      }
      else
      {
         printf("dbus_connection_open() Error %s\n",err.message);
         dbus_error_free(&err);
      }
   }
   else
   {
      printf("Use default dbus bus (DBUS_BUS_SYSTEM) !!!!!!\n");
      gDbusConn = dbus_bus_get(DBUS_BUS_SYSTEM, &err);
   }

   // wain until dbus main loop has been setup and running
   pthread_mutex_lock(&gDbusInitializedMtx);

   // create here the dbus connection and pass to main loop
   rval = pthread_create(&thread, NULL, run_mainloop, gDbusConn);
   if(rval)
   {
     fprintf(stderr, "Server: - ERROR! pthread_create( run_mainloop ) returned: %d\n", rval);
   }

   // wait for condition variable
   pthread_cond_wait(&gDbusInitializedCond, &gDbusInitializedMtx);

   pthread_mutex_unlock(&gDbusInitializedMtx);
   return rval;
}





static dbus_bool_t addWatch(DBusWatch *watch, void *data)
{
   dbus_bool_t result = FALSE;

   //fprintf(stderr, "addWatch called @%08x flags: %08x enabled: %c\n", (unsigned int)watch, dbus_watch_get_flags(watch), TRUE==dbus_watch_get_enabled(watch)?'x':'-');

   if (ARRAY_SIZE(gPollInfo.fds)>gPollInfo.nfds)
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
   printf("removeWatch called @0x%08x\n", (int)watch);
}



static void watchToggled(DBusWatch *watch, void *data)
{
   printf("watchToggled called @0x%08x\n", (int)watch);
}



static dbus_bool_t addTimeout(DBusTimeout *timeout, void *data)
{
   dbus_bool_t ret = FALSE;

   if (ARRAY_SIZE(gPollInfo.fds)>gPollInfo.nfds)
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
               fprintf(stderr, "timerfd_settime() failed %d '%s'\n", errno, strerror(errno));
            }
         }
         else
         {
            fprintf(stderr, "timerfd_create() failed %d '%s'\n", errno, strerror(errno));
         }
      }
   }
   else
   {
      fprintf(stderr, "cannot create another fd to be poll()'ed\n");
   }

   return ret;
}



static void removeTimeout(DBusTimeout *timeout, void *data)
{

   int i = gPollInfo.nfds;
   while ((0<i--)&&(timeout!=gPollInfo.objects[i].timeout));

   if (0<i)
   {
      if (-1==close(gPollInfo.fds[i].fd))
      {
         fprintf(stderr, "close() timerfd #%d failed %d '%s'\n", gPollInfo.fds[i].fd, errno, strerror(errno));
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



/** callback for libdbus' when timeout changed */
static void timeoutToggled(DBusTimeout *timeout, void *data)
{
   int i = gPollInfo.nfds;
   while ((0<i--)&&(timeout!=gPollInfo.objects[i].timeout));
   fprintf(stderr, "timeoutToggled @%x %c %dms @%d [%d]\n", (int)timeout, dbus_timeout_get_enabled(timeout)?'x':'-', dbus_timeout_get_interval(timeout), i, gPollInfo.nfds);
   if (0<i)
   {
      const int interval = (TRUE==dbus_timeout_get_enabled(timeout))?dbus_timeout_get_interval(timeout):0;
      const struct itimerspec its = { .it_value= {interval/1000, interval%1000} };
      if (-1!=timerfd_settime(gPollInfo.fds[i].fd, 0, &its, NULL))
      {
         fprintf(stderr, "timerfd_settime() %d failed %d '%s'\n", interval, errno, strerror(errno));
      }
   }
}



int mainLoop(DBusObjectPathVTable vtable, DBusObjectPathVTable vtable2,
             DBusObjectPathVTable vtableFallback, void* userData)
{
   DBusError err;
   // lock mutex to make sure dbus main loop is running
   pthread_mutex_lock(&gDbusInitializedMtx);

   signal(SIGTERM, sigHandler);
   signal(SIGQUIT, sigHandler);
   signal(SIGINT,  sigHandler);

   DBusConnection* conn = (DBusConnection*)userData;
   dbus_error_init(&err);

   if (dbus_error_is_set(&err))
   {
      printf("Connection Error (%s)\n", err.message);
      dbus_error_free(&err);
   }
   else if (NULL != conn)
   {
      dbus_connection_set_exit_on_disconnect (conn, FALSE);
      //printf("connected as '%s'\n", dbus_bus_get_unique_name(conn));
      if (-1 == (gEfds = eventfd(0, 0)))
      {
         printf("eventfd() failed w/ errno %d\n", errno);
      }
      else
      {
         int ret;
         int bContinue = 0;
         memset(&gPollInfo, 0 , sizeof(gPollInfo));

         gPollInfo.nfds = 1;
         gPollInfo.fds[0].fd = gEfds;
         gPollInfo.fds[0].events = POLLIN;

         dbus_bus_add_match(conn, "type='signal',interface='org.genivi.persistence.admin',member='PersistenceModeChanged',path='/org/genivi/persistence/admin'", &err);

         // register for messages
         if (   (TRUE==dbus_connection_register_object_path(conn, "/org/genivi/persistence/adminconsumer", &vtable, userData))
             && (TRUE==dbus_connection_register_object_path(conn, "/org/genivi/NodeStateManager/LifeCycleConsumer", &vtable2, userData))
             && (TRUE==dbus_connection_register_fallback(conn, "/", &vtableFallback, userData)) )
         {
            if(   (TRUE!=dbus_connection_set_watch_functions(conn, addWatch, removeWatch, watchToggled, NULL, NULL))
               || (TRUE!=dbus_connection_set_timeout_functions(conn, addTimeout, removeTimeout, timeoutToggled, NULL, NULL)) )
            {
               printf("dbus_connection_set_watch_functions() failed\n");
            }
            else
            {
               pthread_cond_signal(&gDbusInitializedCond);
               pthread_mutex_unlock(&gDbusInitializedMtx);
               do
               {
                  bContinue = 0; /* assume error */

                  while (DBUS_DISPATCH_DATA_REMAINS==dbus_connection_dispatch(conn));

                  while ((-1==(ret=poll(gPollInfo.fds, gPollInfo.nfds, -1)))&&(EINTR==errno));

                  if (0>ret)
                  {
                     fprintf(stderr, "poll() failed w/ errno %d\n", errno);
                  }
                  else if (0==ret)
                  {
                     /* poll time-out */
                  }
                  else
                  {
                     int i;

                     for (i=0; gPollInfo.nfds>i; ++i)
                     {
                        /* anything to do */
                        if (0!=gPollInfo.fds[i].revents)
                        {
                           //fprintf(stderr, "\t[%d] revents 0x%04x\n", i, gPollInfo.fds[i].revents);

                           if (OT_TIMEOUT==gPollInfo.objects[i].objtype)
                           {
                              /* time-out occured */
                              unsigned long long nExpCount = 0;
                              if ((ssize_t)sizeof(nExpCount)!=read(gPollInfo.fds[i].fd, &nExpCount, sizeof(nExpCount)))
                              {
                                 fprintf(stderr, "read failed!?\n");
                              }
                              fprintf(stderr, "timeout %x #%d!\n", (int)gPollInfo.objects[i].timeout, (int)nExpCount);
                              if (FALSE==dbus_timeout_handle(gPollInfo.objects[i].timeout))
                              {
                                 fprintf(stderr, "dbus_timeout_handle() failed!?\n");
                              }
                              bContinue = TRUE;
                           }
                           else if (gPollInfo.fds[i].fd==gEfds)
                           {
                              /* internal command */
                              if (0!=(gPollInfo.fds[i].revents & POLLIN))
                              {
                                 uint16_t buf[64];
                                 bContinue = TRUE;
                                 while ((-1==(ret=read(gPollInfo.fds[i].fd, buf, 64)))&&(EINTR==errno));
                                 if (0>ret)
                                 {
                                    printf("read() failed w/ errno %d | %s\n", errno, strerror(errno));
                                 }
                                 else if (ret != -1)
                                 {
                                    switch (buf[0])
                                    {
                                       case CMD_PAS_BLOCK_AND_WRITE_BACK:
                                          process_block_and_write_data_back((buf[2]), buf[1]);
                                          break;
                                       case CMD_LC_PREPARE_SHUTDOWN:
                                          process_prepare_shutdown((buf[2]), buf[1]);
                                          break;
                                       case CMD_QUIT:
                                          bContinue = FALSE;
                                          break;
                                       default:
                                          printf("command %d not handled!\n", buf[0]);
                                          break;
                                    }
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
                  if(endLoop == 1)
                     break;
               }
               while (0!=bContinue);
            }
            dbus_connection_unregister_object_path(conn, "/org/genivi/persistence/adminconsumer");
            dbus_connection_unregister_object_path(conn, "/org/genivi/NodeStateManager/LifeCycleConsumer");
            dbus_connection_unregister_object_path(conn, "/");
         }
         close(gEfds);
      }
      dbus_connection_unref(conn);
      dbus_shutdown();
   }

   pthread_cond_signal(&gDbusInitializedCond);
   pthread_mutex_unlock(&gDbusInitializedMtx);
   return 0;
}

