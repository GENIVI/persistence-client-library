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
 * @file           persistence_client_library_dbus_service.c
 * @ingroup        Persistence client library
 * @author         Ingo Huerner
 * @brief          Implementation of the persistence client library dbus service.
 * @see
 */

//#include "persistence_client_service_dbus_service.h"

#include "persistence_client_library_dbus_service.h"
#include "persistence_client_library_lc_interface.h"
#include "persistence_client_library_pas_interface.h"

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>


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

//const char* gPersDbusAdminInterface    =  "org.genivi.persistence.admin";
//const char* gPersDbusAdminPath         = "/org/genivi/persistence/admin";



#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))


/* function to unregister ojbect path message handler */
static void unregisterMessageHandler(DBusConnection *connection, void *user_data)
{
   printf("unregisterObjectPath\n");
}

/* catches messages not directed to any registered object path ("garbage collector") */
static DBusHandlerResult handleObjectPathMessageFallback(DBusConnection * connection, DBusMessage * message, void * user_data)
{
   printf("handleObjectPathMessageFallback '%s' -> '%s'\n", dbus_message_get_interface(message), dbus_message_get_member(message) );
   return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}



static void  unregisterObjectPathFallback(DBusConnection *connection, void *user_data)
{
   printf("unregisterObjectPathFallback\n");
}



void* run_mainloop(void* dataPtr)
{
   // lock mutex to make sure dbus main loop is running
   pthread_mutex_lock(&gDbusInitializedMtx);

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
      printf("Use default dbus bus!!!!!!\n");
      gDbusConn = dbus_bus_get(DBUS_BUS_SYSTEM, &err);
   }

   // create here the dbus connection and pass to main loop
   rval = pthread_create(&thread, NULL, run_mainloop, gDbusConn);

   if (rval)
   {
     fprintf(stderr, "Server: - ERROR! pthread_create( run_mainloop ) returned: %d\n", rval);
   }
   return rval;
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

         static const int cmd = CMD_REQUEST_NAME;
         if (sizeof(int)!=write(gPipefds[1], &cmd, sizeof(int)))
         {
            printf("write failed w/ errno %d\n", errno);
         }
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



int mainLoop(DBusObjectPathVTable vtable, DBusObjectPathVTable vtable2,
             DBusObjectPathVTable vtableFallback, void* userData)
{
   DBusError err;

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

      if (0!=pipe(gPipefds))
      {
         printf("pipe() failed w/ errno %d\n", errno);
      }
      else
      {
         int ret;
         int bContinue = 0;
         memset(&gPollInfo, 0 , sizeof(gPollInfo));

         gPollInfo.nfds = 1;
         gPollInfo.fds[0].fd = gPipefds[0];
         gPollInfo.fds[0].events = POLLIN;

         if (   (TRUE==dbus_connection_register_object_path(conn, "/org/genivi/persistence/adminconsumer", &vtable, userData))
             && (TRUE==dbus_connection_register_object_path(conn, "/com/contiautomotive/NodeStateManager/LifecycleConsumer", &vtable2, userData))
             && (TRUE==dbus_connection_register_fallback(conn, "/", &vtableFallback, userData)) )
         {
            if (TRUE!=dbus_connection_set_watch_functions(conn, addWatch, removeWatch, watchToggled, NULL, NULL))
            {
               printf("dbus_connection_set_watch_functions() failed\n");
            }
            else
            {
               char buf[64];

               // minloop is running now, release mutex
               pthread_mutex_unlock(&gDbusInitializedMtx);
               do
               {
                  bContinue = 0; /* assume error */

                  while (DBUS_DISPATCH_DATA_REMAINS==dbus_connection_dispatch(conn));

                  while ((-1==(ret=poll(gPollInfo.fds, gPollInfo.nfds, 500)))&&(EINTR==errno));

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
                           if (gPollInfo.fds[i].fd==gPipefds[0])
                           {
                              if (0!=(gPollInfo.fds[i].revents & POLLIN))
                              {
                                 bContinue = TRUE;
                                 while ((-1==(ret=read(gPollInfo.fds[i].fd, buf, 64)))&&(EINTR==errno));
                                 if (0>ret)
                                 {
                                    printf("read() failed w/ errno %d\n", errno);
                                 }
                                 else if (sizeof(int)==ret)
                                 {
                                    switch (buf[0])
                                    {
                                       case CMD_REQUEST_NAME:
                                          if(DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER
                                             != dbus_bus_request_name(conn, "org.genivi.persistence.adminconsumer", DBUS_NAME_FLAG_DO_NOT_QUEUE, &err))
                                          {
                                             printf("*** Cannot acquire name '%s' (%s). Bailing out!\n", "org.genivi.persistence.admin\n", err.message);
                                             dbus_error_free(&err);
                                             bContinue = FALSE;
                                          }
                                          if(DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER
                                             != dbus_bus_request_name(conn, "com.contiautomotive.NodeStateManager.LifecycleConsumer", DBUS_NAME_FLAG_DO_NOT_QUEUE, &err))
                                          {
                                             printf("*** Cannot acquire name '%s' (%s). Bailing out!\n", "com.contiautomotive.NodeStateManager.LifecycleConsumer\n", err.message);
                                             dbus_error_free(&err);
                                             bContinue = FALSE;
                                          }
                                          break;
                                       case CMD_PAS_BLOCK_AND_WRITE_BACK:
                                          process_block_and_write_data_back();
                                          break;
                                       case CMD_LC_PREPARE_SHUTDOWN:
                                          process_prepare_shutdown(buf[1]);
                                          break;
                                       case CMD_QUIT:
                                          bContinue = FALSE;
                                          break;
                                       default:
                                          printf("command %d not handled!\n", buf[0]);
                                          break;
                                    }
                                 }
                                 else
                                 {
                                    printf("read() returned %d (%s)\n", ret, buf);
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
            dbus_connection_unregister_object_path(conn, "/org/genivi/persistence/adminconsumer");
            //dbus_connection_unregister_object_path(conn, "/com/");
            dbus_connection_unregister_object_path(conn, "/");
         }
         close(gPipefds[1]);
         close(gPipefds[0]);
      }
      dbus_connection_unref(conn);
      dbus_shutdown();
   }
   return 0;
}

