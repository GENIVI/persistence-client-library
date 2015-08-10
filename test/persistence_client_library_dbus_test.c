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
 * @file           persistence_client_library_dbus_test.c
 * @ingroup        Persistence client library test
 * @author         Ingo Huerner
 * @brief          Test of persistence client library
 * @see            
 */

#include "../include/persistence_client_library_key.h"
#include "../include/persistence_client_library_file.h"
#include "../include/persistence_client_library_error_def.h"

#include "../src/rbtree.h"
#include "../src/persistence_client_library_tree_helper.h"

#include <stdio.h>
#include <stdlib.h>

#include <pthread.h>
#include <dlt.h>
#include <dlt_common.h>


#define READ_BUFFER_SIZE   124

pthread_mutex_t gMtx   = PTHREAD_MUTEX_INITIALIZER;



int myChangeCallback(pclNotification_s * notifyStruct)
{
   printf(" ==> * - *** myChangeCallback *** - *\n");

   printf("Notification received ==> lbid: %d | resource_id: %s | seat: %d | user: %d | status: %d \n", notifyStruct->ldbid,
         notifyStruct->resource_id,
         notifyStruct->seat_no,
         notifyStruct->user_no,
         notifyStruct->pclKeyNotify_Status );

   pthread_mutex_unlock(&gMtx);

   printf(" <== * - *** myChangeCallback *** - *\n");

   return 1;
}



int main(int argc, char *argv[])
{
   int ret = 0, i = 0;
   unsigned int shutdownReg = PCL_SHUTDOWN_TYPE_FAST | PCL_SHUTDOWN_TYPE_NORMAL;

   unsigned char readBuffer[READ_BUFFER_SIZE] = {0};

   const char* appID = "lt-persistence_client_library_dbus_test";

   (void)argc;
   (void)argv;

   printf("Dbus interface test application\n");

   /// debug log and trace (DLT) setup
   DLT_REGISTER_APP("noty","tests the persistence client library");
   ret = pclInitLibrary(appID, shutdownReg);
   printf("pclInitLibrary - %s - : %d\n", appID, ret);

   ret = pclKeyHandleOpen(PCL_LDBID_LOCAL, "posHandle/last_position", 0, 0);

   printf("Register for change notification\n");
   ret = pclKeyRegisterNotifyOnChange(0x20, "links/last_link2", 2/*user_no*/, 1/*seat_no*/, &myChangeCallback);


#if 0
   ret = pclKeyRegisterNotifyOnChange(0x20, "links/last_link3", 3/*user_no*/, 2/*seat_no*/, &myChangeCallback);
   ret = pclKeyRegisterNotifyOnChange(0x20, "links/last_link4", 4/*user_no*/, 1/*seat_no*/, &myChangeCallback);

   ret = pclKeyRegisterNotifyOnChange(PCL_LDBID_LOCAL, "69",     1/*user_no*/, 2/*seat_no*/, &myChangeCallback);
   printf("Reg => 69: %d\n", ret);
   ret = pclKeyRegisterNotifyOnChange(PCL_LDBID_LOCAL, "70",     1/*user_no*/, 2/*seat_no*/, &myChangeCallback);
   printf("Reg => 70: %d\n", ret);
   ret = pclKeyRegisterNotifyOnChange(PCL_LDBID_LOCAL, "key_70", 1/*user_no*/, 2/*seat_no*/, &myChangeCallback);
   printf("Reg => key_70: %d\n", ret);

   printf("Press enter to unregister to notifications\n");
   getchar();

   ret = pclKeyUnRegisterNotifyOnChange(0x20, "links/last_link2", 2/*user_no*/, 1/*seat_no*/, &myChangeCallback);
   printf("UnReg => last_link2: %d\n", ret);
   printf("Press enter to proceed\n");
   getchar();

   ret = pclKeyUnRegisterNotifyOnChange(0x20, "links/last_link3", 3/*user_no*/, 2/*seat_no*/, &myChangeCallback);
   printf("UnReg => last_link3: %d\n", ret);
   printf("Press enter to proceed\n");
   getchar();

   ret = pclKeyUnRegisterNotifyOnChange(0x20, "links/last_link4", 4/*user_no*/, 1/*seat_no*/, &myChangeCallback);
   printf("UnReg => last_link4: %d\n", ret);

   printf("Press enter to register to notifications\n");
   getchar();

   ret = pclKeyRegisterNotifyOnChange(0x20, "links/last_link2", 2/*user_no*/, 1/*seat_no*/, &myChangeCallback);
   ret = pclKeyRegisterNotifyOnChange(0x20, "links/last_link3", 3/*user_no*/, 2/*seat_no*/, &myChangeCallback);
   ret = pclKeyRegisterNotifyOnChange(0x20, "links/last_link4", 4/*user_no*/, 1/*seat_no*/, &myChangeCallback);

   printf("Press enter to end\n");
   getchar();

   sleep(2);

#else



   while(i<18)
   {
      memset(readBuffer, 0, READ_BUFFER_SIZE);
      pthread_mutex_lock(&gMtx);

      pclKeyReadData(0x20, "links/last_link2", 2, 1, readBuffer, READ_BUFFER_SIZE);
      printf("%d - Read value of resource \"links/last_link2\" = %s \n\n\n", i++, readBuffer);
   }

#endif


   pclDeinitLibrary();


   // unregister debug log and trace
   DLT_UNREGISTER_APP();

   dlt_free();

   printf("By\n");
   return ret;
}

