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
 * @file           persistence_admin_service_mockup.c
 * @ingroup        Persistence client library test
 * @author         Ingo Huerner
 * @brief          Persistence Administration Serivce mockup
 * @see            
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>     /* exit */
#include <check.h>
#include <time.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <dbus/dbus.h>
#include <poll.h>
#include <pthread.h>
#include <sys/eventfd.h>


/// command definitions for main loop
typedef enum ECmd
{
   CMD_NONE = 0,                    /// command none
   CMD_PAS_BLOCK_AND_WRITE_BACK,    /// command block access and write data back
   CMD_LC_PREPARE_SHUTDOWN,         /// command to prepare shutdown
   CMD_QUIT,                         /// quit command
   CMD_REQUEST_NAME
} tCmd;


/// pipe file descriptors
int gEfds;



pthread_mutex_t gDbusInitializedMtx  = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  gDbusInitializedCond = PTHREAD_COND_INITIALIZER;

/// polling structure
typedef struct SPollInfo
{
   int nfds;
   struct pollfd fds[10];
   DBusWatch * watches[10];
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



int checkAdminMsg(DBusConnection *connection, DBusMessage *message)
{
   char* busName   = NULL;
   char* objName = NULL;
   int32_t  notificationFlag = 0;
   uint32_t gTimeoutMs = 0;
   int msgReturn = 123321;

   DBusMessage *reply;
   DBusError error;
   dbus_error_init (&error);

   if (!dbus_message_get_args (message, &error, DBUS_TYPE_STRING, &busName,  // bus name
                                                DBUS_TYPE_STRING, &objName,
                                                DBUS_TYPE_INT32,  &notificationFlag,
                                                DBUS_TYPE_UINT32, &gTimeoutMs,
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

      dbus_message_unref(reply);

      return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
   }


   printf("   checkAdminMsg ==> busName: %s | objName: %s | notificationFlag: %d | gTimeoutMs: %u\n\n", busName, objName, notificationFlag, gTimeoutMs);
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

   if (!dbus_connection_send(connection, reply, NULL))
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

   //printf("checkPersAdminMsg '%s' -> '%s'\n", dbus_message_get_interface(message), dbus_message_get_member(message));
   if((0==strcmp("org.genivi.persistence.admin", dbus_message_get_interface(message))))
   {
      if((0==strcmp("RegisterPersAdminNotification", dbus_message_get_member(message))))
      {
         printf(" ==> org.genivi.persistence.admin - received - ==> RegisterPersAdminNotification \n");

         result = checkAdminMsg(connection, message);
      }
      else if((0==strcmp("UnRegisterPersAdminNotification", dbus_message_get_member(message))))
      {
         printf(" ==> org.genivi.persistence.admin - received - ==> UnRegisterPersAdminNotification \n");

         result = checkAdminMsg(connection, message);
      }
      else
      {
         printf(" ==> org.genivi.persistence.admin - received U N KN O W N-'%s'\n", dbus_message_get_interface(message));
      }
   }
   else
   {
      printf(" ==> org.genivi.persistence - received U N KN O W N-'%s'\n", dbus_message_get_interface(message));
   }
   return result;
}


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

   return result;
}



static void  unregisterObjectPathFallback(DBusConnection *connection, void *user_data)
{
   printf("unregisterObjectPathFallback\n");
}






static dbus_bool_t addWatch(DBusWatch *watch, void *data)
{
   dbus_bool_t result = FALSE;

   //printf("addWatch called @%08x flags: %08x enabled: %c\n", (unsigned int)watch, dbus_watch_get_flags(watch), TRUE==dbus_watch_get_enabled(watch)?'x':'-');

   if (ARRAY_SIZE(gPollInfo.fds)>gPollInfo.nfds)
   {
      int flags = dbus_watch_get_flags(watch);

      gPollInfo.watches[gPollInfo.nfds] = watch;

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
         /* wakeup main-loop, just in case */
         static const uint64_t cmd = CMD_REQUEST_NAME;
         if (sizeof(uint64_t)!=write(gEfds, &cmd, sizeof(uint64_t)))
         {
            fprintf(stderr, "write failed w/ errno %d\n", errno);
         }
      }

      result = TRUE;
   }

   return result;
}


static void removeWatch(DBusWatch *watch, void *data)
{
   void* w_data = dbus_watch_get_data(watch);

   printf("removeWatch called @0x%08x\n", (int)watch);

   if(w_data)
      free(w_data);

   dbus_watch_set_data(watch, NULL, NULL);
}


static void watchToggled(DBusWatch *watch, void *data)
{
   printf("watchToggled called @0x%08x\n", (int)watch);

   if(dbus_watch_get_enabled(watch))
      addWatch(watch, data);
   else
      removeWatch(watch, data);
}


int mainLoop(DBusObjectPathVTable vtable, DBusObjectPathVTable vtableFallback, void* userData)
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
      printf("connected as '%s'\n", dbus_bus_get_unique_name(conn));
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

         // register for messages
         if (   (TRUE==dbus_connection_register_object_path(conn, "/org/genivi/persistence/admin", &vtable, userData))
             && (TRUE==dbus_connection_register_fallback(conn, "/", &vtableFallback, userData)) )
         {
            if (TRUE!=dbus_connection_set_watch_functions(conn, addWatch, removeWatch, watchToggled, NULL, NULL))
            {
               printf("dbus_connection_set_watch_functions() failed\n");
            }
            else
            {
               uint16_t buf[64];

               pthread_cond_signal(&gDbusInitializedCond);
               pthread_mutex_unlock(&gDbusInitializedMtx);
               do
               {
                  bContinue = 0; /* assume error */

                  while(DBUS_DISPATCH_DATA_REMAINS==dbus_connection_dispatch(conn));

                  while((-1==(ret=poll(gPollInfo.fds, gPollInfo.nfds, 500)))&&(EINTR==errno));

                  if(0>ret)
                  {
                     printf("poll() failed w/ errno %d\n", errno);
                  }
                  else
                  {
                     int i;
                     bContinue = 1;

                     for (i=0; gPollInfo.nfds>i; ++i)
                     {
                        if (0!=gPollInfo.fds[i].revents)
                        {
                           if (gPollInfo.fds[i].fd==gEfds)
                           {
                              if (0!=(gPollInfo.fds[i].revents & POLLIN))
                              {
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
                                    case CMD_REQUEST_NAME:
                                       if (DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER !=dbus_bus_request_name(conn, "org.genivi.persistence.admin", DBUS_NAME_FLAG_DO_NOT_QUEUE, &err))
                                       {
                                          fprintf(stderr, "Cannot acquire name 'org.genivi.persistence.admin': \n    \"(%s)\". Bailing out!\n", err.message);
                                          dbus_error_free(&err);
                                          bContinue = FALSE;
                                       }
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
                              //printf("handle watch @0x%08x flags: %04x\n", (int)gPollInfo.watches[i], flags);
                              bContinue = dbus_watch_handle(gPollInfo.watches[i], flags);
                           }
                        }
                     }
                  }
                  if(endLoop == 1)
                     break;
               }
               while (0!=bContinue);
            }
            dbus_connection_unregister_object_path(conn, "/org/genivi/persistence/admin");
            dbus_connection_unregister_object_path(conn, "/");
         }
         close(gEfds);
      }
      dbus_connection_close(conn);
      dbus_connection_unref(conn);
      dbus_shutdown();
   }

   pthread_cond_signal(&gDbusInitializedCond);
   pthread_mutex_unlock(&gDbusInitializedMtx);
   return 0;
}


void* run_mainloop(void* dataPtr)
{
   // persistence admin message
   static const struct DBusObjectPathVTable vtablePersAdmin
      = {unregisterMessageHandler, checkPersAdminMsg, NULL, };

   // fallback
   static const struct DBusObjectPathVTable vtableFallback
      = {unregisterObjectPathFallback, handleObjectPathMessageFallback, NULL, };

   // setup the dbus
   mainLoop(vtablePersAdmin, vtableFallback, dataPtr);

   printf("Exit dbus main loop!!!!\n");

   return NULL;
}


int setup_dbus_mainloop(void)
{
   int rval = 0;
   pthread_t thread;
   DBusError err;
   const char *pAddress = getenv("PERS_CLIENT_DBUS_ADDRESS");
   dbus_error_init(&err);

   // enable locking of data structures in the D-Bus library for multi threading.
   dbus_threads_init_default();

   // Connect to the bus and check for errors
   if(pAddress != NULL)
   {
      printf("Use specific dbus address: %s\n !", pAddress);
      gDbusConn = dbus_connection_open_private(pAddress, &err);

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
      printf("Use default dbus bus!!!!!!\n");
      gDbusConn = dbus_bus_get_private(DBUS_BUS_SYSTEM, &err);
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



int main(int argc, char *argv[])
{
   setup_dbus_mainloop();

   printf("Wait, press enter to exit!!\n");
   getchar();
   printf("Exiting Persistence Admin mockup!!\n");

   return 0;
}

