/******************************************************************************
 * Project         Persistence
 * (c) copyright   2012
 * Company         XS Embedded GmbH
 *****************************************************************************/
/******************************************************************************
 * This Source Code Form is subject to the terms of the
 * Mozilla Public License, v. 2.0. If a  copy of the MPL was not distributed
 * with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
******************************************************************************/
 /**
 * @file           persistence_client_library_test.c
 * @author         Ingo Huerner
 * @brief          Test of persistence client library
 * @see            
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>     /* exit */
#include <time.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <dbus/dbus.h>
#include <dlt.h>
#include <dlt_common.h>
#include <pthread.h>

#include <check.h>

#include "../include/persistence_client_library_key.h"
#include "../include/persistence_client_library.h"
#include "../include/persistence_client_library_error_def.h"

#define SKIP_MULTITHREADED_TESTS 1

#define BUF_SIZE        64
#define NUM_OF_FILES    3
#define READ_SIZE       1024
#define MaxAppNameLen   256

#define NUM_THREADS     20
#define NUM_OF_READS    100
#define NUM_OF_WRITES   100
#define NAME_LEN     24

typedef struct s_threadData
{
   char threadName[NAME_LEN];
   int index;
   int fd1;
   int fd2;
} t_threadData;

static pthread_barrier_t gBarrierOne, gBarrierTwo;

/// application id
char gTheAppId[MaxAppNameLen] = {0};

// definition of weekday
char* dayOfWeek[] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
char* gWriteBackupTestData  = "This is the content of the file /Data/mnt-c/lt-persistence_client_library_test/user/1/seat/1/media/mediaDB_ReadWrite.db";
char* gWriteRecoveryTestData = "This is the data to recover: /Data/mnt-c/lt-persistence_client_library_test/user/1/seat/1/media/mediaDB_DataRecovery.db";
char* gRecovChecksum = "608a3b5d";  // generated with http://www.tools4noobs.com/online_php_functions/crc32/

extern const char* gWriteBuffer;
extern const char* gWriteBuffer2;


/// debug log and trace (DLT) setup
DLT_DECLARE_CONTEXT(gPcltDLTContext);


// function prototype
void run_concurrency_test();

void data_setup(void)
{
   int shutdownReg = PCL_SHUTDOWN_TYPE_FAST | PCL_SHUTDOWN_TYPE_NORMAL;
   const char* envVariable = "PERS_CLIENT_LIB_CUSTOM_LOAD";

   setenv(envVariable, "/etc/pclCustomLibConfigFileTest.cfg", 1);

   (void)pclInitLibrary(gTheAppId, shutdownReg);
}


void data_setup_browser(void)
{
   int shutdownReg = PCL_SHUTDOWN_TYPE_FAST | PCL_SHUTDOWN_TYPE_NORMAL;
   (void)pclInitLibrary("browser", shutdownReg);
}


void data_setup_norct(void)
{
   int shutdownReg = PCL_SHUTDOWN_TYPE_FAST | PCL_SHUTDOWN_TYPE_NORMAL;

   (void)pclInitLibrary("norct", shutdownReg);
}


void data_teardown(void)
{
   pclDeinitLibrary();
}


int myChangeCallback(pclNotification_s * notifyStruct)
{
   printf(" ==> * - * myChangeCallback * - *\n");
   (void)notifyStruct;
   return 1;
}



/**
 * Test the key value interface using different logicalDB id's, users and seats.
 * Each resource below has an entry in the resource configuration table where the
 * storage location (cached or write through) and type (e.g. custom) has been configured.
 */
START_TEST(test_GetData)
{
   int ret = 0;
   unsigned char buffer[READ_SIZE] = {0};

   DLT_LOG(gPcltDLTContext, DLT_LOG_INFO, DLT_STRING("PCL_TEST test_GetData"));

   /**
    * Logical DB ID: PCL_LDBID_LOCAL with user 0 and seat 0
    *       ==> local value accessible by all users (user 0, seat 0)
    */
   ret = pclKeyReadData(PCL_LDBID_LOCAL, "pos/last_position",         1, 1, buffer, READ_SIZE);
   //printf("----test_GetData => pos/last_position: \"%s\" => ret: %d \nReference: %s => size: %d\n", buffer, ret, "CACHE_ +48 10' 38.95, +8 44' 39.06", strlen("CACHE_ +48 10' 38.95, +8 44' 39.06"));
   ck_assert_str_eq( (char*)buffer, "CACHE_ +48 10' 38.95, +8 44' 39.06");
   ck_assert_int_eq( ret,  (int)strlen("CACHE_ +48 10' 38.95, +8 44' 39.06") );
   memset(buffer, 0, READ_SIZE);

   /**
    * Logical DB ID: PCL_LDBID_LOCAL with user 3 and seat 2
    *       ==> local USER value (user 3, seat 2)
    */
   ret = pclKeyReadData(PCL_LDBID_LOCAL, "status/open_document",      3, 2, buffer, READ_SIZE);
   //printf("----test_GetData => status/open_document \"%s\" => ret: %d \n", buffer, ret);
   ck_assert_str_eq( (char*)buffer, "WT_ /var/opt/user_manual_climateControl.pdf");
   ck_assert_int_eq(ret, (int)strlen("WT_ /var/opt/user_manual_climateControl.pdf"));
   memset(buffer, 0, READ_SIZE);

   /**
    * Logical DB ID: 0x20 with user 4 and seat 0
    *       ==> shared user value accessible by a group (user 4 and seat 0)
    */
   ret = pclKeyReadData(0x20, "address/home_address",      4, 0, buffer, READ_SIZE);
   //printf("----test_GetData => address/home_address \"%s\" => ret: %d \n", buffer, ret);
   ck_assert_str_eq( (char*)buffer, "WT_ 55327 Heimatstadt, Wohnstrasse 31");
   ck_assert_int_eq(ret, (int)strlen("WT_ 55327 Heimatstadt, Wohnstrasse 31"));
   memset(buffer, 0, READ_SIZE);

   /**
    * Logical DB ID: PCL_LDBID_LOCAL with user 0 and seat 0
    *       ==> local value accessible by ALL USERS (user 0, seat 0)
    */
   ret = pclKeyReadData(PCL_LDBID_LOCAL, "pos/last_satellites",       0, 0, buffer, READ_SIZE);
   //printf("----test_GetData => pos/last_satellites \"%s\" => ret: %d \n", buffer, ret);
   ck_assert_str_eq( (char*)buffer, "WT_ 17");
   ck_assert_int_eq(ret, (int)strlen("WT_ 17"));
   memset(buffer, 0, READ_SIZE);

   /**
    * Logical DB ID: 0x20 with user 4 and seat 0
    *       ==> shared user value accessible by A GROUP (user 4 and seat 0)
    */
   ret = pclKeyReadData(0x20, "links/last_link",           2, 0, buffer, READ_SIZE);
   //printf("----test_GetData => links/last_link \"%s\" => ret: %d \n", buffer, ret);
   ck_assert_str_eq( (char*)buffer, "CACHE_ /last_exit/queens");
   ck_assert_int_eq(ret, (int)strlen("CACHE_ /last_exit/queens"));
   memset(buffer, 0, READ_SIZE);
}
END_TEST


/**
 * Test the key value  h a n d l e  interface using different logicalDB id's, users and seats
 * Each resource below has an entry in the resource configuration table where
 * the storage location (cached or write through) and type (e.g. custom) has bee configured.
 */
START_TEST (test_GetDataHandle)
{
   int ret = 0, handle = 0, handle2 = 0, handle3 = 0, handle4 = 0, size = 0;

   unsigned char buffer[READ_SIZE] = {0};
   struct tm *locTime;

   char sysTimeBuffer[128];

   DLT_LOG(gPcltDLTContext, DLT_LOG_INFO, DLT_STRING("PCL_TEST test_GetDataHandle"));

   time_t t = time(0);

   locTime = localtime(&t);

   snprintf(sysTimeBuffer, 128, "TimeAndData: \"%s %d.%d.%d - %d:%.2d:%.2d Uhr\"", dayOfWeek[locTime->tm_wday], locTime->tm_mday, locTime->tm_mon+1, (locTime->tm_year+1900),
                                                                  locTime->tm_hour, locTime->tm_min, locTime->tm_sec);
   // open handle ---------------------------------------------------
   /**
    * Logical DB ID: PCL_LDBID_LOCAL with user 0 and seat 0
    *       ==> local value accessible by ALL USERS (user 0, seat 0)
    */
   handle = pclKeyHandleOpen(PCL_LDBID_LOCAL, "posHandle/last_position", 0, 0);
   ck_assert_int_gt(handle, 0);

   ret = pclKeyHandleReadData(handle, buffer, READ_SIZE);
   //printf("pclKeyHandleReadData: \nsoll: %s \nist : %s => ret: %d | strlen: %d\n", "WT_ H A N D L E: +48° 10' 38.95\", +8° 44' 39.06\"", buffer, ret, strlen("WT_ H A N D L E: +48° 10' 38.95\", +8° 44' 39.06\""));
   fail_unless(strncmp((char*)buffer, "WT_ H A N D L E: +48° 10' 38.95\", +8° 44' 39.06\"", (size_t)ret) == 0, "Buffer not correctly read => 1");

   size = pclKeyHandleGetSize(handle);
   //printf("pclKeyHandleGetSize => size: %d\n", size);
   ck_assert_int_eq(size, (int)strlen("WT_ H A N D L E: +48° 10' 38.95\", +8° 44' 39.06\""));
   // ---------------------------------------------------------------------------------------------

   // open handle ---------------------------------------------------
   /**
    * Logical DB ID: PCL_LDBID_LOCAL with user 3 and seat 2
    *       ==> local USER value (user 3, seat 2)
    */
   handle2 = pclKeyHandleOpen(PCL_LDBID_LOCAL, "statusHandle/open_document", 3, 2);
   fail_unless(handle2 >= 0, "Failed to open handle /statusHandle/open_document");

   size = pclKeyHandleWriteData(handle2, (unsigned char*)sysTimeBuffer, (int)strlen(sysTimeBuffer));
   fail_unless(size == (int)strlen(sysTimeBuffer));
   // close
   ret = pclKeyHandleClose(handle2);
   // ---------------------------------------------------------------------------------------------

   // open handle ---------------------------------------------------
   /**
    * Logical DB ID: PCL_LDBID_LOCAL with user 3 and seat 2
    *       ==> local USER value (user 3, seat 2)
    */
   handle3 = pclKeyHandleOpen(PCL_LDBID_LOCAL, "statusHandle/open_document", 3, 2);
   fail_unless(handle3 >= 0, "Failed to open handle /statusHandle/open_document");

   ret = pclKeyHandleReadData(handle3, buffer, READ_SIZE);
   fail_unless(strncmp((char*)buffer, sysTimeBuffer, strlen(sysTimeBuffer)) == 0, "Buffer not correctly read => 3");

   size = pclKeyHandleGetSize(handle3);
   fail_unless(size = (int)strlen(sysTimeBuffer));
   // ---------------------------------------------------------------------------------------------

   // close handle
   ret = pclKeyHandleClose(handle);
   ret = pclKeyHandleClose(handle3);
   ret = pclKeyHandleClose(handle4);
}
END_TEST


/*
 * Write data to a key using the key interface.
 * First write data to different keys and after
 * read the data for verification.
 */
START_TEST(test_SetData)
{
   int ret = 0;
   unsigned char buffer[READ_SIZE]  = {0};

   char write1[READ_SIZE] = {0};
   char write2[READ_SIZE] = {0};
   char sysTimeBuffer[256];

   struct tm *locTime;

   DLT_LOG(gPcltDLTContext, DLT_LOG_INFO, DLT_STRING("PCL_TEST test_SetData"));

   /**
    * Logical DB ID: PCL_LDBID_LOCAL with user 3 and seat 2
    *       ==> local USER value (user 3, seat 2)
    */
   ret = pclKeyWriteData(PCL_LDBID_LOCAL, "status/open_document",      3, 2, (unsigned char*)"WT_ /var/opt/user_manual_climateControl.pdf", strlen("WT_ /var/opt/user_manual_climateControl.pdf"));
   fail_unless(ret == strlen("WT_ /var/opt/user_manual_climateControl.pdf"), "Wrong write size");


   ret = pclKeyWriteData(0x84, "links/last_link",      2, 1, (unsigned char*)"CACHE_ /last_exit/queens", strlen("CACHE_ /last_exit/queens"));
   fail_unless(ret == strlen("CACHE_ /last_exit/queens"), "Wrong write size");

   ret = pclKeyWriteData(PCL_LDBID_LOCAL, "posHandle/last_position", 0, 0, (unsigned char*)"WT_ H A N D L E: +48° 10' 38.95\", +8° 44' 39.06\"", strlen("WT_ H A N D L E: +48° 10' 38.95\", +8° 44' 39.06\""));
   fail_unless(ret == strlen("WT_ H A N D L E: +48° 10' 38.95\", +8° 44' 39.06\""), "Wrong write size");

   time_t t = time(0);
   locTime = localtime(&t);

   // write data
   snprintf(sysTimeBuffer, 128, "\"%s %d.%d.%d - %d:%.2d:%.2d Uhr\"", dayOfWeek[locTime->tm_wday], locTime->tm_mday, locTime->tm_mon+1, (locTime->tm_year+1900),
                                                                      locTime->tm_hour, locTime->tm_min, locTime->tm_sec);

   /**
    * Logical DB ID: PCL_LDBID_LOCAL with user 1 and seat 2
    *       ==> local USER value (user 1, seat 2)
    * Resource ID: 69
    */
   ret = pclKeyWriteData(PCL_LDBID_LOCAL, "69", 1, 2, (unsigned char*)sysTimeBuffer, (int)strlen(sysTimeBuffer));
   fail_unless(ret == (int)strlen(sysTimeBuffer), "Wrong write size");

   snprintf(write1, 128, "%s %s", "/70",  sysTimeBuffer);
   /**
    * Logical DB ID: PCL_LDBID_LOCAL with user 1 and seat 2
    *       ==> local USER value (user 1, seat 2)
    * Resource ID: 70
    */
   ret = pclKeyWriteData(PCL_LDBID_LOCAL, "70", 1, 2, (unsigned char*)write1, (int)strlen(write1));
   fail_unless(ret == (int)strlen(write1), "Wrong write size");

   snprintf(write2, 128, "%s %s", "/key_70",  sysTimeBuffer);
   /**
    * Logical DB ID: PCL_LDBID_LOCAL with user 1 and seat 2
    *       ==> local USER value (user 1, seat 2)
    * Resource ID: key_70
    */
   ret = pclKeyWriteData(PCL_LDBID_LOCAL, "key_70", 1, 2, (unsigned char*)write2, (int)strlen(write2));
   fail_unless(ret == (int)strlen(write2), "Wrong write size");


   /*******************************************************************************************************************************************/
   /* used for changed notification testing */
   /*******************************************************************************************************************************************/
#if 0
   /**
    * Logical DB ID: 0x84 with user 2 and seat 1
    *       ==> shared user value accessible by A GROUP (user 2 and seat 1)
    *
    *       ==> used for shared testing
    */
   //printf("Write data to trigger change notification\n");
   ret = pclKeyWriteData(0x20, "links/last_link2",  2, 1, (unsigned char*)"Test notify shared data", strlen("Test notify shared data"));
   printf("Ist: %d - Soll: %d\n", ret, (int)strlen("Test notify shared data"));
   fail_unless(ret == (int)strlen("Test notify shared data"), "Wrong write size");

   /**
    * Logical DB ID: 0x84 with user 2 and seat 1
    *       ==> shared user value accessible by A GROUP (user 2 and seat 1)
    *
    *       ==> used for shared testing
    */
   //printf("Write data to trigger change notification\n");
   ret = pclKeyWriteData(0x20, "links/last_link3",  3, 2, (unsigned char*)"Test notify shared data", strlen("Test notify shared data"));
   fail_unless(ret == (int)strlen("Test notify shared data"), "Wrong write size");

   /**
    * Logical DB ID: 0x84 with user 2 and seat 1
    *       ==> shared user value accessible by A GROUP (user 2 and seat 1)
    *
    *       ==> used for shared testing
    */
   //printf("Write data to trigger change notification\n");
   ret = pclKeyWriteData(0x20, "links/last_link4",  4, 1, (unsigned char*)"Test notify shared data", strlen("Test notify shared data"));
   fail_unless(ret == strlen("Test notify shared data"), "Wrong write size");
#endif
   /*******************************************************************************************************************************************/
   /*******************************************************************************************************************************************/


   /*
    * now read the data written in the previous steps to the keys
    * and verify data has been written correctly.
    */
   memset(buffer, 0, READ_SIZE);

   ret = pclKeyReadData(PCL_LDBID_LOCAL, "69", 1, 2, buffer, READ_SIZE);
   fail_unless(strncmp((char*)buffer, sysTimeBuffer, strlen(sysTimeBuffer)) == 0, "Buffer not correctly read");
   fail_unless(ret == (int)strlen(sysTimeBuffer), "Wrong read size");

   memset(buffer, 0, READ_SIZE);

   ret = pclKeyReadData(PCL_LDBID_LOCAL, "70", 1, 2, buffer, READ_SIZE);
   fail_unless(strncmp((char*)buffer, write1, strlen(write1)) == 0, "Buffer not correctly read");
   fail_unless(ret == (int)strlen(write1), "Wrong read size");

   memset(buffer, 0, READ_SIZE);

   ret = pclKeyReadData(PCL_LDBID_LOCAL, "key_70", 1, 2, buffer, READ_SIZE);
   fail_unless(strncmp((char*)buffer, write2, strlen(write2)) == 0, "Buffer not correctly read");
   fail_unless(ret == (int)strlen(write2), "Wrong read size");

}
END_TEST



/**
 * Write data to a key using the key interface.
 * The key is not in the persistence resource table.
 * The key sill then be stored to the location local and cached.
 */
START_TEST(test_SetDataNoPRCT)
{
   int ret = 0;
   unsigned char buffer[READ_SIZE] = {0};
   struct tm *locTime;

   time_t t = time(0);

   char sysTimeBuffer[128];

   DLT_LOG(gPcltDLTContext, DLT_LOG_INFO, DLT_STRING("PCL_TEST test_SetDataNoPRCT"));

   locTime = localtime(&t);

   snprintf(sysTimeBuffer, 128, "TimeAndData: \"%s %d.%d.%d - %d:%.2d:%.2d Uhr\"", dayOfWeek[locTime->tm_wday], locTime->tm_mday, locTime->tm_mon+1, (locTime->tm_year+1900),
                                                                  locTime->tm_hour, locTime->tm_min, locTime->tm_sec);

   /**
    * Logical DB ID: PCL_LDBID_LOCAL with user 1 and seat 2
    *       ==> local USER value (user 1, seat 2)
    */
   ret = pclKeyWriteData(PCL_LDBID_LOCAL, "NoPRCT", 1, 2, (unsigned char*)sysTimeBuffer, (int)strlen(sysTimeBuffer));
   fail_unless(ret == (int)strlen(sysTimeBuffer), "Wrong write size");
   //printf("Write Buffer : %s\n", sysTimeBuffer);

   // read data again and and verify datat has been written correctly
   memset(buffer, 0, READ_SIZE);

   ret = pclKeyReadData(PCL_LDBID_LOCAL, "NoPRCT", 1, 2, buffer, READ_SIZE);
   fail_unless(strncmp((char*)buffer, sysTimeBuffer, strlen(sysTimeBuffer)) == 0, "Buffer not correctly read");
   fail_unless(ret == (int)strlen(sysTimeBuffer), "Wrong read size");
   //printf("read buffer  : %s\n", buffer);
}
END_TEST



/*
 * Test the key interface.
 * Read the size of a key.
 */
START_TEST(test_GetDataSize)
{
   int size = 0;

   DLT_LOG(gPcltDLTContext, DLT_LOG_INFO, DLT_STRING("PCL_TEST test_GetDataSize"));
   /**
    * Logical DB ID: PCL_LDBID_LOCAL with user 3 and seat 2
    *       ==> local USER value (user 3, seat 2)
    */
   size = pclKeyGetSize(PCL_LDBID_LOCAL, "status/open_document", 3, 2);
   fail_unless(size == strlen("WT_ /var/opt/user_manual_climateControl.pdf"), "Invalid size");

   /**
    * Logical DB ID: 0x84 with user 2 and seat 1
    *       ==> shared user value accessible by A GROUP (user 2 and seat 1)
    */
   size = pclKeyGetSize(0x84, "links/last_link", 2, 1);
   fail_unless(size == strlen("CACHE_ /last_exit/queens"), "Invalid size");
}
END_TEST


/*
 * Delete a key using the key value interface.
 * First read a from a key, the delte the key
 * and then try to read again. The Last read must fail.
 */
START_TEST(test_DeleteData)
{
   int rval = 0;
   unsigned char buffer[READ_SIZE];

   DLT_LOG(gPcltDLTContext, DLT_LOG_INFO, DLT_STRING("PCL_TEST test_DeleteData"));

   // read data from key
   rval = pclKeyReadData(PCL_LDBID_LOCAL, "key_70", 1, 2, buffer, READ_SIZE);
   fail_unless(rval != EPERS_NOKEY, "Read form key key_70 fails");

   // delete key
   rval = pclKeyDelete(PCL_LDBID_LOCAL, "key_70", 1, 2);
   fail_unless(rval >= 0, "Failed to delete key");

   // after deleting the key, reading from key must fail now!
   rval = pclKeyReadData(PCL_LDBID_LOCAL, "key_70", 1, 2, buffer, READ_SIZE);
   fail_unless(rval == EPERS_NOKEY, "Read form key key_70 works, but should fail");



   // read data from key
   rval = pclKeyReadData(PCL_LDBID_LOCAL, "70", 1, 2, buffer, READ_SIZE);
   fail_unless(rval != EPERS_NOKEY, "Read form key 70 fails");

   // delete key
   rval = pclKeyDelete(PCL_LDBID_LOCAL, "70", 1, 2);
   fail_unless(rval >= 0, "Failed to delete key");

   // after deleting the key, reading from key must fail now!
   rval = pclKeyReadData(PCL_LDBID_LOCAL, "70", 1, 2, buffer, READ_SIZE);
   fail_unless(rval == EPERS_NOKEY, "Read form key 70 works, but should fail");
}
END_TEST



void data_setupBackup(void)
{
   int handle = -1;
   const char* path = "/Data/mnt-c/lt-persistence_client_library_test/user/1/seat/1/media/mediaDB_ReadWrite.db";

   int shutdownReg = PCL_SHUTDOWN_TYPE_FAST | PCL_SHUTDOWN_TYPE_NORMAL;
   (void)pclInitLibrary(gTheAppId, shutdownReg);

   handle = open(path, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
   if(write(handle, gWriteBackupTestData, strlen(gWriteBackupTestData)) == -1)
   {
      printf("setup test: failed to write test data: %s\n", path);
   }
}







/*
 * Extended key handle test.
 * Test have been created after a bug in the key handle function occured.
 */
START_TEST(test_DataHandleOpen)
{
   int hd1 = -2, hd2 = -2, hd3 = -2, hd4 = -2, hd5 = -2, hd6 = -2, hd7 = -2, hd8 = -2, hd9 = -2, ret = 0;

   DLT_LOG(gPcltDLTContext, DLT_LOG_INFO, DLT_STRING("PCL_TEST test_DataHandleOpen"));

   // open handles ----------------------------------------------------
   hd1 = pclKeyHandleOpen(PCL_LDBID_LOCAL, "posHandle/last_position1", 0, 0);
   fail_unless(hd1 == 1, "Failed to open handle ==> /posHandle/last_position1");

   hd2 = pclKeyHandleOpen(PCL_LDBID_LOCAL, "posHandle/last_position2", 0, 0);
   fail_unless(hd2 == 2, "Failed to open handle ==> /posHandle/last_position2");

   hd3 = pclKeyHandleOpen(PCL_LDBID_LOCAL, "posHandle/last_position3", 0, 0);
   fail_unless(hd3 == 3, "Failed to open handle ==> /posHandle/last_position3");

   // close handles ---------------------------------------------------
   ret = pclKeyHandleClose(hd1);
   fail_unless(ret != -1, "Failed to close handle!!");

   ret = pclKeyHandleClose(hd2);
   fail_unless(ret != -1, "Failed to close handle!!");

   ret = pclKeyHandleClose(hd3);
   fail_unless(ret != -1, "Failed to close handle!!");

   // open handles ----------------------------------------------------
   hd4 = pclKeyHandleOpen(PCL_LDBID_LOCAL, "posHandle/last_position4", 0, 0);
   fail_unless(hd4 == 3, "Failed to open handle ==> /posHandle/last_position4");

   hd5 = pclKeyHandleOpen(PCL_LDBID_LOCAL, "posHandle/last_position5", 0, 0);
   fail_unless(hd5 == 2, "Failed to open handle ==> /posHandle/last_position5");

   hd6 = pclKeyHandleOpen(PCL_LDBID_LOCAL, "posHandle/last_position6", 0, 0);
   fail_unless(hd6 == 1, "Failed to open handle ==> /posHandle/last_position6");

   hd7 = pclKeyHandleOpen(PCL_LDBID_LOCAL, "posHandle/last_position7", 0, 0);
   fail_unless(hd7 == 4, "Failed to open handle ==> /posHandle/last_position7");

   hd8 = pclKeyHandleOpen(PCL_LDBID_LOCAL, "posHandle/last_position8", 0, 0);
   fail_unless(hd8 == 5, "Failed to open handle ==> /posHandle/last_position8");

   hd9 = pclKeyHandleOpen(PCL_LDBID_LOCAL, "posHandle/last_position9", 0, 0);
   fail_unless(hd9 == 6, "Failed to open handle ==> /posHandle/last_position9");

   // close handles ---------------------------------------------------
   ret = pclKeyHandleClose(hd4);
   fail_unless(ret != -1, "Failed to close handle!!");

   ret = pclKeyHandleClose(hd5);
   fail_unless(ret != -1, "Failed to close handle!!");

   ret = pclKeyHandleClose(hd6);
   fail_unless(ret != -1, "Failed to close handle!!");

   ret = pclKeyHandleClose(hd7);
   fail_unless(ret != -1, "Failed to close handle!!");

   ret = pclKeyHandleClose(hd8);
   fail_unless(ret != -1, "Failed to close handle!!");

   ret = pclKeyHandleClose(hd9);
   fail_unless(ret != -1, "Failed to close handle!!");
}
END_TEST



START_TEST(test_Plugin)
{
   int ret = 0;
   unsigned char buffer[READ_SIZE]  = {0};

   DLT_LOG(gPcltDLTContext, DLT_LOG_INFO, DLT_STRING("PCL_TEST test_Plugin"));

   ret = pclKeyReadData(PCL_LDBID_LOCAL, "secured",           0, 0, buffer, READ_SIZE);
   //printf("B U F F E R - secure: \"%s\" => ist: %d | soll: %d\n", buffer, ret, strlen("Custom plugin -> plugin_get_data: secure!"));
   fail_unless(ret == strlen("Custom plugin -> plugin_get_data: secure!") );
   fail_unless(strncmp((char*)buffer,"Custom plugin -> plugin_get_data: secure!",
                 strlen((char*)buffer)) == 0, "Buffer SECURE not correctly read");
   memset(buffer, 0, READ_SIZE);

   ret = pclKeyReadData(PCL_LDBID_LOCAL, "early",     0, 0, buffer, READ_SIZE);
   //printf("B U F F E R - early: \"%s\" => ist: %d | soll: %d\n", buffer, ret, strlen("Custom plugin -> plugin_get_data: early!"));
   fail_unless(ret == strlen("Custom plugin -> plugin_get_data: early!"));
   fail_unless(strncmp((char*)buffer,"Custom plugin -> plugin_get_data: early!",
               strlen((char*)buffer)) == 0, "Buffer EARLY not correctly read");
   memset(buffer, 0, READ_SIZE);

   ret = pclKeyReadData(PCL_LDBID_LOCAL, "emergency", 0, 0, buffer, READ_SIZE);
   //printf("B U F F E R - emergency: \"%s\" => ist: %d | soll: %d\n", buffer, ret, strlen("Custom plugin -> plugin_get_data: emergency!"));
   fail_unless(ret == strlen("Custom plugin -> plugin_get_data: emergency!"));
   fail_unless(strncmp((char*)buffer,"Custom plugin -> plugin_get_data: emergency!",
               strlen((char*)buffer)) == 0, "Buffer EMERGENCY not correctly read");
   memset(buffer, 0, READ_SIZE);

   ret = pclKeyReadData(PCL_LDBID_LOCAL, "hwinfo",   0, 0, buffer, READ_SIZE);
   //printf("B U F F E R - hwinfo: \"%s\" => ist: %d | soll: %d\n", buffer, ret, strlen("Custom plugin -> plugin_get_data: hwinfo!"));
   fail_unless(ret != EPERS_NOT_INITIALIZED);
   fail_unless(strncmp((char*)buffer,"Custom plugin -> plugin_get_data: hwinfo!",
               strlen((char*)buffer)) == 0, "Buffer HWINFO not correctly read");
   memset(buffer, 0, READ_SIZE);

   ret = pclKeyReadData(PCL_LDBID_LOCAL, "custom2",   0, 0, buffer, READ_SIZE);
   //printf("B U F F E R - custom2: \"%s\" => ist: %d | soll: %d\n", buffer, ret, strlen("Custom plugin -> plugin_get_data: custom2!"));
   fail_unless(ret == strlen("Custom plugin -> plugin_get_data: custom2!"));
   fail_unless(strncmp((char*)buffer,"Custom plugin -> plugin_get_data: custom2!",
               strlen((char*)buffer)) == 0, "Buffer CUSTOM 2 not correctly read");
   memset(buffer, 0, READ_SIZE);

   ret = pclKeyReadData(PCL_LDBID_LOCAL, "custom3",   0, 0, buffer, READ_SIZE);
   //printf("B U F F E R - custom3: \"%s\" => ist: %d | soll: %d\n", buffer, ret, strlen("Custom plugin -> plugin_get_data: custom3!"));
   fail_unless(ret == strlen("Custom plugin -> plugin_get_data: custom3!"));
   fail_unless(strncmp((char*)buffer,"Custom plugin -> plugin_get_data: custom3!",
                 strlen((char*)buffer)) == 0, "Buffer CUSTOM 3 not correctly read");
   memset(buffer, 0, READ_SIZE);

   ret = pclKeyWriteData(PCL_LDBID_LOCAL, "custom3",   0, 0, (unsigned char*)"This is a message to write", READ_SIZE);
   fail_unless(ret == 321456, "Failed to write custom data");  // plugin should return 321456


   ret = pclKeyGetSize(PCL_LDBID_LOCAL, "custom3",   0, 0);
   fail_unless(ret == 44332211, "Failed query custom data size"); // plugin should return 44332211


   ret = pclKeyDelete(PCL_LDBID_LOCAL, "custom3",   0, 0);
   fail_unless(ret == 13579, "Failed query custom data size"); // plugin should return 13579
}
END_TEST





START_TEST(test_ReadDefault)
{
   int ret = 0;
   unsigned char buffer[READ_SIZE]  = {0};

   DLT_LOG(gPcltDLTContext, DLT_LOG_INFO, DLT_STRING("PCL_TEST test_ReadDefault"));

   ret = pclKeyReadData(PCL_LDBID_LOCAL, "statusHandle/default01", 3, 2, buffer, READ_SIZE);
   //printf(" --- test_ReadConfDefault => statusHandle/default01: %s => retIst: %d retSoll: %d\n", buffer, ret, strlen("DEFAULT_01!"));
   fail_unless(ret == strlen("DEFAULT_01!"));
   fail_unless(strncmp((char*)buffer,"DEFAULT_01!", strlen((char*)buffer)) == 0, "Buffer not correctly read");

   ret = pclKeyReadData(PCL_LDBID_LOCAL, "statusHandle/default02", 3, 2, buffer, READ_SIZE);
   //printf(" --- test_ReadConfDefault => statusHandle/default02: %s => retIst: %d retSoll: %d\n", buffer, ret, strlen("DEFAULT_02!"));
   fail_unless(ret == strlen("DEFAULT_02!"));
   fail_unless(strncmp((char*)buffer,"DEFAULT_02!", strlen((char*)buffer)) == 0, "Buffer not correctly read");

   ret = pclKeyGetSize(PCL_LDBID_LOCAL, "statusHandle/default02", 3, 2);
   //printf("IST: %d - SOLL: %d\n", ret, (int)strlen("DEFAULT_02!"));
   fail_unless(ret == strlen("DEFAULT_01!"), "Invalid size");

   ret = pclKeyGetSize(PCL_LDBID_LOCAL, "statusHandle/default01", 3, 2);
   //printf("IST: %d - SOLL: %d\n", ret, (int)strlen("DEFAULT_01!"));
   fail_unless(ret == strlen("DEFAULT_01!"), "Invalid size");
}
END_TEST



START_TEST(test_ReadConfDefault)
{
   int ret = 0;
   unsigned char buffer[READ_SIZE]  = {0};

   DLT_LOG(gPcltDLTContext, DLT_LOG_INFO, DLT_STRING("PCL_TEST test_ReadConfDefault"));

#if 1
   ret = pclKeyReadData(PCL_LDBID_LOCAL, "statusHandle/confdefault01",     3, 2, buffer, READ_SIZE);
   //printf(" --- test_ReadConfDefault => statusHandle/confdefault01: %s => retIst: %d retSoll: %d\n", buffer, ret, strlen("CONF_DEFAULT_01!"));
   fail_unless(ret == strlen("CONF_DEFAULT_01!"));
   fail_unless(strncmp((char*)buffer,"CONF_DEFAULT_01!", strlen((char*)buffer)) == 0, "Buffer not correctly read");

   ret = pclKeyReadData(PCL_LDBID_LOCAL, "statusHandle/confdefault02",     3, 2, buffer, READ_SIZE);
   //printf(" --- test_ReadConfDefault => statusHandle/confdefault02: %s => retIst: %d retSoll: %d\n", buffer, ret, strlen("CONF_DEFAULT_02!"));
   fail_unless(ret == strlen("CONF_DEFAULT_02!"));
   fail_unless(strncmp((char*)buffer,"CONF_DEFAULT_02!", strlen((char*)buffer)) == 0, "Buffer not correctly read");

   ret = pclKeyGetSize(PCL_LDBID_LOCAL, "statusHandle/confdefault02", 3, 2);
   fail_unless(ret == strlen("CONF_DEFAULT_02!"), "Invalid size");

#endif
}
END_TEST



START_TEST(test_WriteConfDefault)
{
   int ret = 0;
   unsigned char writeBuffer[]  = "This is a test string";
   unsigned char writeBuffer2[]  = "And this is a test string which is different form previous test string";
   unsigned char readBuffer[READ_SIZE]  = {0};

   DLT_LOG(gPcltDLTContext, DLT_LOG_INFO, DLT_STRING("PCL_TEST test_WriteConfDefault"));

   // -- key-value interface ---
   ret = pclKeyWriteData(PCL_LDBID_LOCAL, "statusHandle/writeconfdefault01", PCL_USER_DEFAULTDATA, 0, writeBuffer, (int)strlen((char*)writeBuffer));
   fail_unless(ret == (int)strlen((char*)writeBuffer), "Write Conf default data: write size does not match");
   ret = pclKeyReadData(PCL_LDBID_LOCAL, "statusHandle/writeconfdefault01",  3, 2, readBuffer, READ_SIZE);
   fail_unless(ret == (int)strlen((char*)writeBuffer), "Write Conf default data: read size does not match");
   fail_unless(strncmp((char*)readBuffer, (char*)writeBuffer, strlen((char*)readBuffer)) == 0, "Buffer not correctly read");
   //printf(" --- test_ReadConfDefault => statusHandle/writeconfdefault01: \"%s\" => \"%s\" \n    retIst: %d retSoll: %d\n", readBuffer, writeBuffer, ret, strlen((char*)writeBuffer));


   ret = pclKeyWriteData(PCL_LDBID_LOCAL, "statusHandle/writeconfdefault01", PCL_USER_DEFAULTDATA, 0, writeBuffer2, (int)strlen((char*)writeBuffer2));
   fail_unless(ret == (int)strlen((char*)writeBuffer2), "Write Conf default data 2: write size does not match");
   ret = pclKeyReadData(PCL_LDBID_LOCAL, "statusHandle/writeconfdefault01",  3, 2, readBuffer, READ_SIZE);
   fail_unless(strncmp((char*)readBuffer, (char*)writeBuffer2, strlen((char*)readBuffer)) == 0, "Buffer2 not correctly read");
   //printf(" --- test_ReadConfDefault => statusHandle/writeconfdefault01: \"%s\" => \"%s\" \n    retIst: %d retSoll: %d\n", readBuffer, writeBuffer2, ret, strlen((char*)writeBuffer2));

}
END_TEST




START_TEST(test_InitDeinit)
{
   int shutdownReg = PCL_SHUTDOWN_TYPE_FAST | PCL_SHUTDOWN_TYPE_NORMAL;

   int i = 0, rval = -1, handle = 0;


   DLT_LOG(gPcltDLTContext, DLT_LOG_INFO, DLT_STRING("PCL_TEST test_InitDeinit"));


   for(i=0; i<5; i++)
   {
      // initialize and deinitialize 1. time
      (void)pclInitLibrary(gTheAppId, shutdownReg);
      pclDeinitLibrary();


      // initialize and deinitialize 2. time
      (void)pclInitLibrary(gTheAppId, shutdownReg);
      pclDeinitLibrary();


      // initialize and deinitialize 3. time
      (void)pclInitLibrary(gTheAppId, shutdownReg);
      pclDeinitLibrary();
   }


   // test multiple init/deinit
   pclInitLibrary(gTheAppId, shutdownReg);
   pclInitLibrary(gTheAppId, shutdownReg);

   pclDeinitLibrary();
   pclDeinitLibrary();
   pclDeinitLibrary();

   // test lifecycle set
   pclInitLibrary(gTheAppId, shutdownReg);
   rval = pclLifecycleSet(PCL_SHUTDOWN);
   fail_unless(rval == EPERS_SHUTDOWN_NO_PERMIT, "Lifecycle set allowed, but should not");
   pclDeinitLibrary();


   pclInitLibrary(gTheAppId, PCL_SHUTDOWN_TYPE_NONE);

   handle = pclKeyHandleOpen(PCL_LDBID_LOCAL, "posHandle/last_position", 0, 0);
   //printf("pclKeyHandleOpen: %d\n", handle);
   fail_unless(handle >= 0, "Failed to open handle ==> /posHandle/last_position");
   (void)pclKeyHandleClose(handle);

   rval = pclLifecycleSet(PCL_SHUTDOWN);
   fail_unless(rval != EPERS_SHUTDOWN_NO_PERMIT, "Lifecycle set NOT allowed, but should");


   rval = pclLifecycleSet(PCL_SHUTDOWN_CANCEL);
   rval = pclLifecycleSet(PCL_SHUTDOWN_CANCEL);
   rval = pclLifecycleSet(PCL_SHUTDOWN_CANCEL);
   rval = pclLifecycleSet(PCL_SHUTDOWN_CANCEL);
   rval = pclLifecycleSet(PCL_SHUTDOWN_CANCEL);
   rval = pclLifecycleSet(PCL_SHUTDOWN_CANCEL);

   pclDeinitLibrary();

   pclInitLibrary("NodeStateManager", PCL_SHUTDOWN_TYPE_NONE);

   pclDeinitLibrary();


}
END_TEST



START_TEST(test_NegHandle)
{
   int handle = -1, ret = 0;
   int negativeHandle = -17;
   unsigned char buffer[128] = {0};

   DLT_LOG(gPcltDLTContext, DLT_LOG_INFO, DLT_STRING("PCL_TEST test_NegHandle"));

   handle = pclKeyHandleOpen(PCL_LDBID_LOCAL, "posHandle/last_position", 0, 0);
   fail_unless(handle >= 0, "Failed to open handle ==> /posHandle/last_position");

   ret = pclKeyHandleReadData(negativeHandle, buffer, READ_SIZE);
   fail_unless(ret == EPERS_MAXHANDLE, "pclKeyHandleReadData => negative handle not detected");

   ret = pclKeyHandleClose(negativeHandle);
   fail_unless(ret == EPERS_MAXHANDLE, "pclKeyHandleClose => negative handle not detected");

   ret = pclKeyHandleGetSize(negativeHandle);
   fail_unless(ret == EPERS_MAXHANDLE, "pclKeyHandleGetSize => negative handle not detected");

   ret = pclKeyHandleReadData(negativeHandle, buffer, 128);
   fail_unless(ret == EPERS_MAXHANDLE, "pclKeyHandleReadData => negative handle not detected");

   ret = pclKeyHandleRegisterNotifyOnChange(negativeHandle, &myChangeCallback);
   fail_unless(ret == EPERS_MAXHANDLE, "pclKeyHandleRegisterNotifyOnChange => negative handle not detected");

   ret = pclKeyHandleUnRegisterNotifyOnChange(negativeHandle, &myChangeCallback);
   fail_unless(ret == EPERS_MAXHANDLE, "pclKeyHandleUnRegisterNotifyOnChange => negative handle not detected");

   ret = pclKeyHandleWriteData(negativeHandle, (unsigned char*)"Whatever", strlen("Whatever"));
   fail_unless(ret == EPERS_MAXHANDLE, "pclKeyHandleWriteData => negative handle not detected");


   // close handle
   ret = pclKeyHandleClose(handle);
}
END_TEST



START_TEST(test_utf8_string)
{
   int ret = 0, size = 0;
   const char* utf8StringBuffer = "String °^° Ñ text";
   unsigned char buffer[128] = {0};

   DLT_LOG(gPcltDLTContext, DLT_LOG_INFO, DLT_STRING("PCL_TEST test_utf8_string"));

   ret = pclKeyReadData(PCL_LDBID_LOCAL, "utf8String", 3, 2, buffer, READ_SIZE);
   fail_unless(ret == (int)strlen(utf8StringBuffer), "Wrong read size");
   fail_unless(strncmp((char*)buffer, utf8StringBuffer, (size_t)ret-1) == 0, "Buffer not correctly read => 1");

   size = pclKeyGetSize(PCL_LDBID_LOCAL, "utf8String", 3, 2);
   fail_unless(size == (int)strlen(utf8StringBuffer), "Invalid size");
}
END_TEST



START_TEST(test_Notifications)
{
   int ret = 0;

   DLT_LOG(gPcltDLTContext, DLT_LOG_INFO, DLT_STRING("PCL_TEST test_Notifications"));

   ret = pclKeyRegisterNotifyOnChange(0x20, "address/home_address", 1, 1, myChangeCallback);
   fail_unless(ret == 0, "Failed to register");

   ret = pclKeyUnRegisterNotifyOnChange(0x20, "address/home_address", 1, 1, myChangeCallback);
   fail_unless(ret == 0, "Failed to register");

   ret = pclKeyUnRegisterNotifyOnChange(PCL_LDBID_PUBLIC, "aSharedResource", 1, 1, myChangeCallback);
   fail_unless(ret == 0, "Failed to register");

   ret = pclKeyUnRegisterNotifyOnChange(PCL_LDBID_LOCAL, "status/open_document", 1, 1, myChangeCallback);
   fail_unless(ret == EPERS_NOTIFY_NOT_ALLOWED, "Possible to register, but should not - is local variable");

   ret = pclKeyUnRegisterNotifyOnChange(0x20, "notInRCT", 1, 1, myChangeCallback);
   fail_unless(ret == EPERS_NOKEYDATA, "Possible to register, but should not - not in rct");
}
END_TEST


#if USE_APPCHECK
START_TEST(test_ValidApplication)
{
   int ret = 0;
   int shutdownReg = PCL_SHUTDOWN_TYPE_FAST | PCL_SHUTDOWN_TYPE_NORMAL;
   unsigned char buffer[128] = {0};

   DLT_LOG(gPcltDLTContext, DLT_LOG_INFO, DLT_STRING("PCL_TEST test_ValidApplication"));

   ret = pclInitLibrary("InvalidAppID", shutdownReg);

   ret = pclKeyGetSize(PCL_LDBID_LOCAL, "JustTesting", 1, 1);
   fail_unless(ret == EPERS_SHUTDOWN_NO_TRUSTED, "pclKeyGetSize => invalid application ID not detected");

   ret =  pclKeyDelete(PCL_LDBID_LOCAL, "JustTesting", 1, 1);
   fail_unless(ret == EPERS_SHUTDOWN_NO_TRUSTED, "pclKeyDelete => invalid application ID not detected");

   ret =  pclKeyHandleClose(1);
   fail_unless(ret == EPERS_SHUTDOWN_NO_TRUSTED, "pclKeyHandleClose => invalid application ID not detected");

   ret =  pclKeyHandleGetSize(1);
   fail_unless(ret == EPERS_SHUTDOWN_NO_TRUSTED, "pclKeyHandleGetSize => invalid application ID not detected");

   ret =  pclKeyHandleOpen(PCL_LDBID_LOCAL, "JustTesting", 1, 1);
   fail_unless(ret == EPERS_SHUTDOWN_NO_TRUSTED, "pclKeyHandleOpen => invalid application ID not detected");

   ret =  pclKeyHandleReadData(1, buffer, 128);
   fail_unless(ret == EPERS_SHUTDOWN_NO_TRUSTED, "pclKeyHandleReadData => invalid application ID not detected");

   ret =  pclKeyHandleWriteData(1, (unsigned char*)"Test", strlen("Test"));
   fail_unless(ret == EPERS_SHUTDOWN_NO_TRUSTED, "pclKeyHandleWriteData => invalid application ID not detected");

   ret =  pclKeyReadData(PCL_LDBID_LOCAL, "JustTesting", 1, 1, buffer, 128);
   fail_unless(ret == EPERS_SHUTDOWN_NO_TRUSTED, "pclKeyReadData => invalid application ID not detected");

   ret =  pclKeyWriteData(PCL_LDBID_LOCAL, "JustTesting", 1, 1, (unsigned char*)"Test", strlen("Test"));
   fail_unless(ret == EPERS_SHUTDOWN_NO_TRUSTED, "pclKeyWriteData => invalid application ID not detected");


   pclDeinitLibrary();
}
END_TEST
#endif


START_TEST(test_PAS_DbusInterface)
{
   // let the administration servis generate a message to the PCL
   if(system("/usr/local/bin/persadmin_tool export /tmp/myBackup 0") == -1)
   {
      printf("Failed to execute command -> admin service!!\n");
   }
}
END_TEST



START_TEST(test_LC_DbusInterface)
{

// send the following dbus command
//
   printf("\n\n*******************************************************\n");
   printf("Past and execute NOW the following command to a console: \"dbus-send --system --print-reply --dest=org.genivi.NodeStateManager /org/genivi/NodeStateManager/LifecycleControl org.genivi.NodeStateManager.LifecycleControl.SetNodeState int32:6\"\n");
   printf("*******************************************************\n\n");

#if 0
#if 0
   const char* theDbusCommand =
   "dbus-send --system --print-reply \
   --dest=org.genivi.NodeStateManager \
   /org/genivi/NodeStateManager/LifecycleControl \
   \"org.genivi.NodeStateManager.LifecycleControl.SetNodeState\" \
   int32:6";

   // notify the NSM to shutdown the system
   if(system(theDbusCommand) == -1)
   {
      printf("Failed to execute command -> NSM!!\n");
   }
#else
   int nodeState = 6;   // shutdown state
   DBusConnection* conn = NULL;
   DBusError err;

   dbus_error_init(&err);
   conn = dbus_bus_get_private(DBUS_BUS_SYSTEM, &err);

   DBusMessage* message = dbus_message_new_method_call("org.genivi.NodeStateManager",        // destination
                                                       "/org/genivi/NodeStateManager/LifecycleControl",           // path
                                                       "org.genivi.NodeStateManager.LifecycleControl",        // interface
                                                       "SetNodeState");                      // method

   dbus_message_append_args(message, DBUS_TYPE_INT32, &nodeState, DBUS_TYPE_INVALID);

   printf("*************************** ==> Send message and block\n");
   if(!dbus_connection_send_with_reply_and_block(conn, message, 5000, &err))
   {
         printf("connection send: - Access denied: %s\n", err.message);
   }
   dbus_connection_flush(conn);
   dbus_message_unref(message);
   printf("*************************** <== \n");

   dbus_connection_close(conn);
   dbus_connection_unref(conn);
#endif
#else

   sleep(6);

#endif
}
END_TEST


START_TEST(test_SharedAccess)
{
   int ret = 0;
   int shutdownReg = PCL_SHUTDOWN_TYPE_FAST | PCL_SHUTDOWN_TYPE_NORMAL;
   unsigned char buffer[256] = {0};
   char sysTimeBuffer[256];
   struct tm *locTime;
   time_t t = time(0);

   DLT_LOG(gPcltDLTContext, DLT_LOG_INFO, DLT_STRING("PCL_TEST test_SharedAccess"));

   locTime = localtime(&t);

   // write data
   snprintf(sysTimeBuffer, 128, "\"%s %d.%d.%d - %d:%.2d:%.2d Uhr\"", dayOfWeek[locTime->tm_wday], locTime->tm_mday, locTime->tm_mon+1, (locTime->tm_year+1900),
                                                                      locTime->tm_hour, locTime->tm_min, locTime->tm_sec);

   (void)pclInitLibrary(gTheAppId, shutdownReg);   // use the app id, the resource is registered for

   ret = pclKeyWriteData(PCL_LDBID_PUBLIC, "aSharedResource", 1, 1, (unsigned char*)sysTimeBuffer, (int)strlen(sysTimeBuffer));
   fail_unless(ret == (int)strlen(sysTimeBuffer), "Failed to write shared data ");

   ret = pclKeyReadData(PCL_LDBID_PUBLIC, "aSharedResource", 1, 1, buffer, 256);
   fail_unless(ret == (int)strlen(sysTimeBuffer), "Failed to read shared data ");

   pclDeinitLibrary();

   // ----------------------------------------------

   (void)pclInitLibrary("node-health-monitor", shutdownReg);   // now use a different app id, which is not able to write this resource

   ret = pclKeyWriteData(PCL_LDBID_PUBLIC, "aSharedResource", 1, 1, (unsigned char*)"This is a test Buffer", (int)strlen("This is a test Buffer"));
   fail_unless(ret == EPERS_NOT_RESP_APP, "Able to write shared data, but should not!!");

   ret = pclKeyReadData(PCL_LDBID_PUBLIC, "aSharedResource", 1, 1, buffer, 256);
   fail_unless(ret == (int)strlen(sysTimeBuffer), "Failed to read shared data ");
   fail_unless(strncmp((char*)buffer, sysTimeBuffer, strlen((char*)sysTimeBuffer)) == 0, "Buffer not correctly read");

   pclDeinitLibrary();
}
END_TEST



START_TEST(test_VO722)
{
   int ret = 0;
   int shutdownReg = PCL_SHUTDOWN_TYPE_FAST | PCL_SHUTDOWN_TYPE_NORMAL;

   unsigned char buffer[256] = {0};
   char* writeBuffer[] = {"VO722 - TestString One",
                          "VO722 - TestString Two -",
                          "VO722 - TestString Three -",};

   char* writeBuffer2[] = {"2 - VO722 - Test - String One",
                           "2 - VO722 - Test - String Two -",
                           "2 - VO722 - Test - String Three -", };

   DLT_LOG(gPcltDLTContext, DLT_LOG_INFO, DLT_STRING("PCL_TEST test_VO722"));

   (void)pclInitLibrary(gTheAppId, shutdownReg);   // use the app id, the resource is registered for

   ret = pclKeyWriteData(PCL_LDBID_LOCAL, "ContactListViewSortOrder", 1, 2, (unsigned char*)writeBuffer[0], (int)strlen(writeBuffer[0]));
   fail_unless(ret == (int)strlen(writeBuffer[0]), "Wrong write size");

   memset(buffer, 0, 256);
   ret = pclKeyReadData(PCL_LDBID_LOCAL, "ContactListViewSortOrder", 1, 2, (unsigned char*)buffer, 256);
   //printf("****** 1.1. read AEVOO722 => buffer: \"%s\"\n", buffer);
   fail_unless(ret == (int)strlen(writeBuffer[0]), "Failed to read shared data ");
   fail_unless(strncmp((char*)buffer, writeBuffer[0], strlen((char*)writeBuffer[0])) == 0, "Buffer not correctly read - 1.1");

   ret = pclKeyWriteData(PCL_LDBID_LOCAL, "ContactListViewSortOrder", 1, 2, (unsigned char*)writeBuffer[1], (int)strlen(writeBuffer[1]));
   fail_unless(ret == (int)strlen(writeBuffer[1]), "Wrong write size");

   memset(buffer, 0, 256);
   ret = pclKeyReadData(PCL_LDBID_LOCAL, "ContactListViewSortOrder", 1, 2, (unsigned char*)buffer, 256);
   //printf("****** 1.2. read AEVOO722 => buffer: \"%s\"\n", buffer);
   fail_unless(ret == (int)strlen(writeBuffer[1]), "Failed to read shared data ");
   fail_unless(strncmp((char*)buffer, writeBuffer[1], strlen((char*)writeBuffer[1])) == 0, "Buffer not correctly read - 1.2");

   ret = pclKeyWriteData(PCL_LDBID_LOCAL, "ContactListViewSortOrder", 1, 2, (unsigned char*)writeBuffer[2], (int)strlen(writeBuffer[2]));
   fail_unless(ret == (int)strlen(writeBuffer[2]), "Wrong write size");

   pclDeinitLibrary();
   sleep(1);


   (void)pclInitLibrary(gTheAppId, shutdownReg);   // use the app id, the resource is registered for

   memset(buffer, 0, 256);
   ret = pclKeyReadData(PCL_LDBID_LOCAL, "ContactListViewSortOrder", 1, 2, (unsigned char*)buffer, 256);
   //printf("****** 1.3. read AEVOO722 => buffer: \"%s\"\n\n", buffer);
   fail_unless(ret == (int)strlen(writeBuffer[2]), "Failed to read shared data ");
   fail_unless(strncmp((char*)buffer, writeBuffer[2], strlen((char*)writeBuffer[2])) == 0, "Buffer not correctly read - 1.3");

   pclDeinitLibrary();

//------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------

   (void)pclInitLibrary(gTheAppId, shutdownReg);   // use the app id, the resource is registered for

   ret = pclKeyWriteData(PCL_LDBID_LOCAL, "ContactListViewSortOrder", 0, 0, (unsigned char*)writeBuffer2[0], (int)strlen(writeBuffer2[0]));
   fail_unless(ret == (int)strlen(writeBuffer2[0]), "Wrong write size");

   memset(buffer, 0, 256);
   ret = pclKeyReadData(PCL_LDBID_LOCAL, "ContactListViewSortOrder", 0, 0, (unsigned char*)buffer, 256);
   //printf("****** 2.1. read AEVOO722 => buffer: \"%s\"\n", buffer);
   fail_unless(ret == (int)strlen(writeBuffer2[0]), "Failed to read shared data ");
   fail_unless(strncmp((char*)buffer, writeBuffer2[0], strlen((char*)writeBuffer2[0])) == 0, "Buffer not correctly read - 2.1");


   ret = pclKeyWriteData(PCL_LDBID_LOCAL, "ContactListViewSortOrder", 0, 0, (unsigned char*)writeBuffer2[1], (int)strlen(writeBuffer2[1]));
   fail_unless(ret == (int)strlen(writeBuffer2[1]), "Wrong write size");

   memset(buffer, 0, 256);
   ret = pclKeyReadData(PCL_LDBID_LOCAL, "ContactListViewSortOrder", 0, 0, (unsigned char*)buffer, 256);
   //printf("****** 2.2. read AEVOO722 => buffer: \"%s\"\n", buffer);
   fail_unless(ret == (int)strlen(writeBuffer2[1]), "Failed to read shared data ");
   fail_unless(strncmp((char*)buffer, writeBuffer2[1], strlen((char*)writeBuffer2[1])) == 0, "Buffer not correctly read - 2.2");

   ret = pclKeyWriteData(PCL_LDBID_LOCAL, "ContactListViewSortOrder", 0, 0, (unsigned char*)writeBuffer2[2], (int)strlen(writeBuffer2[2]));
   fail_unless(ret == (int)strlen(writeBuffer2[2]), "Wrong write size");

   pclDeinitLibrary();


   (void)pclInitLibrary(gTheAppId, shutdownReg);   // use the app id, the resource is registered for

   memset(buffer, 0, 256);
   ret = pclKeyReadData(PCL_LDBID_LOCAL, "ContactListViewSortOrder", 0, 0, (unsigned char*)buffer, 256);
   //printf("****** 2.3. read AEVOO722 => buffer: \"%s\"\n\n", buffer);
   fail_unless(ret == (int)strlen(writeBuffer2[2]), "Failed to read shared data ");
   fail_unless(strncmp((char*)buffer, writeBuffer2[2], strlen((char*)writeBuffer2[2])) == 0, "Buffer not correctly read - 2.3");

   pclDeinitLibrary();
}
END_TEST



START_TEST(test_NoRct)
{
   int ret = 0;
   const char writeBuffer[] = "This is a test string";

   DLT_LOG(gPcltDLTContext, DLT_LOG_INFO, DLT_STRING("PCL_TEST test_NoRct"));

   ret = pclKeyWriteData(PCL_LDBID_LOCAL, "someResourceId", 0, 0, (unsigned char*)writeBuffer, (int)strlen(writeBuffer));

#if USE_APPCHECK
   fail_unless(ret == EPERS_SHUTDOWN_NO_TRUSTED, "Shutdown is trusted, but should not");
#else
   fail_unless(ret == EPERS_NOPRCTABLE, "RCT available, but should not");
#endif

}
END_TEST



START_TEST(test_InvalidPluginfConf)
{
   int shutdownReg = PCL_SHUTDOWN_TYPE_FAST | PCL_SHUTDOWN_TYPE_NORMAL;
   const char* envVariable = "PERS_CLIENT_LIB_CUSTOM_LOAD";

   DLT_LOG(gPcltDLTContext, DLT_LOG_INFO, DLT_STRING("PCL_TEST test_InvalidPluginfConf"));

   // change to an invalid plugin configuration file using environment variable
   setenv(envVariable, "/tmp/whatever/pclCustomLibConfigFile.cfg", 1);

   (void)pclInitLibrary(gTheAppId, shutdownReg);   // use the app id, the resource is registered for

   pclDeinitLibrary();


   // change to an empty plugin configuration file using environment variable
   setenv(envVariable, "/etc/pclCustomLibConfigFileEmpty.cfg", 1);

   (void)pclInitLibrary(gTheAppId, shutdownReg);   // use the app id, the resource is registered for

   pclDeinitLibrary();

   (void)unsetenv(envVariable);
}
END_TEST


START_TEST(test_SharedData)
{
   int ret = 0;
   DLT_LOG(gPcltDLTContext, DLT_LOG_INFO, DLT_STRING("PCL_TEST test_SharedData"));

   ret = pclKeyWriteData(0x20, "links/last_link2",  2, 1, (unsigned char*)"Test notify shared data___1111", strlen("Test notify shared data___1111"));
   fail_unless(ret == (int)strlen("Test notify shared data___1111"), "Wrong write size");

   sleep(1);

   ret = pclKeyWriteData(0x20, "links/last_link2",  2, 1, (unsigned char*)"Test notify shared data___2211", strlen("Test notify shared data___2211"));
   fail_unless(ret == (int)strlen("Test notify shared data___2211"), "Wrong write size");

   sleep(1);

   ret = pclKeyWriteData(0x20, "links/last_link2",  2, 1, (unsigned char*)"Test notify shared data___3311", strlen("Test notify shared data___3311"));
   fail_unless(ret == (int)strlen("Test notify shared data___3311"), "Wrong write size");

   sleep(1);

   ret = pclKeyWriteData(0x20, "links/last_link2",  2, 1, (unsigned char*)"Test notify shared data___4411", strlen("Test notify shared data___4411"));
   fail_unless(ret == (int)strlen("Test notify shared data___4411"), "Wrong write size");

   sleep(1);

   ret = pclKeyWriteData(0x20, "links/last_link2",  2, 1, (unsigned char*)"Test notify shared data___5511", strlen("Test notify shared data___5511"));
   fail_unless(ret == (int)strlen("Test notify shared data___5511"), "Wrong write size");
}
END_TEST



void* readThread(void* userData)
{
   int ret = 0, i = 0, handleOne = 0, handleTwo = 0, handleThree = 0;
   unsigned char buffer[READ_SIZE] = {0};
   char threadName[64] = {0};
   char* uData = NULL;
   uData = (char*)userData;

   int shutdownReg = PCL_SHUTDOWN_TYPE_FAST | PCL_SHUTDOWN_TYPE_NORMAL;
   const char* envVariable = "PERS_CLIENT_LIB_CUSTOM_LOAD";

   memset(threadName, 0, 64-1);
   memcpy(threadName, uData, 64-1);
   threadName[64-1] = '\0';

   setenv(envVariable, "/etc/pclCustomLibConfigFileTest.cfg", 1);

   (void)pclInitLibrary(gTheAppId, shutdownReg);


   pthread_barrier_wait(&gBarrierOne);
   usleep(5000);

   handleOne   = pclKeyHandleOpen(PCL_LDBID_LOCAL, "pos/last_satellites", 1, 2);
   handleTwo   = pclKeyHandleOpen(PCL_LDBID_LOCAL, "pos/last_satellites", 2, 3);
   handleThree = pclKeyHandleOpen(PCL_LDBID_LOCAL, "pos/last_satellites", 3, 4);

   ret = pclKeyHandleWriteData(handleOne, (unsigned char*)"pos/last_satellites_1_2_data", (int)strlen("pos/last_satellites_1_2_data"));
   fail_unless(ret == (int)strlen("pos/last_satellites_1_2_data"));

   ret = pclKeyHandleWriteData(handleTwo, (unsigned char*)"pos/last_satellites_2_3_data_23", (int)strlen("pos/last_satellites_2_3_data_23"));
   fail_unless(ret == (int)strlen("pos/last_satellites_2_3_data_23"));

   ret = pclKeyHandleWriteData(handleThree, (unsigned char*)"pos/last_satellites_3_4_data_34_34", (int)strlen("pos/last_satellites_3_4_data_34_34"));
   fail_unless(ret == (int)strlen("pos/last_satellites_3_4_data_34_34"));

   for(i=0; i<NUM_OF_READS; i++)
   {
      /**
       * Logical DB ID: PCL_LDBID_LOCAL with user 0 and seat 0
       *       ==> local value accessible by all users (user 0, seat 0)
       */
      memset(buffer, 0, READ_SIZE);
      ret = pclKeyReadData(PCL_LDBID_LOCAL, "pos/last_position",         1, 1, buffer, READ_SIZE);
      fail_unless(strncmp((char*)buffer, "CACHE_ +48 10' 38.95, +8 44' 39.06",
                    strlen((char*)buffer)) == 0, "Buffer not correctly read - pos/last_position");
      fail_unless(ret == strlen("CACHE_ +48 10' 38.95, +8 44' 39.06"));
      usleep(3000);

      memset(buffer, 0, READ_SIZE);
      ret = pclKeyHandleReadData(handleOne, buffer, READ_SIZE);
      fail_unless(strncmp((char*)buffer, "pos/last_satellites_1_2_data",
                    strlen((char*)buffer)) == 0, "Buffer not correctly read - pos/last_satellites_1_2_data");
      fail_unless(ret == strlen("pos/last_satellites_1_2_data"));
      usleep(3000);

      /**
       * Logical DB ID: PCL_LDBID_LOCAL with user 3 and seat 2
       *       ==> local USER value (user 3, seat 2)
       */
      memset(buffer, 0, READ_SIZE);
      ret = pclKeyReadData(PCL_LDBID_LOCAL, "status/open_document",      3, 2, buffer, READ_SIZE);
      fail_unless(strncmp((char*)buffer, "WT_ /var/opt/user_manual_climateControl.pdf", strlen((char*)buffer)) == 0,
                    "Buffer not correctly read - status/open_document");
      fail_unless(ret == strlen("WT_ /var/opt/user_manual_climateControl.pdf"));
      usleep(2000);

#if 1
      /**
       * Logical DB ID: 0x20 with user 4 and seat 0
       *       ==> shared user value accessible by a group (user 4 and seat 0)
       */
      memset(buffer, 0, READ_SIZE);
      ret = pclKeyReadData(0x20, "address/home_address",      4, 0, buffer, READ_SIZE);
      fail_unless(strncmp((char*)buffer, "WT_ 55327 Heimatstadt, Wohnstrasse 31", strlen((char*)buffer)) == 0,
                    "Buffer not correctly read - address/home_address");
      fail_unless(ret == strlen("WT_ 55327 Heimatstadt, Wohnstrasse 31"));
      usleep(5000);
#endif

      memset(buffer, 0, READ_SIZE);
      ret = pclKeyHandleReadData(handleTwo, buffer, READ_SIZE);
      fail_unless(strncmp((char*)buffer, "pos/last_satellites_2_3_data_23",
                    strlen((char*)buffer)) == 0, "Buffer not correctly read - pos/last_satellites_2_3_data_23");
      fail_unless(ret == strlen("pos/last_satellites_2_3_data_23"));
      usleep(3000);


      /**
       * Logical DB ID: PCL_LDBID_LOCAL with user 0 and seat 0
       *       ==> local value accessible by ALL USERS (user 0, seat 0)
       */
      memset(buffer, 0, READ_SIZE);
      ret = pclKeyReadData(PCL_LDBID_LOCAL, "pos/last_satellites",       0, 0, buffer, READ_SIZE);

      fail_unless(strncmp((char*)buffer, "WT_ 17", strlen((char*)buffer)) == 0,
                    "Buffer not correctly read - pos/last_satellites");
      fail_unless(ret == strlen("WT_ 17"));
      usleep(2000);

#if 1
      /**
       * Logical DB ID: 0x20 with user 4 and seat 0
       *       ==> shared user value accessible by A GROUP (user 4 and seat 0)
       */
      memset(buffer, 0, READ_SIZE);
      ret = pclKeyReadData(0x20, "links/last_link",           2, 0, buffer, READ_SIZE);
      fail_unless(strncmp((char*)buffer, "CACHE_ /last_exit/queens", strlen((char*)buffer)) == 0,
                    "Buffer not correctly read - links/last_link");
      fail_unless(ret == strlen("CACHE_ /last_exit/queens"));
      usleep(3000);
#endif

      memset(buffer, 0, READ_SIZE);
      ret = pclKeyHandleReadData(handleThree, buffer, READ_SIZE);
      fail_unless(strncmp((char*)buffer, "pos/last_satellites_3_4_data_34_34",
                    strlen((char*)buffer)) == 0, "Buffer not correctly read - pos/last_satellites_3_4_data_34_34");
      fail_unless(ret == strlen("pos/last_satellites_3_4_data_34_34"));
      usleep(3000);
   }

   (void)pclKeyHandleClose(handleOne);
   (void)pclKeyHandleClose(handleTwo);
   (void)pclKeyHandleClose(handleThree);


   pclDeinitLibrary();

   pthread_exit(0);
}



START_TEST(test_MultiThreadedRead)
{
   pthread_t gReadthreads[NUM_THREADS];
   int i=0;
   char threadName[NUM_THREADS][NAME_LEN];

   DLT_LOG(gPcltDLTContext, DLT_LOG_INFO, DLT_STRING("PCL_TEST test_MultiThreadedRead"));

   if(pthread_barrier_init(&gBarrierOne, NULL, NUM_THREADS) == 0)
   {
      for(i=0; i<NUM_THREADS; i++)
      {
         memset(threadName[i], 0, NAME_LEN);
         sprintf(threadName[i], "R-Thread -%3d-", i);
         threadName[i][NAME_LEN-1] = '\0';

         if(pthread_create(&gReadthreads[i], NULL, readThread, threadName[i]) != -1)
         {
            (void)pthread_setname_np(gReadthreads[i], threadName[i]);
         }
      }

      for(i=0; i<NUM_THREADS; i++)
      {
         if(pthread_join(gReadthreads[i], NULL) != 0)    // wait until thread has ended
            printf("pthread_join - FAILED [%d]\n", i);
      }

      if(pthread_barrier_destroy(&gBarrierOne) != 0)
         printf("Failed to destroy barrier\n");
   }
   else
   {
      printf("Failed to init barrier\n");
   }
}
END_TEST



void* writeThread(void* userData)
{
   int ret = 0, i = 0;
   unsigned char buffer[READ_SIZE] = {0};
   char sysTimeBuffer[128];
   char* staticString = "A quick movement of the enemy will jeopardize six gunboats";
   char payload[NAME_LEN] = {0};
   struct tm *locTime;
   struct timespec curTime;
   t_threadData* threadData = (t_threadData*)userData;

   memset(payload, 0, NAME_LEN);
   strncpy(payload, threadData->threadName, NAME_LEN);
   payload[NAME_LEN-1] = '\0'; // string end termination

   pthread_barrier_wait(&gBarrierTwo);
   usleep(5000);

   for(i=0; i<NUM_OF_WRITES; i++)
   {
      time_t t = time(0);
      locTime = localtime(&t);
      clock_gettime(CLOCK_MONOTONIC, &curTime);
      memset(sysTimeBuffer, 0, 128);
      snprintf(sysTimeBuffer, 128, "\"%s %d.%d.%d - %d:%.2d:%.2d::%.4d:%.8ld Uhr\"", dayOfWeek[locTime->tm_wday], (int)locTime->tm_mday, (int)locTime->tm_mon+1, (int)(locTime->tm_year+1900),
                                                                        (int)locTime->tm_hour, (int)locTime->tm_min, (int)locTime->tm_sec, (int)((int)curTime.tv_nsec / 1.0e6), curTime.tv_nsec );

      ret = pclKeyWriteData((unsigned int)PCL_LDBID_LOCAL, "69", (unsigned int)threadData->index, 2, (unsigned char*)payload, (int)strlen(payload));
      fail_unless(ret == (int)strlen(payload), "Wrong write size");
      usleep(2000);

      ret = pclKeyWriteData(PCL_LDBID_LOCAL, payload, 1, (unsigned int)threadData->index, (unsigned char*)sysTimeBuffer, (int)strlen(sysTimeBuffer));
      fail_unless(ret == (int)strlen(sysTimeBuffer), "Wrong write size");
      usleep(3000);

      ret = pclKeyWriteData(PCL_LDBID_LOCAL, "70", 1, 2, (unsigned char*)staticString, (int)strlen(staticString));
      fail_unless(ret == (int)strlen(staticString), "Wrong write size");
      usleep(3000);

      memset(buffer, 0, READ_SIZE);
      ret = pclKeyReadData(PCL_LDBID_LOCAL, "69",      (unsigned int)threadData->index, 2, buffer, READ_SIZE);
      fail_unless(strncmp((char*)buffer, payload, strlen(payload)) == 0, "2: Buffer not correctly read");
      fail_unless(ret == (int)strlen(payload), "Wrong read size");
      usleep(5000);

      memset(buffer, 0, READ_SIZE);
      ret = pclKeyReadData(PCL_LDBID_LOCAL, payload,  1, (unsigned int)threadData->index, buffer, READ_SIZE);
      fail_unless(strncmp((char*)buffer, sysTimeBuffer, strlen(sysTimeBuffer)) == 0, "1: Buffer not correctly read");
      fail_unless(ret == (int)strlen(sysTimeBuffer), "Wrong read size");
      usleep(2000);

      memset(buffer, 0, READ_SIZE);
      ret = pclKeyReadData(PCL_LDBID_LOCAL, "70",      1, 2, buffer, READ_SIZE);
      fail_unless(strncmp((char*)buffer, staticString, strlen(staticString)) == 0, "3: Buffer not correctly read");
      fail_unless(ret == (int)strlen(staticString), "Wrong read size");
      usleep(3000);
   }

   pthread_exit(0);
}



START_TEST(test_MultiThreadedWrite)
{
   int i=0;
   pthread_t gWritethreads[NUM_THREADS];
   t_threadData threadData[NUM_THREADS];

   DLT_LOG(gPcltDLTContext, DLT_LOG_INFO, DLT_STRING("PCL_TEST test_MultiThreadedWrite"));

   if(pthread_barrier_init(&gBarrierTwo, NULL, NUM_THREADS) == 0)
   {
      for(i=0; i<NUM_THREADS; i++)
      {
         memset(threadData[i].threadName, 0, NAME_LEN);
         sprintf(threadData[i].threadName, "-%3d-W-Key-%3d-", i, i);
         threadData[i].threadName[NAME_LEN-1] = '\0';
         threadData[i].index = i;

         if(pthread_create(&gWritethreads[i], NULL, writeThread, &(threadData[i])) != -1)
         {
            (void)pthread_setname_np(gWritethreads[i], threadData[i].threadName);
         }
      }

      for(i=0; i<NUM_THREADS; i++)
      {
         if(pthread_join(gWritethreads[i], NULL) != 0)    // wait until thread has ended
            printf("pthread_join - FAILED [%d]\n", i);
      }

      if(pthread_barrier_destroy(&gBarrierTwo) != 0)
         printf("Failed to destroy barrier\n");
   }
   else
   {
      printf("Failed to init barrier\n");
   }
}
END_TEST



START_TEST(test_NoPluginFunc)
{
   unsigned char buffer[READ_SIZE] = {0};
   int ret = 0, handle;
   int shutdownReg = PCL_SHUTDOWN_TYPE_FAST | PCL_SHUTDOWN_TYPE_NORMAL;
   const char* envVariable = "PERS_CLIENT_LIB_CUSTOM_LOAD";

   DLT_LOG(gPcltDLTContext, DLT_LOG_INFO, DLT_STRING("PCL_TEST test_NoPluginFunc"));

   // change to an wrong plugin configuration file using environment variable
   setenv(envVariable, "/etc/pclCustomLibConfigFileWrongDefault.cfg", 1);

   sleep(2);

   (void)pclInitLibrary(gTheAppId, shutdownReg);   // use the app id, the resource is registered for

   ret = pclKeyReadData(PCL_LDBID_LOCAL, "status/open_document", 3, 2, buffer, READ_SIZE);
   ck_assert_int_eq(ret, EPERS_COMMON);

   ret = pclKeyGetSize(PCL_LDBID_LOCAL, "status/open_document", 3, 2);
   ck_assert_int_eq(ret, EPERS_COMMON);

   handle = pclKeyHandleOpen(PCL_LDBID_LOCAL, "posHandle/last_position", 0, 0);
   ck_assert_int_gt(handle, 0);

   ret = pclKeyHandleReadData(handle, buffer, READ_SIZE);
   ck_assert_int_eq(ret, EPERS_COMMON);

   ret = pclKeyHandleClose(handle);
   ck_assert_int_eq(ret, 1);

   pclDeinitLibrary();

   (void)unsetenv(envVariable);
}
END_TEST


static Suite * persistenceClientLib_suite()
{
   const char* testSuiteName = "Persistence Client Library (Key-API)";

   Suite * s  = suite_create(testSuiteName);

   TCase * tc_persGetData = tcase_create("GetData");
   tcase_add_test(tc_persGetData, test_GetData);
   tcase_set_timeout(tc_persGetData, 3);

   TCase * tc_persSetData = tcase_create("SetData");
   tcase_add_test(tc_persSetData, test_SetData);
   tcase_set_timeout(tc_persSetData, 3);

   TCase * tc_persSetDataNoPRCT = tcase_create("SetDataNoPRCT");
   tcase_add_test(tc_persSetDataNoPRCT, test_SetDataNoPRCT);
   tcase_set_timeout(tc_persSetDataNoPRCT, 3);

   TCase * tc_persGetDataSize = tcase_create("GetDataSize");
   tcase_add_test(tc_persGetDataSize, test_GetDataSize);
   tcase_set_timeout(tc_persGetDataSize, 3);

   TCase * tc_persDeleteData = tcase_create("DeleteData");
   tcase_add_test(tc_persDeleteData, test_DeleteData);
   tcase_set_timeout(tc_persDeleteData, 3);

   TCase * tc_persGetDataHandle = tcase_create("GetDataHandle");
   tcase_add_test(tc_persGetDataHandle, test_GetDataHandle);
   tcase_set_timeout(tc_persGetDataHandle, 3);

   TCase * tc_persDataHandleOpen = tcase_create("DataHandleOpen");
   tcase_add_test(tc_persDataHandleOpen, test_DataHandleOpen);
   tcase_set_timeout(tc_persDataHandleOpen, 3);

   TCase * tc_Plugin = tcase_create("Plugin");
   tcase_add_test(tc_Plugin, test_Plugin);
   tcase_set_timeout(tc_Plugin, 3);

   TCase * tc_ReadDefault = tcase_create("ReadDefault");
   tcase_add_test(tc_ReadDefault, test_ReadDefault);
   tcase_set_timeout(tc_ReadDefault, 3);

   TCase * tc_ReadConfDefault = tcase_create("ReadConfDefault");
   tcase_add_test(tc_ReadConfDefault, test_ReadConfDefault);
   tcase_set_timeout(tc_ReadConfDefault, 3);

   TCase * tc_WriteConfDefault = tcase_create("WriteConfDefault");
   tcase_add_test(tc_WriteConfDefault, test_WriteConfDefault);
   tcase_set_timeout(tc_WriteConfDefault, 3);

   TCase * tc_InitDeinit = tcase_create("InitDeinit");
   tcase_add_test(tc_InitDeinit, test_InitDeinit);
   tcase_set_timeout(tc_InitDeinit, 3);

   TCase * tc_NegHandle = tcase_create("NegHandle");
   tcase_add_test(tc_NegHandle, test_NegHandle);
   tcase_set_timeout(tc_NegHandle, 3);

   TCase * tc_utf8_string = tcase_create("UTF-8");
   tcase_add_test(tc_utf8_string, test_utf8_string);
   tcase_set_timeout(tc_utf8_string, 3);

   TCase * tc_Notifications = tcase_create("Notifications");
   tcase_add_test(tc_Notifications, test_Notifications);
   tcase_set_timeout(tc_Notifications, 3);

#if USE_APPCHECK
   TCase * tc_ValidApplication = tcase_create("ValidApplication");
   tcase_add_test(tc_ValidApplication, test_ValidApplication);
   tcase_set_timeout(tc_ValidApplication, 3);
#endif

   TCase * tc_PAS_DbusInterface = tcase_create("PAS_DbusInterface");
   tcase_add_test(tc_PAS_DbusInterface, test_PAS_DbusInterface);
   tcase_set_timeout(tc_PAS_DbusInterface, 3);

   TCase * tc_LC_DbusInterface = tcase_create("LC_DbusInterface");
   tcase_add_test(tc_LC_DbusInterface, test_LC_DbusInterface);
   tcase_set_timeout(tc_LC_DbusInterface, 3);

   TCase * tc_SharedAccess = tcase_create("SharedAccess");
   tcase_add_test(tc_SharedAccess, test_SharedAccess);
   tcase_set_timeout(tc_SharedAccess, 3);

   TCase * tc_VO722 = tcase_create("VO722");
   tcase_add_test(tc_VO722, test_VO722);
   tcase_set_timeout(tc_VO722, 5);

   TCase * tc_NoRct = tcase_create("NoRct");
   tcase_add_test(tc_NoRct, test_NoRct);
   tcase_set_timeout(tc_NoRct, 3);

   TCase * tc_NoPluginFunc = tcase_create("NoPluginFunc");
   tcase_add_test(tc_NoPluginFunc, test_NoPluginFunc);

   TCase * tc_InvalidPluginfConf = tcase_create("InvalidPluginfConf");
   tcase_add_test(tc_InvalidPluginfConf, test_InvalidPluginfConf);

   TCase * tc_SharedData = tcase_create("SharedData");
   tcase_add_test(tc_SharedData, test_SharedData);
   tcase_set_timeout(tc_SharedData, 10);


#ifdef SKIP_MULTITHREADED_TESTS
   printf("INFO: Skipping testcase MultiThreadedRead  (%p)\n", test_MultiThreadedRead);
   printf("INFO: Skipping testcase MultiThreadedWrite (%p)\n", test_MultiThreadedWrite);
#else
   TCase * tc_MultiThreadedRead = tcase_create("MultiThreadedRead");
   tcase_add_test(tc_MultiThreadedRead, test_MultiThreadedRead);
   tcase_set_timeout(tc_MultiThreadedRead, 20);

   TCase * tc_MultiThreadedWrite = tcase_create("MultiThreadedWrite");
   tcase_add_test(tc_MultiThreadedWrite, test_MultiThreadedWrite);
   tcase_set_timeout(tc_MultiThreadedWrite, 20);
#endif


#if 1
   suite_add_tcase(s, tc_NoPluginFunc);

   suite_add_tcase(s, tc_persSetData);
   tcase_add_checked_fixture(tc_persSetData, data_setup, data_teardown);

   suite_add_tcase(s, tc_persGetData);
   tcase_add_checked_fixture(tc_persGetData, data_setup, data_teardown);

   suite_add_tcase(s, tc_persGetDataHandle);
   tcase_add_checked_fixture(tc_persGetDataHandle, data_setup, data_teardown);

   suite_add_tcase(s, tc_persSetDataNoPRCT);
   tcase_add_checked_fixture(tc_persSetDataNoPRCT, data_setup, data_teardown);

   suite_add_tcase(s, tc_persGetDataSize);
   tcase_add_checked_fixture(tc_persGetDataSize, data_setup, data_teardown);

   suite_add_tcase(s, tc_persDeleteData);
   tcase_add_checked_fixture(tc_persDeleteData, data_setup, data_teardown);

   suite_add_tcase(s, tc_persDataHandleOpen);
   tcase_add_checked_fixture(tc_persDataHandleOpen, data_setup, data_teardown);

   suite_add_tcase(s, tc_ReadDefault);
   tcase_add_checked_fixture(tc_ReadDefault, data_setup, data_teardown);

   suite_add_tcase(s, tc_ReadConfDefault);
   tcase_add_checked_fixture(tc_ReadConfDefault, data_setup, data_teardown);

   suite_add_tcase(s, tc_WriteConfDefault);
   tcase_add_checked_fixture(tc_WriteConfDefault, data_setup, data_teardown);

   suite_add_tcase(s, tc_NegHandle);
   tcase_add_checked_fixture(tc_NegHandle, data_setup, data_teardown);

   suite_add_tcase(s, tc_utf8_string);
   tcase_add_checked_fixture(tc_utf8_string, data_setup, data_teardown);

   suite_add_tcase(s, tc_Notifications);
   tcase_add_checked_fixture(tc_Notifications, data_setup, data_teardown);

   suite_add_tcase(s, tc_Plugin);
   tcase_add_checked_fixture(tc_Plugin, data_setup, data_teardown);

   suite_add_tcase(s, tc_SharedAccess);

   suite_add_tcase(s, tc_VO722);

   suite_add_tcase(s, tc_InvalidPluginfConf);

   suite_add_tcase(s, tc_NoRct);
   tcase_add_checked_fixture(tc_NoRct, data_setup_norct, data_teardown);

   suite_add_tcase(s, tc_InitDeinit);

   suite_add_tcase(s, tc_SharedData);
   tcase_add_checked_fixture(tc_SharedData, data_setup, data_teardown);

#endif


#if USE_APPCHECK
   suite_add_tcase(s, tc_ValidApplication);
#endif


#ifndef SKIP_MULTITHREADED_TESTS
   suite_add_tcase(s, tc_MultiThreadedRead);
   tcase_add_checked_fixture(tc_MultiThreadedRead, data_setup, data_teardown);

   suite_add_tcase(s, tc_MultiThreadedWrite);
   tcase_add_checked_fixture(tc_MultiThreadedWrite, data_setup, data_teardown);

#endif


#if 0
   suite_add_tcase(s, tc_PAS_DbusInterface);
   tcase_add_checked_fixture(tc_PAS_DbusInterface, data_setup, data_teardown);
   tcase_set_timeout(tc_PAS_DbusInterface, 10);

   suite_add_tcase(s, tc_LC_DbusInterface);
   tcase_add_checked_fixture(tc_LC_DbusInterface, data_setup, data_teardown);
   tcase_set_timeout(tc_LC_DbusInterface, 8);
#endif


   return s;
}


int main(int argc, char *argv[])
{
   int nr_failed = 0;
   (void)argv;

   // assign application name
   strncpy(gTheAppId, "lt-persistence_client_library_test", MaxAppNameLen);
   gTheAppId[MaxAppNameLen-1] = '\0';

   /// debug log and trace (DLT) setup
   DLT_REGISTER_APP("PCLTk", "PCL test");

   DLT_REGISTER_CONTEXT(gPcltDLTContext, "PCLt", "Context for PCL testing");

   DLT_LOG(gPcltDLTContext, DLT_LOG_INFO, DLT_STRING("Starting PCL test"));

   if(argc >= 2)
   {
      printf("Running concurrency tests\n");

      run_concurrency_test();
   }
   else
   {
      Suite * s = persistenceClientLib_suite();
      SRunner * sr = srunner_create(s);
      srunner_set_fork_status(sr, CK_NOFORK);
      srunner_set_xml(sr, "/tmp/persistenceClientLibraryTest.xml");
      srunner_set_log(sr, "/tmp/persistenceClientLibraryTest.log");

      srunner_run_all(sr, CK_VERBOSE /*CK_NORMAL CK_VERBOSE CK_SUBUNIT*/);

      nr_failed = srunner_ntests_failed(sr);
      srunner_ntests_run(sr);

      srunner_free(sr);
   }

   DLT_LOG(gPcltDLTContext, DLT_LOG_INFO, DLT_STRING("End of PCL test"));

   // unregister debug log and trace
   DLT_UNREGISTER_CONTEXT(gPcltDLTContext);
   DLT_UNREGISTER_APP();

   return (0==nr_failed)?EXIT_SUCCESS:EXIT_FAILURE;

}


void do_pcl_concurrency_access(const char* applicationID, const char* resourceID, int operation)
{
   int ret = 0, i = 0;
   size_t bufferSize = 0;
   int shutdownReg = PCL_SHUTDOWN_TYPE_FAST | PCL_SHUTDOWN_TYPE_NORMAL;
   unsigned char* buffer = NULL;

   char* writeBuffer = "Pack my box with five dozen liquor jugs. - "
      "Jackdaws love my big sphinx of quartz. - "
      "The five boxing wizards jump quickly. - "
      "How vexingly quick daft zebras jump! - "
      "Bright vixens jump; dozy fowl quack - "
      "Sphinx of black quartz, judge my vow"
      "Voyez le brick géant que j’examine près du wha"
      "Zornig und gequält rügen jeweils Pontifex und Volk die maßlose bischöfliche Hybris"
      "Xaver schreibt für Wikipedia zum Spaß quälend lang über Yoga, Soja und Öko"
      "Polyfon zwitschernd aßen Mäxchens Vögel Rüben, Joghurt und Quark"
      "Fix, Schwyz!“ quäkt Jürgen blöd vom Paß";

   bufferSize = strlen(writeBuffer);

   buffer = malloc(bufferSize);

   if(buffer != NULL)
   {
      (void)pclInitLibrary(applicationID, shutdownReg);

      for(i=0; i< 10000; i++)
      {
         memset(buffer, 0, bufferSize);

         //printf("[%d] - i: %d \n", operation, i);
         if(operation == 0 )
         {
            ret = pclKeyWriteData(0x20, resourceID,  2, 1,(unsigned char* )writeBuffer, (int)bufferSize);
            if(ret < 0)
               printf("Failed to write data: %d\n", ret);

            ret = pclKeyReadData(0x20, resourceID,  2, 1, buffer, (int)bufferSize);
            if(ret < 0)
            {
               printf("Failed to read data: %d\n", ret);
            }
            else
            {
               if(strncmp((char*)buffer, writeBuffer, (size_t)ret) != 0)
                  printf("Wrong buffer\n");
            }
         }
         else if(operation == 1)
         {
            ret = pclKeyReadData(0x20, resourceID, 2, 1, buffer, (int)bufferSize);
            if(ret < 0)
            {
               printf("Failed to read data: %d\n", ret);
            }
            else
            {
               if(strncmp((char*)buffer, writeBuffer, (size_t)ret) != 0)
                  printf("Wrong buffer\n");
            }
         }
         else
         {
            printf("invalid operation - end!! \n");
            break;
         }

         if(operation == 0)
            usleep(1500);
         else
            usleep(1000);
      }

      free(buffer);

      pclDeinitLibrary();
   }

}


void run_concurrency_test()
{
   const char* appId_one = "lt-persistence_client_library_test";
   const char* appId_two = "concurrency_test";

   int pid = fork();

   if (pid == 0)
   { /*child*/
      printf("Started child process with PID: [%d] \n", pid);

      do_pcl_concurrency_access(appId_one, "links/last_link2", 0); //write

      printf("CHILD exits! \n");

      _exit(EXIT_SUCCESS);
   }
   else if (pid > 0)
   { /*parent*/
      printf("Started father process with PID: [%d] \n", pid);

      do_pcl_concurrency_access(appId_two, "links/last_link2", 1); //read

      printf("PARENT exits! \n");

      _exit(EXIT_SUCCESS);
   }
}


const char* gWriteBuffer =   "Pack my box with five dozen liquor jugs. - "
   "Jackdaws love my big sphinx of quartz. - "
   "The five boxing wizards jump quickly. - "
   "How vexingly quick daft zebras jump! - "
   "Bright vixens jump; dozy fowl quack - "
   "Sphinx of black quartz, judge my vow"
   "Voyez le brick géant que j’examine près du wha"
   "Zornig und gequält rügen jeweils Pontifex und Volk die maßlose bischöfliche Hybris"
   "Xaver schreibt für Wikipedia zum Spaß quälend lang über Yoga, Soja und Öko"
   "Polyfon zwitschernd aßen Mäxchens Vögel Rüben, Joghurt und Quark"
   "Fix, Schwyz!“ quäkt Jürgen blöd vom Paß"
   "Welch fieser Katzentyp quält da süße Vögel bloß zum Jux"
   "Die heiße Zypernsonne quälte Max und Victoria ja böse auf dem Weg bis zur Küste"
   "Pack my box with five dozen liquor jugs. - "
   "Jackdaws love my big sphinx of quartz. - "
   "The five boxing wizards jump quickly. - "
   "How vexingly quick daft zebras jump! - "
   "Bright vixens jump; dozy fowl quack - "
   "Sphinx of black quartz, judge my vow"
   "Voyez le brick géant que j’examine près du wha"
   "Zornig und gequält rügen jeweils Pontifex und Volk die maßlose bischöfliche Hybris"
   "Xaver schreibt für Wikipedia zum Spaß quälend lang über Yoga, Soja und Öko"
   "Polyfon zwitschernd aßen Mäxchens Vögel Rüben, Joghurt und Quark"
   "Fix, Schwyz!“ quäkt Jürgen blöd vom Paß"
   "Welch fieser Katzentyp quält da süße Vögel bloß zum Jux"
   "Die heiße Zypernsonne quälte Max und Victoria ja böse auf dem Weg bis zur Küste"
   "Pack my box with five dozen liquor jugs. - "
   "Jackdaws love my big sphinx of quartz. - "
   "The five boxing wizards jump quickly. - "
   "How vexingly quick daft zebras jump! - "
   "Bright vixens jump; dozy fowl quack - "
   "Sphinx of black quartz, judge my vow"
   "Voyez le brick géant que j’examine près du wha"
   "Zornig und gequält rügen jeweils Pontifex und Volk die maßlose bischöfliche Hybris"
   "Xaver schreibt für Wikipedia zum Spaß quälend lang über Yoga, Soja und Öko"
   "Polyfon zwitschernd aßen Mäxchens Vögel Rüben, Joghurt und Quark"
   "Fix, Schwyz!“ quäkt Jürgen blöd vom Paß"
   "Welch fieser Katzentyp quält da süße Vögel bloß zum Jux"
   "Die heiße Zypernsonne quälte Max und Victoria ja böse auf dem Weg bis zur Küste"
   "Pack my box with five dozen liquor jugs. - "
   "Jackdaws love my big sphinx of quartz. - "
   "The five boxing wizards jump quickly. - "
   "How vexingly quick daft zebras jump! - "
   "Bright vixens jump; dozy fowl quack - "
   "Sphinx of black quartz, judge my vow"
   "Voyez le brick géant que j’examine près du wha"
   "Zornig und gequält rügen jeweils Pontifex und Volk die maßlose bischöfliche Hybris"
   "Xaver schreibt für Wikipedia zum Spaß quälend lang über Yoga, Soja und Öko"
   "Polyfon zwitschernd aßen Mäxchens Vögel Rüben, Joghurt und Quark"
   "Fix, Schwyz!“ quäkt Jürgen blöd vom Paß"
   "Welch fieser Katzentyp quält da süße Vögel bloß zum Jux"
   "Die heiße Zypernsonne quälte Max und Victoria ja böse auf dem Weg bis zur Küste""Pack my box with five dozen liquor jugs. - "
   "Jackdaws love my big sphinx of quartz. - "
   "The five boxing wizards jump quickly. - "
   "How vexingly quick daft zebras jump! - "
   "Bright vixens jump; dozy fowl quack - "
   "Sphinx of black quartz, judge my vow"
   "Voyez le brick géant que j’examine près du wha"
   "Zornig und gequält rügen jeweils Pontifex und Volk die maßlose bischöfliche Hybris"
   "Xaver schreibt für Wikipedia zum Spaß quälend lang über Yoga, Soja und Öko"
   "Polyfon zwitschernd aßen Mäxchens Vögel Rüben, Joghurt und Quark"
   "Fix, Schwyz!“ quäkt Jürgen blöd vom Paß"
   "Welch fieser Katzentyp quält da süße Vögel bloß zum Jux"
   "Die heiße Zypernsonne quälte Max und Victoria ja böse auf dem Weg bis zur Küste"
   "Pack my box with five dozen liquor jugs. - "
   "Jackdaws love my big sphinx of quartz. - "
   "The five boxing wizards jump quickly. - "
   "How vexingly quick daft zebras jump! - "
   "Bright vixens jump; dozy fowl quack - "
   "Sphinx of black quartz, judge my vow"
   "Voyez le brick géant que j’examine près du wha"
   "Zornig und gequält rügen jeweils Pontifex und Volk die maßlose bischöfliche Hybris"
   "Xaver schreibt für Wikipedia zum Spaß quälend lang über Yoga, Soja und Öko"
   "Polyfon zwitschernd aßen Mäxchens Vögel Rüben, Joghurt und Quark"
   "Fix, Schwyz!“ quäkt Jürgen blöd vom Paß"
   "Welch fieser Katzentyp quält da süße Vögel bloß zum Jux"
   "Die heiße Zypernsonne quälte Max und Victoria ja böse auf dem Weg bis zur Küste"
   "Pack my box with five dozen liquor jugs. - "
   "Jackdaws love my big sphinx of quartz. - "
   "The five boxing wizards jump quickly. - "
   "How vexingly quick daft zebras jump! - "
   "Bright vixens jump; dozy fowl quack - "
   "Sphinx of black quartz, judge my vow"
   "Voyez le brick géant que j’examine près du wha"
   "Zornig und gequält rügen jeweils Pontifex und Volk die maßlose bischöfliche Hybris"
   "Xaver schreibt für Wikipedia zum Spaß quälend lang über Yoga, Soja und Öko"
   "Polyfon zwitschernd aßen Mäxchens Vögel Rüben, Joghurt und Quark"
   "Fix, Schwyz!“ quäkt Jürgen blöd vom Paß"
   "Welch fieser Katzentyp quält da süße Vögel bloß zum Jux"
   "Die heiße Zypernsonne quälte Max und Victoria ja böse auf dem Weg bis zur Küste"
   "Pack my box with five dozen liquor jugs. - "
   "Jackdaws love my big sphinx of quartz. - "
   "The five boxing wizards jump quickly. - "
   "How vexingly quick daft zebras jump! - "
   "Bright vixens jump; dozy fowl quack - "
   "Sphinx of black quartz, judge my vow"
   "Voyez le brick géant que j’examine près du wha"
   "Zornig und gequält rügen jeweils Pontifex und Volk die maßlose bischöfliche Hybris"
   "Xaver schreibt für Wikipedia zum Spaß quälend lang über Yoga, Soja und Öko"
   "Polyfon zwitschernd aßen Mäxchens Vögel Rüben, Joghurt und Quark"
   "Fix, Schwyz!“ quäkt Jürgen blöd vom Paß"
   "Welch fieser Katzentyp quält da süße Vögel bloß zum Jux"
   "Die heiße Zypernsonne quälte Max und Victoria ja böse auf dem Weg bis zur Küste";

const char* gWriteBuffer2 =   "Pack my box with five dozen liquor jugs. - "
   "Jackdaws love my big sphinx of quartz. - "
   "The five boxing wizards jump quickly. - "
   "How vexingly quick daft zebras jump! - "
   "Bright vixens jump; dozy fowl quack - "
   "Sphinx of black quartz, judge my vow"
   "Voyez le brick géant que j’examine près du wha"
   "Zornig und gequält rügen jeweils Pontifex und Volk die maßlose bischöfliche Hybris"
   "Xaver schreibt für Wikipedia zum Spaß quälend lang über Yoga, Soja und Öko"
   "Polyfon zwitschernd aßen Mäxchens Vögel Rüben, Joghurt und Quark"
   "Fix, Schwyz!“ quäkt Jürgen blöd vom Paß"
   "Welch fieser Katzentyp quält da süße Vögel bloß zum Jux"
   "Die heiße Zypernsonne quälte Max und Victoria ja böse auf dem Weg bis zur Küste"
   "Pack my box with five dozen liquor jugs. - "
   "Jackdaws love my big sphinx of quartz. - "
   "The five boxing wizards jump quickly. - "
   "How vexingly quick daft zebras jump! - "
   "Bright vixens jump; dozy fowl quack - "
   "Sphinx of black quartz, judge my vow"
   "Voyez le brick géant que j’examine près du wha"
   "Zornig und gequält rügen jeweils Pontifex und Volk die maßlose bischöfliche Hybris"
   "Xaver schreibt für Wikipedia zum Spaß quälend lang über Yoga, Soja und Öko"
   "Polyfon zwitschernd aßen Mäxchens Vögel Rüben, Joghurt und Quark"
   "Fix, Schwyz!“ quäkt Jürgen blöd vom Paß"
   "Welch fieser Katzentyp quält da süße Vögel bloß zum Jux"
   "Die heiße Zypernsonne quälte Max und Victoria ja böse auf dem Weg bis zur Küste"
   "Pack my box with five dozen liquor jugs. - "
   "Jackdaws love my big sphinx of quartz. - "
   "The five boxing wizards jump quickly. - "
   "How vexingly quick daft zebras jump! - "
   "Bright vixens jump; dozy fowl quack - "
   "Sphinx of black quartz, judge my vow"
   "Voyez le brick géant que j’examine près du wha"
   "Zornig und gequält rügen jeweils Pontifex und Volk die maßlose bischöfliche Hybris"
   "Xaver schreibt für Wikipedia zum Spaß quälend lang über Yoga, Soja und Öko"
   "Polyfon zwitschernd aßen Mäxchens Vögel Rüben, Joghurt und Quark"
   "Fix, Schwyz!“ quäkt Jürgen blöd vom Paß"
   "Welch fieser Katzentyp quält da süße Vögel bloß zum Jux"
   "Die heiße Zypernsonne quälte Max und Victoria ja böse auf dem Weg bis zur Küste"
   "Pack my box with five dozen liquor jugs. - "
   "Jackdaws love my big sphinx of quartz. - "
   "The five boxing wizards jump quickly. - "
   "How vexingly quick daft zebras jump! - "
   "Bright vixens jump; dozy fowl quack - "
   "Sphinx of black quartz, judge my vow"
   "Voyez le brick géant que j’examine près du wha"
   "Zornig und gequält rügen jeweils Pontifex und Volk die maßlose bischöfliche Hybris"
   "Xaver schreibt für Wikipedia zum Spaß quälend lang über Yoga, Soja und Öko"
   "Polyfon zwitschernd aßen Mäxchens Vögel Rüben, Joghurt und Quark"
   "Fix, Schwyz!“ quäkt Jürgen blöd vom Paß"
   "Welch fieser Katzentyp quält da süße Vögel bloß zum Jux"
   "Die heiße Zypernsonne quälte Max und Victoria ja böse auf dem Weg bis zur Küste""Pack my box with five dozen liquor jugs. - "
   "Jackdaws love my big sphinx of quartz. - "
   "The five boxing wizards jump quickly. - "
   "How vexingly quick daft zebras jump! - "
   "Bright vixens jump; dozy fowl quack - "
   "Sphinx of black quartz, judge my vow"
   "Voyez le brick géant que j’examine près du wha"
   "Zornig und gequält rügen jeweils Pontifex und Volk die maßlose bischöfliche Hybris"
   "Xaver schreibt für Wikipedia zum Spaß quälend lang über Yoga, Soja und Öko"
   "Polyfon zwitschernd aßen Mäxchens Vögel Rüben, Joghurt und Quark"
   "Fix, Schwyz!“ quäkt Jürgen blöd vom Paß"
   "Welch fieser Katzentyp quält da süße Vögel bloß zum Jux"
   "Die heiße Zypernsonne quälte Max und Victoria ja böse auf dem Weg bis zur Küste"
   "Pack my box with five dozen liquor jugs. - "
   "Jackdaws love my big sphinx of quartz. - "
   "The five boxing wizards jump quickly. - ";

