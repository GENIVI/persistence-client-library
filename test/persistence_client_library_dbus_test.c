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

#include <stdio.h>

#include <dlt/dlt.h>
#include <dlt/dlt_common.h>


int myChangeCallback(pclNotification_s * notifyStruct)
{
   printf(" ==> * - * myChangeCallback * - *\n");
   printf("Notification received ==> lbid: %d | resource_id: %s | seat: %d | user: %d | status: %d \n", notifyStruct->ldbid,
         notifyStruct->resource_id,
         notifyStruct->seat_no,
         notifyStruct->user_no,
         notifyStruct->pclKeyNotify_Status );
   printf(" <== * - * myChangeCallback * - *\n");

   return 1;
}


int main(int argc, char *argv[])
{
   int ret = 0;
   unsigned int shutdownReg = PCL_SHUTDOWN_TYPE_FAST | PCL_SHUTDOWN_TYPE_NORMAL;

   printf("Dbus interface test application\n");

   /// debug log and trace (DLT) setup
   DLT_REGISTER_APP("noty","tests the persistence client library");
   ret = pclInitLibrary("lt-persistence_client_library_dbus_test", shutdownReg);
   printf("pclInitLibrary: %d\n", ret);

#if 0
   printf("Press a key to end application\n");
   ret = pclKeyHandleOpen(0xFF, "posHandle/last_position", 0, 0);

   printf("Register for change notification\n");
   ret = pclKeyRegisterNotifyOnChange(0x84, "links/last_link2", 2/*user_no*/, 1/*seat_no*/, &myChangeCallback);
   ret = pclKeyRegisterNotifyOnChange(0x84, "links/last_link3", 3/*user_no*/, 2/*seat_no*/, &myChangeCallback);
   ret = pclKeyRegisterNotifyOnChange(0x84, "links/last_link4", 4/*user_no*/, 1/*seat_no*/, &myChangeCallback);

   getchar();
#endif
   sleep(2);
   pclDeinitLibrary();


   // unregister debug log and trace
   DLT_UNREGISTER_APP();

   dlt_free();

   printf("By\n");
   return ret;
}
