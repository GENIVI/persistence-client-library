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

#include <dlt/dlt.h>
#include <dlt/dlt_common.h>

#include "persCheck.h"


#include "../include/persistence_client_library_file.h"
#include "../include/persistence_client_library_key.h"
#include "../include/persistence_client_library.h"
#include "../include/persistence_client_library_error_def.h"



#define BUF_SIZE     64
#define NUM_OF_FILES 3
#define READ_SIZE    1024
#define MaxAppNameLen 256

#define SOURCE_PATH "/Data/mnt-c/lt-persistence_client_library_test/"


static const char* gPathSegemnts[] = {"user/", "1/", "seat/", "1/", "media", NULL };

/// application id
char gTheAppId[MaxAppNameLen] = {0};

// definition of weekday
char* dayOfWeek[] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
char* gWriteBackupTestData  = "This is the content of the file /Data/mnt-c/lt-persistence_client_library_test/user/1/seat/1/media/mediaDB_ReadWrite.db";
char* gWriteRecoveryTestData = "This is the data to recover: /Data/mnt-c/lt-persistence_client_library_test/user/1/seat/1/media/mediaDB_DataRecovery.db";
char* gRecovChecksum = "608a3b5d";	// generated with http://www.tools4noobs.com/online_php_functions/crc32/



void data_teardown(void)
{
   printf("* * * tear down * * *\n");	// nothing
}


int myChangeCallback(pclNotification_s * notifyStruct)
{
   printf(" ==> * - * myChangeCallback * - *\n");

   return 1;
}



/**
 * Test the key value interface using different logicalDB id's, users and seats.
 * Each resource below has an entry in the resource configuration table where the
 * storage location (cached or write through) and type (e.g. custom) has been configured.
 */
START_TEST(test_GetData)
{
   X_TEST_REPORT_TEST_NAME("persistence_client_library_test");
   X_TEST_REPORT_COMP_NAME("libpersistence_client_library");
   X_TEST_REPORT_REFERENCE("NONE");
   X_TEST_REPORT_DESCRIPTION("Test of get data");
   X_TEST_REPORT_TYPE(GOOD);

   int ret = 0;
   unsigned int shutdownReg = (PCL_SHUTDOWN_TYPE_FAST | PCL_SHUTDOWN_TYPE_NORMAL);

   unsigned char buffer[READ_SIZE] = {0};

   ret = pclInitLibrary(gTheAppId, shutdownReg);
   x_fail_unless(ret <= 1, "Failed to init PCL");

#if 1
   /**
    * Logical DB ID: 0xFF with user 0 and seat 0
    *       ==> local value accessible by all users (user 0, seat 0)
    */
   ret = pclKeyReadData(0xFF, "pos/last_position",         1, 1, buffer, READ_SIZE);
   //printf("----test_GetData => pos/last_position: \"%s\" => ret: %d \nReference: %s => size: %d\n", buffer, ret, "CACHE_ +48 10' 38.95, +8 44' 39.06", strlen("CACHE_ +48 10' 38.95, +8 44' 39.06"));
   x_fail_unless(strncmp((char*)buffer, "CACHE_ +48 10' 38.95, +8 44' 39.06",
                 strlen((char*)buffer)) == 0, "Buffer not correctly read - pos/last_position");
   x_fail_unless(ret == strlen("CACHE_ +48 10' 38.95, +8 44' 39.06"));
   memset(buffer, 0, READ_SIZE);

   /**
    * Logical DB ID: 0xFF with user 0 and seat 0
    *       ==> local value accessible by all users (user 0, seat 0)
    */
   /*
   ret = pclKeyReadData(0xFF, "language/country_code",         0, 0, buffer, READ_SIZE);
   x_fail_unless(strncmp((char*)buffer, "Custom plugin -> plugin_get_data: secure!",
               strlen((char*)buffer)) == 0, "Buffer not correctly read");
   x_fail_unless(ret = strlen("Custom plugin -> plugin_get_data_handle"));
   memset(buffer, 0, READ_SIZE);
   */


   /**
    * Logical DB ID: 0 with user 3 and seat 0
    *       ==> public shared user value (user 3, seat 0)
    */
   //ret = pclKeyReadData(0,    "language/current_language", 0, 0, buffer, READ_SIZE);
   //printf("----test_GetData => language/current_language \"%s\" => ret: %d \n", buffer, ret);
   //x_fail_unless(strncmp((char*)buffer, "CACHE_ Kisuaheli", strlen((char*)buffer)) == 0, "Buffer not correctly read");
   //memset(buffer, 0, READ_SIZE);

   /**
    * Logical DB ID: 0xFF with user 3 and seat 2
    *       ==> local USER value (user 3, seat 2)
    */
   ret = pclKeyReadData(0xFF, "status/open_document",      3, 2, buffer, READ_SIZE);
   //printf("----test_GetData => status/open_document \"%s\" => ret: %d \n", buffer, ret);
   x_fail_unless(strncmp((char*)buffer, "WT_ /var/opt/user_manual_climateControl.pdf", strlen((char*)buffer)) == 0,
   		        "Buffer not correctly read - status/open_document");
   x_fail_unless(ret == strlen("WT_ /var/opt/user_manual_climateControl.pdf"));
   memset(buffer, 0, READ_SIZE);

   /**
    * Logical DB ID: 0x20 with user 4 and seat 0
    *       ==> shared user value accessible by a group (user 4 and seat 0)
    */
   ret = pclKeyReadData(0x20, "address/home_address",      4, 0, buffer, READ_SIZE);
   //printf("----test_GetData => address/home_address \"%s\" => ret: %d \n", buffer, ret);
   x_fail_unless(strncmp((char*)buffer, "WT_ 55327 Heimatstadt, Wohnstrasse 31", strlen((char*)buffer)) == 0,
   		        "Buffer not correctly read - address/home_address");
   x_fail_unless(ret == strlen("WT_ 55327 Heimatstadt, Wohnstrasse 31"));
   memset(buffer, 0, READ_SIZE);

   /**
    * Logical DB ID: 0xFF with user 0 and seat 0
    *       ==> local value accessible by ALL USERS (user 0, seat 0)
    */
   ret = pclKeyReadData(0xFF, "pos/last_satellites",       0, 0, buffer, READ_SIZE);
   //printf("----test_GetData => pos/last_satellites \"%s\" => ret: %d \n", buffer, ret);
   x_fail_unless(strncmp((char*)buffer, "WT_ 17", strlen((char*)buffer)) == 0,
   		        "Buffer not correctly read - pos/last_satellites");
   x_fail_unless(ret == strlen("WT_ 17"));
   memset(buffer, 0, READ_SIZE);

   /**
    * Logical DB ID: 0x20 with user 4 and seat 0
    *       ==> shared user value accessible by A GROUP (user 4 and seat 0)
    */
   ret = pclKeyReadData(0x20, "links/last_link",           2, 0, buffer, READ_SIZE);
   //printf("----test_GetData => links/last_link \"%s\" => ret: %d \n", buffer, ret);
   x_fail_unless(strncmp((char*)buffer, "CACHE_ /last_exit/queens", strlen((char*)buffer)) == 0,
   		        "Buffer not correctly read - links/last_link");
   x_fail_unless(ret == strlen("CACHE_ /last_exit/queens"));
   memset(buffer, 0, READ_SIZE);

#endif
   pclDeinitLibrary();
}
END_TEST



/**
 * Test the key value  h a n d l e  interface using different logicalDB id's, users and seats
 * Each resource below has an entry in the resource configuration table where
 * the storage location (cached or write through) and type (e.g. custom) has bee configured.
 */
START_TEST (test_GetDataHandle)
{
   X_TEST_REPORT_TEST_NAME("persistence_client_library_test");
   X_TEST_REPORT_COMP_NAME("libpersistence_client_library");
   X_TEST_REPORT_REFERENCE("NONE");
   X_TEST_REPORT_DESCRIPTION("Test of get data handle");
   X_TEST_REPORT_TYPE(GOOD);

   int ret = 0, handle = 0, handle2 = 0, handle3 = 0, handle4 = 0, size = 0;
   unsigned int shutdownReg = PCL_SHUTDOWN_TYPE_FAST | PCL_SHUTDOWN_TYPE_NORMAL;

   unsigned char buffer[READ_SIZE] = {0};
   struct tm *locTime;

   char sysTimeBuffer[128];

   ret = pclInitLibrary(gTheAppId, shutdownReg);
   x_fail_unless(ret <= 1, "Failed to init PCL");
#if 1
   time_t t = time(0);

   locTime = localtime(&t);

   snprintf(sysTimeBuffer, 128, "TimeAndData: \"%s %d.%d.%d - %d:%.2d:%.2d Uhr\"", dayOfWeek[locTime->tm_wday], locTime->tm_mday, locTime->tm_mon, (locTime->tm_year+1900),
                                                                  locTime->tm_hour, locTime->tm_min, locTime->tm_sec);


   // open handle ---------------------------------------------------
   /**
    * Logical DB ID: 0xFF with user 0 and seat 0
    *       ==> local value accessible by ALL USERS (user 0, seat 0)
    */
   handle = pclKeyHandleOpen(0xFF, "posHandle/last_position", 0, 0);
   x_fail_unless(handle >= 0, "Failed to open handle ==> /posHandle/last_position");

   ret = pclKeyHandleReadData(handle, buffer, READ_SIZE);
   x_fail_unless(strncmp((char*)buffer, "WT_ H A N D L E: +48° 10' 38.95\", +8° 44' 39.06\"", ret-1) == 0, "Buffer not correctly read => 1");

   size = pclKeyHandleGetSize(handle);
   x_fail_unless(size == strlen("WT_ H A N D L E: +48° 10' 38.95\", +8° 44' 39.06\""));
   // ---------------------------------------------------------------------------------------------


   // open handle ---------------------------------------------------
   /**
    * Logical DB ID: 0xFF with user 3 and seat 2
    *       ==> local USER value (user 3, seat 2)
    */
   handle2 = pclKeyHandleOpen(0xFF, "statusHandle/open_document", 3, 2);
   x_fail_unless(handle2 >= 0, "Failed to open handle /statusHandle/open_document");

   size = pclKeyHandleWriteData(handle2, (unsigned char*)sysTimeBuffer, strlen(sysTimeBuffer));
   x_fail_unless(size == strlen(sysTimeBuffer));
   // close
   ret = pclKeyHandleClose(handle2);
   // ---------------------------------------------------------------------------------------------


   // open handle ---------------------------------------------------
   /**
    * Logical DB ID: 0xFF with user 0 and seat 0
    *       ==> local value accessible by ALL USERS (user 0, seat 0)
    */
#if 0 // plugin test case
   memset(buffer, 0, READ_SIZE);
   handle4 = pclKeyHandleOpen(0xFF, "language/country_code", 0, 0);
   printf("H A N D L E: %d\n", handle4);
   x_fail_unless(handle4 >= 0, "Failed to open handle /language/country_code");

   ret = pclKeyHandleReadData(handle4, buffer, READ_SIZE);
   x_fail_unless(strncmp((char*)buffer, "Custom plugin -> plugin_get_data_handle: secure!", -1) == 0, "Buffer not correctly read => 2");

   size = pclKeyHandleGetSize(handle4);
   x_fail_unless(size = strlen("Custom plugin -> plugin_get_data_handle"));

   ret = pclKeyHandleWriteData(handle4, (unsigned char*)"Only dummy implementation behind custom library", READ_SIZE);
#endif
   // ---------------------------------------------------------------------------------------------


   // open handle ---------------------------------------------------
   /**
    * Logical DB ID: 0xFF with user 3 and seat 2
    *       ==> local USER value (user 3, seat 2)
    */
   handle3 = pclKeyHandleOpen(0xFF, "statusHandle/open_document", 3, 2);
   x_fail_unless(handle3 >= 0, "Failed to open handle /statusHandle/open_document");

   ret = pclKeyHandleReadData(handle3, buffer, READ_SIZE);
   x_fail_unless(strncmp((char*)buffer, sysTimeBuffer, strlen(sysTimeBuffer)) == 0, "Buffer not correctly read => 3");

   size = pclKeyHandleGetSize(handle3);
   x_fail_unless(size = strlen(sysTimeBuffer));
   // ---------------------------------------------------------------------------------------------


   // close handle
   ret = pclKeyHandleClose(handle);
   ret = pclKeyHandleClose(handle3);
   ret = pclKeyHandleClose(handle4);
#endif
   pclDeinitLibrary();
}
END_TEST


/*
 * Write data to a key using the key interface.
 * First write data to different keys and after
 * read the data for verification.
 */
START_TEST(test_SetData)
{
   X_TEST_REPORT_TEST_NAME("persistence_client_library_test");
   X_TEST_REPORT_COMP_NAME("libpersistence_client_library");
   X_TEST_REPORT_REFERENCE("NONE");
   X_TEST_REPORT_DESCRIPTION("Test of set data");
   X_TEST_REPORT_TYPE(GOOD);

   int ret = 0;
   unsigned int shutdownReg = PCL_SHUTDOWN_TYPE_FAST | PCL_SHUTDOWN_TYPE_NORMAL;
   unsigned char buffer[READ_SIZE]  = {0};
   char write1[READ_SIZE] = {0};
   char write2[READ_SIZE] = {0};
   char sysTimeBuffer[256];

   struct tm *locTime;

   ret = pclInitLibrary(gTheAppId, shutdownReg);
   x_fail_unless(ret <= 1, "Failed to init PCL");

#if 1
   /**
    * Logical DB ID: 0xFF with user 3 and seat 2
    *       ==> local USER value (user 3, seat 2)
    */
   ret = pclKeyWriteData(0xFF, "status/open_document",      3, 2, (unsigned char*)"WT_ /var/opt/user_manual_climateControl.pdf", strlen("WT_ /var/opt/user_manual_climateControl.pdf"));
   x_fail_unless(ret == strlen("WT_ /var/opt/user_manual_climateControl.pdf"), "Wrong write size");


   ret = pclKeyWriteData(0x84, "links/last_link",      2, 1, (unsigned char*)"CACHE_ /last_exit/queens", strlen("CACHE_ /last_exit/queens"));
   x_fail_unless(ret == strlen("CACHE_ /last_exit/queens"), "Wrong write size");

   ret = pclKeyWriteData(0xFF, "posHandle/last_position", 0, 0, (unsigned char*)"WT_ H A N D L E: +48° 10' 38.95\", +8° 44' 39.06\"", strlen("WT_ H A N D L E: +48° 10' 38.95\", +8° 44' 39.06\""));
   x_fail_unless(ret == strlen("WT_ H A N D L E: +48° 10' 38.95\", +8° 44' 39.06\""), "Wrong write size");
#endif

#if 1
   time_t t = time(0);

   locTime = localtime(&t);

   // write data
   snprintf(sysTimeBuffer, 128, "\"%s %d.%d.%d - %d:%.2d:%.2d Uhr\"", dayOfWeek[locTime->tm_wday], locTime->tm_mday, locTime->tm_mon, (locTime->tm_year+1900),
                                                                 locTime->tm_hour, locTime->tm_min, locTime->tm_sec);

   /**
    * Logical DB ID: 0xFF with user 1 and seat 2
    *       ==> local USER value (user 1, seat 2)
    * Resource ID: 69
    */
   ret = pclKeyWriteData(0xFF, "69", 1, 2, (unsigned char*)sysTimeBuffer, strlen(sysTimeBuffer));
   x_fail_unless(ret == strlen(sysTimeBuffer), "Wrong write size");
#if 1
   snprintf(write1, 128, "%s %s", "/70",  sysTimeBuffer);
   /**
    * Logical DB ID: 0xFF with user 1 and seat 2
    *       ==> local USER value (user 1, seat 2)
    * Resource ID: 70
    */
   ret = pclKeyWriteData(0xFF, "70", 1, 2, (unsigned char*)write1, strlen(write1));
   x_fail_unless(ret == strlen(write1), "Wrong write size");

   snprintf(write2, 128, "%s %s", "/key_70",  sysTimeBuffer);
   /**
    * Logical DB ID: 0xFF with user 1 and seat 2
    *       ==> local USER value (user 1, seat 2)
    * Resource ID: key_70
    */
   ret = pclKeyWriteData(0xFF, "key_70", 1, 2, (unsigned char*)write2, strlen(write2));
   x_fail_unless(ret == strlen(write2), "Wrong write size");


   /*******************************************************************************************************************************************/
   /* used for changed notification testing */
   /*******************************************************************************************************************************************/
   /**
    * Logical DB ID: 0x84 with user 2 and seat 1
    *       ==> shared user value accessible by A GROUP (user 2 and seat 1)
    *
    *       ==> used for shared testing
    */
   //printf("Write data to trigger change notification\n");
   ret = pclKeyWriteData(0x84, "links/last_link2",  2, 1, (unsigned char*)"Test notify shared data", strlen("Test notify shared data"));
   x_fail_unless(ret == strlen("Test notify shared data"), "Wrong write size");

   /**
    * Logical DB ID: 0x84 with user 2 and seat 1
    *       ==> shared user value accessible by A GROUP (user 2 and seat 1)
    *
    *       ==> used for shared testing
    */
   //printf("Write data to trigger change notification\n");
   ret = pclKeyWriteData(0x84, "links/last_link3",  3, 2, (unsigned char*)"Test notify shared data", strlen("Test notify shared data"));
   x_fail_unless(ret == strlen("Test notify shared data"), "Wrong write size");

   /**
    * Logical DB ID: 0x84 with user 2 and seat 1
    *       ==> shared user value accessible by A GROUP (user 2 and seat 1)
    *
    *       ==> used for shared testing
    */
   //printf("Write data to trigger change notification\n");
   ret = pclKeyWriteData(0x84, "links/last_link4",  4, 1, (unsigned char*)"Test notify shared data", strlen("Test notify shared data"));
   x_fail_unless(ret == strlen("Test notify shared data"), "Wrong write size");
   /*******************************************************************************************************************************************/
   /*******************************************************************************************************************************************/


   /*
    * now read the data written in the previous steps to the keys
    * and verify data has been written correctly.
    */
   memset(buffer, 0, READ_SIZE);

   ret = pclKeyReadData(0xFF, "69", 1, 2, buffer, READ_SIZE);
   x_fail_unless(strncmp((char*)buffer, sysTimeBuffer, strlen(sysTimeBuffer)) == 0, "Buffer not correctly read");
   x_fail_unless(ret == strlen(sysTimeBuffer), "Wrong read size");

   memset(buffer, 0, READ_SIZE);

   ret = pclKeyReadData(0xFF, "70", 1, 2, buffer, READ_SIZE);
   x_fail_unless(strncmp((char*)buffer, write1, strlen(write1)) == 0, "Buffer not correctly read");
   x_fail_unless(ret == strlen(write1), "Wrong read size");

   memset(buffer, 0, READ_SIZE);

   ret = pclKeyReadData(0xFF, "key_70", 1, 2, buffer, READ_SIZE);
   x_fail_unless(strncmp((char*)buffer, write2, strlen(write2)) == 0, "Buffer not correctly read");
   x_fail_unless(ret == strlen(write2), "Wrong read size");
#endif
#endif
   pclDeinitLibrary();
}
END_TEST



/**
 * Write data to a key using the key interface.
 * The key is not in the persistence resource table.
 * The key sill then be stored to the location local and cached.
 */
START_TEST(test_SetDataNoPRCT)
{
   X_TEST_REPORT_TEST_NAME("persistence_client_library_test");
   X_TEST_REPORT_COMP_NAME("libpersistence_client_library");
   X_TEST_REPORT_REFERENCE("NONE");
   X_TEST_REPORT_DESCRIPTION("Test of set data no PRCT");
   X_TEST_REPORT_TYPE(GOOD);

   int ret = 0;
   unsigned int shutdownReg = PCL_SHUTDOWN_TYPE_FAST | PCL_SHUTDOWN_TYPE_NORMAL;
   unsigned char buffer[READ_SIZE] = {0};
   struct tm *locTime;

   ret = pclInitLibrary(gTheAppId, shutdownReg);
   x_fail_unless(ret <= 1, "Failed to init PCL");
#if 1
   time_t t = time(0);

   char sysTimeBuffer[128];

   locTime = localtime(&t);

   snprintf(sysTimeBuffer, 128, "TimeAndData: \"%s %d.%d.%d - %d:%.2d:%.2d Uhr\"", dayOfWeek[locTime->tm_wday], locTime->tm_mday, locTime->tm_mon, (locTime->tm_year+1900),
                                                                  locTime->tm_hour, locTime->tm_min, locTime->tm_sec);

   /**
    * Logical DB ID: 0xFF with user 1 and seat 2
    *       ==> local USER value (user 1, seat 2)
    */
   ret = pclKeyWriteData(0xFF, "NoPRCT", 1, 2, (unsigned char*)sysTimeBuffer, strlen(sysTimeBuffer));
   x_fail_unless(ret == strlen(sysTimeBuffer), "Wrong write size");
   //printf("Write Buffer : %s\n", sysTimeBuffer);

   // read data again and and verify datat has been written correctly
   memset(buffer, 0, READ_SIZE);

   ret = pclKeyReadData(0xFF, "NoPRCT", 1, 2, buffer, READ_SIZE);
   x_fail_unless(strncmp((char*)buffer, sysTimeBuffer, strlen(sysTimeBuffer)) == 0, "Buffer not correctly read");
   x_fail_unless(ret == strlen(sysTimeBuffer), "Wrong read size");
   //printf("read buffer  : %s\n", buffer);
#endif
   pclDeinitLibrary();
}
END_TEST



/*
 * Test the key interface.
 * Read the size of a key.
 */
START_TEST(test_GetDataSize)
{
   X_TEST_REPORT_TEST_NAME("persistence_client_library_test");
   X_TEST_REPORT_COMP_NAME("libpersistence_client_library");
   X_TEST_REPORT_REFERENCE("NONE");
   X_TEST_REPORT_DESCRIPTION("Test of get data size");
   X_TEST_REPORT_TYPE(GOOD);

   int size = 0, ret = 0;

   unsigned int shutdownReg = PCL_SHUTDOWN_TYPE_FAST | PCL_SHUTDOWN_TYPE_NORMAL;

   ret = pclInitLibrary(gTheAppId, shutdownReg);
   x_fail_unless(ret <= 1, "Failed to init PCL");
#if 1
   /**
    * Logical DB ID: 0xFF with user 3 and seat 2
    *       ==> local USER value (user 3, seat 2)
    */
   size = pclKeyGetSize(0xFF, "status/open_document", 3, 2);
   x_fail_unless(size == strlen("WT_ /var/opt/user_manual_climateControl.pdf"), "Invalid size");


   /**
    * Logical DB ID: 0x84 with user 2 and seat 1
    *       ==> shared user value accessible by A GROUP (user 2 and seat 1)
    */
   size = pclKeyGetSize(0x84, "links/last_link", 2, 1);
   x_fail_unless(size == strlen("CACHE_ /last_exit/queens"), "Invalid size");
#endif
   pclDeinitLibrary();
}
END_TEST


/*
 * Delete a key using the key value interface.
 * First read a from a key, the delte the key
 * and then try to read again. The Last read must fail.
 */
START_TEST(test_DeleteData)
{
   X_TEST_REPORT_TEST_NAME("persistence_client_library_test");
   X_TEST_REPORT_COMP_NAME("libpersistence_client_library");
   X_TEST_REPORT_REFERENCE("NONE");
   X_TEST_REPORT_DESCRIPTION("Test of delete data");
   X_TEST_REPORT_TYPE(GOOD);

   int rval = 0;
   unsigned char buffer[READ_SIZE];
   unsigned int shutdownReg = PCL_SHUTDOWN_TYPE_FAST | PCL_SHUTDOWN_TYPE_NORMAL;

   rval = pclInitLibrary(gTheAppId, shutdownReg);
   x_fail_unless(rval <= 1, "Failed to init PCL");
#if 1
   // read data from key
   rval = pclKeyReadData(0xFF, "key_70", 1, 2, buffer, READ_SIZE);
   x_fail_unless(rval != EPERS_NOKEY, "Read form key key_70 fails");

   // delete key
   rval = pclKeyDelete(0xFF, "key_70", 1, 2);
   x_fail_unless(rval >= 0, "Failed to delete key");

   // after deleting the key, reading from key must fail now!
   rval = pclKeyReadData(0xFF, "key_70", 1, 2, buffer, READ_SIZE);
   x_fail_unless(rval == EPERS_NOKEY, "Read form key key_70 works, but should fail");



   // read data from key
   rval = pclKeyReadData(0xFF, "70", 1, 2, buffer, READ_SIZE);
   x_fail_unless(rval != EPERS_NOKEY, "Read form key 70 fails");

   // delete key
   rval = pclKeyDelete(0xFF, "70", 1, 2);
   x_fail_unless(rval >= 0, "Failed to delete key");

   // after deleting the key, reading from key must fail now!
   rval = pclKeyReadData(0xFF, "70", 1, 2, buffer, READ_SIZE);
   x_fail_unless(rval == EPERS_NOKEY, "Read form key 70 works, but should fail");
#endif
   pclDeinitLibrary();
}
END_TEST



// creat blacklist file, if this does not exist
void data_setupBlacklist(void)
{

/// backup info
char gBackupInfo[] = {
"/media/doNotBackupMe.txt_START\n\
/media/doNotBackupMe_01.txt\n\
/media/doNotBackupMe_02.txt\n\
/media/doNotBackupMe_03.txt\n\
/media/doNotBackupMe_04.txt\n\
/media/iDontWantDoBeBackuped_01.txt\n\
/media/iDontWantDoBeBackuped_02.txt\n\
/media/iDontWantDoBeBackuped_03.txt\n\
/media/iDontWantDoBeBackuped_04.txt\n\
/media/iDontWantDoBeBackuped_05.txt_END\n"
};

	const char* backupBlacklist = "/Data/mnt-c/lt-persistence_client_library_test/BackupFileList.info";

	if(access(backupBlacklist, F_OK) == -1)
	{
		int ret = 0;
		int handle = open(backupBlacklist, O_CREAT|O_RDWR|O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);

		ret = write(handle, gBackupInfo, strlen(gBackupInfo));
		if(ret != strlen(gBackupInfo))
		{
			printf("data_setupBlacklist => Wrong size written: %d", ret);
		}
		close(handle);
	}
}


/*
 * Test the file interface:
 * - open file
 * - read / write
 * - remove file
 * - map file
 * - get size
 */
START_TEST(test_DataFile)
{
   X_TEST_REPORT_TEST_NAME("persistence_client_library_test");
   X_TEST_REPORT_COMP_NAME("libpersistence_client_library");
   X_TEST_REPORT_REFERENCE("NONE");
   X_TEST_REPORT_DESCRIPTION("Test of data file");
   X_TEST_REPORT_TYPE(GOOD);

   int fd = 0, i = 0, idx = 0;
   int size = 0, ret = 0, avail = 100;
   int writeSize = 16*1024;
   int fdArray[10] = {0};
   unsigned int shutdownReg = PCL_SHUTDOWN_TYPE_FAST | PCL_SHUTDOWN_TYPE_NORMAL;

   unsigned char buffer[READ_SIZE] = {0};
   unsigned char wBuffer[READ_SIZE] = {0};
   const char* refBuffer = "/Data/mnt-wt/lt-persistence_client_library_test/user/1/seat/1/media";
   char* writeBuffer;
   char* fileMap = NULL;

   ret = pclInitLibrary(gTheAppId, shutdownReg);
   x_fail_unless(ret <= 1, "Failed to init PCL");
#if 1
   writeBuffer = malloc(writeSize);

   // fill buffer a sequence
   for(i = 0; i<(writeSize/8); i++)
   {
      writeBuffer[idx++] = 'A';
      writeBuffer[idx++] = 'B';
      writeBuffer[idx++] = 'C';
      writeBuffer[idx++] = ' ';
      writeBuffer[idx++] = 'D';
      writeBuffer[idx++] = 'E';
      writeBuffer[idx++] = 'F';
      writeBuffer[idx++] = ' ';
   }
   // create file
   fd = open("/Data/mnt-wt/lt-persistence_client_library_test/user/1/seat/1/media/mediaDBWrite.db",
             O_CREAT|O_RDWR|O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
   close(fd);

   // open ------------------------------------------------------------
   fd = pclFileOpen(0xFF, "media/mediaDB.db", 1, 1);
   x_fail_unless(fd != -1, "Could not open file ==> /media/mediaDB.db");


   size = pclFileGetSize(fd);
   x_fail_unless(size == 68, "Wrong file size");


   size = pclFileReadData(fd, buffer, READ_SIZE);
   //printf("pclFileReadData:\n   ist : \"%s\"\n   soll: \"%s\" ==> ret: %d => fd: %d\n", buffer, refBuffer, size, fd);
   x_fail_unless(strncmp((char*)buffer, refBuffer, strlen(refBuffer)) == 0, "Buffer not correctly read => media/mediaDB.db");
   x_fail_unless(size == (strlen(refBuffer)+1), "Wrong size returned");      // strlen + 1 ==> inlcude cr/lf

   ret = pclFileClose(fd);
   x_fail_unless(ret == 0, "Failed to close file");

   // open ------------------------------------------------------------
   fd = pclFileOpen(0xFF, "media/mediaDBWrite.db", 1, 1);
   x_fail_unless(fd != -1, "Could not open file ==> /media/mediaDBWrite.db");

   size = pclFileWriteData(fd, writeBuffer, strlen(writeBuffer));
   x_fail_unless(size == strlen(writeBuffer), "Failed to write data");
   ret = pclFileClose(fd);
   x_fail_unless(ret == 0, "Failed to close file");

   // remove ----------------------------------------------------------
   ret = pclFileRemove(0xFF, "media/mediaDBWrite.db", 1, 1);
   x_fail_unless(ret == 0, "File can't be removed ==> /media/mediaDBWrite.db");

   fd = open("/Data/mnt-wt/lt-persistence_client_library_test/user/1/seat/1/media/mediaDBWrite.db",O_RDWR);
   x_fail_unless(fd == -1, "Failed to remove file, file still exists");
   close(fd);

   // map file --------------------------------------------------------

   fd = pclFileOpen(0xFF, "media/mediaDB.db", 1, 1);

   size = pclFileGetSize(fd);
   pclFileMapData(fileMap, size, 0, fd);
   x_fail_unless(fileMap != MAP_FAILED, "Failed to map file");

   ret = pclFileUnmapData(fileMap, size);
   x_fail_unless(ret != -1, "Failed to unmap file");

   // file seek
   ret = pclFileSeek(fd, 0, SEEK_CUR);
   x_fail_unless(ret == 0, "Failed to seek file - pos 0");

   ret = pclFileSeek(fd, 8, SEEK_CUR);
   x_fail_unless(ret == 8, "Failed to seek file - pos 8");

   // negative test
   size = pclFileGetSize(1024);
   x_fail_unless(size < 0 , "Got size, but should not");
   /*
   ret = pclFileClose(fd);
   x_fail_unless(ret == 0, "Failed to close file");
*/

   // test backup blacklist functionality
   fdArray[0] = pclFileOpen(0xFF, "media/doNotBackupMe.txt_START", 1, 1);
   fdArray[1] = pclFileOpen(0xFF, "media/doNotBackupMe.txt_START", 1, 2);
   fdArray[2] = pclFileOpen(0xFF, "media/doNotBackupMe.txt_START", 20, 10);
   fdArray[3] = pclFileOpen(0xFF, "media/doNotBackupMe.txt_START", 200, 100);

   fdArray[4] = pclFileOpen(0xFF, "media/doNotBackupMe_01.txt", 2, 1);
   fdArray[5] = pclFileOpen(0xFF, "media/doNotBackupMe_02.txt", 2, 1);
   fdArray[6] = pclFileOpen(0xFF, "media/doNotBackupMe_03.txt", 2, 1);
   fdArray[7] = pclFileOpen(0xFF, "media/doNotBackupMe_04.txt", 2, 1);

   fdArray[8] = pclFileOpen(0xFF, "media/iDontWantDoBeBackuped_04.txt", 2, 1);
   fdArray[9] = pclFileOpen(0xFF, "media/iDontWantDoBeBackuped_05.txt_END", 2, 1);

   for(i=0; i<10; i++)
   {
   	snprintf( (char*)wBuffer, 1024, "Test - %d", i);
   	pclFileWriteData(fdArray[i], wBuffer, strlen( (char*)wBuffer));
   }

   //
   // test if backup blacklist works correctly
   //
	avail = access("/Data/mnt-backup/lt-persistence_client_library_test/user/1/seat/1/media/doNotBackupMe.txt_START~", F_OK);
	x_fail_unless(avail == -1, "1. Failed backup => backup available, but should not");


	avail = access("/Data/mnt-backup/lt-persistence_client_library_test/user/1/seat/2/media/doNotBackupMe.txt_START~", F_OK);
	x_fail_unless(avail == -1, "2. Failed backup => backup available, but should not");

	avail = access("/Data/mnt-backup/lt-persistence_client_library_test/user/20/seat/10/media/doNotBackupMe.txt_START~", F_OK);
	x_fail_unless(avail == -1, "3. Failed backup => backup available, but should not");

	avail = access("/Data/mnt-backup/lt-persistence_client_library_test/user/200/seat/100/media/doNotBackupMe.txt_START~", F_OK);
	x_fail_unless(avail == -1, "4. Failed backup => backup available, but should not");

	avail = access("/Data/mnt-backup/lt-persistence_client_library_test/user/2/seat/1/media/doNotBackupMe_01.txt~", F_OK);
	x_fail_unless(avail == -1, "5. Failed backup => backup available, but should not");

	avail = access("/Data/mnt-backup/lt-persistence_client_library_test/user/2/seat/1/media/doNotBackupMe_02.txt~", F_OK);
	x_fail_unless(avail == -1, "6. Failed backup => backup available, but should not");

	avail = access("/Data/mnt-backup/lt-persistence_client_library_test/user/2/seat/1/media/doNotBackupMe_03.txt~", F_OK);
	x_fail_unless(avail == -1, "7. Failed backup => backup available, but should not");

	avail = access("/Data/mnt-backup/lt-persistence_client_library_test/user/2/seat/1/media/doNotBackupMe_04.txt~", F_OK);
	x_fail_unless(avail == -1, "8. Failed backup => backup available, but should not");

	avail = access("/Data/mnt-backup/lt-persistence_client_library_test/user/2/seat/1/media/iDontWantDoBeBackuped_04.txt~", F_OK);
	x_fail_unless(avail == -1, "9. Failed backup => backup available, but should not");

	avail = access("/Data/mnt-backup/lt-persistence_client_library_test/user/2/seat/1/media/iDontWantDoBeBackuped_05.txt_END~", F_OK);
	x_fail_unless(avail == -1, "10. Failed backup => backup available, but should not");

   for(i=0; i<10; i++)
   {
   	pclFileClose(fdArray[i]);
   }

   free(writeBuffer);
#endif
   pclDeinitLibrary();
}
END_TEST



void data_setupBackup(void)
{
	int handle = -1;
	const char* path = "/Data/mnt-c/lt-persistence_client_library_test/user/1/seat/1/media/mediaDB_ReadWrite.db";

   handle = open(path, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
   if(write(handle, gWriteBackupTestData, strlen(gWriteBackupTestData)) == -1)
   {
      printf("setup test: failed to write test data: %s\n", path);
   }
}

START_TEST(test_DataFileBackupCreation)
{
   X_TEST_REPORT_TEST_NAME("persistence_client_library_test");
   X_TEST_REPORT_COMP_NAME("libpersistence_client_library");
   X_TEST_REPORT_REFERENCE("NONE");
   X_TEST_REPORT_DESCRIPTION("Test of file backup creation");
   X_TEST_REPORT_TYPE(GOOD);

   int fd_RW = 0, fd_RO = 0, rval = -1, handle = -1;
   int ret = 0;
   char* wBuffer = " ==> Appended: Test Data - test_DataFileRecovery! ";
   const char* path = "/Data/mnt-backup/lt-persistence_client_library_test/user/1/seat/1/media/mediaDB_ReadWrite.db~";
   char rBuffer[1024] = {0};
   unsigned int shutdownReg = PCL_SHUTDOWN_TYPE_FAST | PCL_SHUTDOWN_TYPE_NORMAL;

   ret = pclInitLibrary(gTheAppId, shutdownReg);
   x_fail_unless(ret <= 1, "Failed to init PCL");
#if 1

   fd_RO = pclFileOpen(0xFF, "media/mediaDB_ReadOnly.db", 1, 1);
   x_fail_unless(fd_RO != -1, "Could not open file ==> /media/mediaDB_ReadOnly.db");

   fd_RW = pclFileOpen(0xFF, "media/mediaDB_ReadWrite.db", 1, 1);
   x_fail_unless(fd_RW != -1, "Could not open file ==> /media/mediaDB_ReadWrite.db");

   rval = pclFileReadData(fd_RW, rBuffer, 10);
   x_fail_unless(rval == 10, "Failed read 10 bytes");
   memset(rBuffer, 0, 1024);

   rval = pclFileReadData(fd_RW, rBuffer, 15);
   x_fail_unless(rval == 15, "Failed read 15 bytes");
   memset(rBuffer, 0, 1024);

   rval = pclFileReadData(fd_RW, rBuffer, 20);
   x_fail_unless(rval == 20, "Failed read 20 bytes");
   memset(rBuffer, 0, 1024);

   rval = pclFileWriteData(fd_RW, wBuffer, strlen(wBuffer));
   x_fail_unless(rval == strlen(wBuffer), "Failed write data");

   // verify the backup creation:
   handle = open(path,  O_RDWR);
   x_fail_unless(handle != -1, "Could not open file ==> failed to access backup file");

   rval = read(handle, rBuffer, 1024);
   //printf(" * * * Backup: \nIst : %s \nSoll: %s\n", rBuffer, gWriteBackupTestData);
   x_fail_unless(strncmp((char*)rBuffer, gWriteBackupTestData, strlen(gWriteBackupTestData)) == 0, "Backup not correctly read");


   (void)close(handle);
   (void)pclFileClose(fd_RW);
   (void)pclFileClose(fd_RO);

#endif

   pclDeinitLibrary();
}
END_TEST



void data_setupRecovery(void)
{
	int i = 0;
   char createPath[128] = {0};

	int handleRecov = -1, handleToBackup = -1, handleToCs = -1;
	char* corruptData = "Some corrupted data ..  )=§?=34=/%&$%&()Ö:ÄNJH/)(";
	const char* pathToRecover  = "/Data/mnt-c/lt-persistence_client_library_test/user/1/seat/1/media/mediaDB_DataRecovery.db";
	const char* pathToBackup   = "/Data/mnt-backup/lt-persistence_client_library_test/user/1/seat/1/media/mediaDB_DataRecovery.db~";
	const char* pathToChecksum = "/Data/mnt-backup/lt-persistence_client_library_test/user/1/seat/1/media/mediaDB_DataRecovery.db~.crc";

   // create directory, even if exist
   snprintf(createPath, 128, "%s", SOURCE_PATH );
   while(gPathSegemnts[i] != NULL)
   {
   	strncat(createPath, gPathSegemnts[i++], 128-1);
   	mkdir(createPath, 0744);
   }

   handleRecov = open(pathToRecover, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
   if(write(handleRecov, corruptData, strlen(corruptData)) == -1)
   {
      printf("setup test: failed to write test data: %s\n", pathToRecover);
   }

   handleToBackup = open(pathToBackup, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
	if(write(handleToBackup, gWriteRecoveryTestData, strlen(gWriteRecoveryTestData)) == -1)
	{
		printf("setup test: failed to write test data: %s\n", pathToBackup);
	}

   handleToCs = open(pathToChecksum, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
	if(write(handleToCs, gRecovChecksum, strlen(gRecovChecksum)) == -1)
	{
		printf("setup test: failed to write test data: %s\n", pathToChecksum);
	}

	close(handleRecov);
	close(handleToBackup);
	close(handleToCs);

}

START_TEST(test_DataFileRecovery)
{
   X_TEST_REPORT_TEST_NAME("persistence_client_library_test");
	X_TEST_REPORT_COMP_NAME("libpersistence_client_library");
	X_TEST_REPORT_REFERENCE("NONE");
	X_TEST_REPORT_DESCRIPTION("Test file recovery form backup");
	X_TEST_REPORT_TYPE(GOOD);

	int handle = 0;
	int ret = 0;
	unsigned char buffer[READ_SIZE] = {0};
	unsigned int shutdownReg = PCL_SHUTDOWN_TYPE_FAST | PCL_SHUTDOWN_TYPE_NORMAL;

	ret = pclInitLibrary(gTheAppId, shutdownReg);
	x_fail_unless(ret <= 1, "Failed to init PCL");


	handle = pclFileOpen(0xFF, "media/mediaDB_DataRecovery.db", 1, 1);
	//printf("pclFileOpen => handle: %d\n", handle);
   x_fail_unless(handle != -1, "Could not open file ==> /media/mediaDB_DataRecovery.db");


	ret = pclFileReadData(handle, buffer, READ_SIZE);
	//printf(" ** pclFileReadData => ist-buffer : %s | size: %d\n", buffer, ret);
	//printf(" ** pclFileReadData => soll-buffer: %s | size: %d\n", gWriteRecoveryTestData, strlen(gWriteRecoveryTestData));
	x_fail_unless(strncmp((char*)buffer, gWriteRecoveryTestData, strlen(gWriteRecoveryTestData)) == 0, "Recovery failed");

   (void)pclFileClose(handle);

   pclDeinitLibrary();

}
END_TEST



/*
 * The the handle function of the key and file interface.
 */
START_TEST(test_DataHandle)
{
   X_TEST_REPORT_TEST_NAME("persistence_client_library_test");
   X_TEST_REPORT_COMP_NAME("libpersistence_client_library");
   X_TEST_REPORT_REFERENCE("NONE");
   X_TEST_REPORT_DESCRIPTION("Test of data handle");
   X_TEST_REPORT_TYPE(GOOD);

   int handle1 = 0, handle2 = 0;
   int handleArray[4] = {0};
   int ret = 0;
   unsigned char buffer[READ_SIZE] = {0};
   unsigned int shutdownReg = PCL_SHUTDOWN_TYPE_FAST | PCL_SHUTDOWN_TYPE_NORMAL;

   ret = pclInitLibrary(gTheAppId, shutdownReg);
   x_fail_unless(ret <= 1, "Failed to init PCL");
#if 1
   // test multiple handles
   handleArray[0] = pclFileOpen(0xFF, "media/mediaDB_write_01.db", 1, 1);
   x_fail_unless(handle1 != -1, "Could not open file ==> /media/mediaDB_write_01.db");

   handleArray[1] = pclFileOpen(0xFF, "media/mediaDB_write_02.db", 1, 1);
   x_fail_unless(handle1 != -1, "Could not open file ==> /media/mediaDB_write_02.db");

   handleArray[2] = pclFileOpen(0xFF, "media/mediaDB_write_03.db", 1, 1);
   x_fail_unless(handle1 != -1, "Could not open file ==> /media/mediaDB_write_03.db");

   handleArray[3] = pclFileOpen(0xFF, "media/mediaDB_write_04.db", 1, 1);
   x_fail_unless(handle1 != -1, "Could not open file ==> /media/mediaDB_write_04.db");

   memset(buffer, 0, READ_SIZE);
   ret = pclFileReadData(handleArray[0], buffer, READ_SIZE);
   x_fail_unless(ret >= 0, "Failed to read handle idx \"0\"!!");
   x_fail_unless(strncmp((char*)buffer, "/user/1/seat/1/media/mediaDB_write_01.db",
         strlen("/user/1/seat/1/media/mediaDB_write_01.db"))
         == 0, "Buffer not correctly read => mediaDB_write_01.db");

   memset(buffer, 0, READ_SIZE);
   ret = pclFileReadData(handleArray[1], buffer, READ_SIZE);
   x_fail_unless(strncmp((char*)buffer, "/user/1/seat/1/media/mediaDB_write_02.db",
         strlen("/user/1/seat/1/media/mediaDB_write_02.db"))
         == 0, "Buffer not correctly read => mediaDB_write_02.db");

   memset(buffer, 0, READ_SIZE);
   ret = pclFileReadData(handleArray[2], buffer, READ_SIZE);
   x_fail_unless(strncmp((char*)buffer, "/user/1/seat/1/media/mediaDB_write_03.db",
         strlen("/user/1/seat/1/media/mediaDB_write_03.db"))
         == 0, "Buffer not correctly read => mediaDB_write_03.db");

   memset(buffer, 0, READ_SIZE);
   (void)pclFileReadData(handleArray[3], buffer, READ_SIZE);
   x_fail_unless(strncmp((char*)buffer, "/user/1/seat/1/media/mediaDB_write_04.db",
         strlen("/user/1/seat/1/media/mediaDB_write_04.db"))
         == 0, "Buffer not correctly read => mediaDB_write_04.db");

   ret = pclKeyHandleClose(handleArray[0]);
   x_fail_unless(ret != -1, "Failed to close handle idx \"0\"!!");

   ret = pclKeyHandleClose(handleArray[1]);
   x_fail_unless(ret != -1, "Failed to close handle idx \"1\"!!");

   ret = pclKeyHandleClose(handleArray[2]);
   x_fail_unless(ret != -1, "Failed to close handle idx \"2\"!!");

   ret = pclKeyHandleClose(handleArray[3]);
   x_fail_unless(ret != -1, "Failed to close handle idx \"3\"!!");

   // test key handles
   handle2 = pclKeyHandleOpen(0xFF, "statusHandle/open_document", 3, 2);
   x_fail_unless(handle2 >= 0, "Failed to open handle /statusHandle/open_document");

   ret = pclKeyHandleClose(handle2);
   x_fail_unless(ret != -1, "Failed to close handle!!");

   ret = pclKeyHandleClose(1024);
   x_fail_unless(ret == EPERS_MAXHANDLE, "Max handle!!");


   // test file handles
	handle1 = pclFileOpen(0xFF, "media/mediaDB.db", 1, 1);
	x_fail_unless(handle1 != -1, "Could not open file ==> /media/mediaDB.db");

	ret = pclFileClose(handle1);
	x_fail_unless(handle1 != -1, "Could not closefile ==> /media/mediaDB.db");

	ret = pclFileClose(1024);
	x_fail_unless(ret == EPERS_MAXHANDLE, "Could close file, but should not!!");

	ret = pclFileClose(19);
	x_fail_unless(ret == -1, "Could close file, but should not!!");

#endif
   pclDeinitLibrary();
}
END_TEST



/*
 * Extended key handle test.
 * Test have been created after a bug in the key handle function occured.
 */
START_TEST(test_DataHandleOpen)
{
   X_TEST_REPORT_TEST_NAME("persistence_client_library_test");
   X_TEST_REPORT_COMP_NAME("libpersistence_client_library");
   X_TEST_REPORT_REFERENCE("NONE");
   X_TEST_REPORT_DESCRIPTION("Test of data handle open");
   X_TEST_REPORT_TYPE(GOOD);

   int hd1 = -2, hd2 = -2, hd3 = -2, hd4 = -2, hd5 = -2, hd6 = -2, hd7 = -2, hd8 = -2, hd9 = -2, ret = 0;
   unsigned int shutdownReg = PCL_SHUTDOWN_TYPE_FAST | PCL_SHUTDOWN_TYPE_NORMAL;

   ret = pclInitLibrary(gTheAppId, shutdownReg);
   x_fail_unless(ret <= 1, "Failed to init PCL");
#if 1
   // open handles ----------------------------------------------------
   hd1 = pclKeyHandleOpen(0xFF, "posHandle/last_position1", 0, 0);
   x_fail_unless(hd1 == 1, "Failed to open handle ==> /posHandle/last_position1");

   hd2 = pclKeyHandleOpen(0xFF, "posHandle/last_position2", 0, 0);
   x_fail_unless(hd2 == 2, "Failed to open handle ==> /posHandle/last_position2");

   hd3 = pclKeyHandleOpen(0xFF, "posHandle/last_position3", 0, 0);
   x_fail_unless(hd3 == 3, "Failed to open handle ==> /posHandle/last_position3");

   // close handles ---------------------------------------------------
   ret = pclKeyHandleClose(hd1);
   x_fail_unless(ret != -1, "Failed to close handle!!");

   ret = pclKeyHandleClose(hd2);
   x_fail_unless(ret != -1, "Failed to close handle!!");

   ret = pclKeyHandleClose(hd3);
   x_fail_unless(ret != -1, "Failed to close handle!!");

   // open handles ----------------------------------------------------
   hd4 = pclKeyHandleOpen(0xFF, "posHandle/last_position4", 0, 0);
   x_fail_unless(hd4 == 3, "Failed to open handle ==> /posHandle/last_position4");

   hd5 = pclKeyHandleOpen(0xFF, "posHandle/last_position5", 0, 0);
   x_fail_unless(hd5 == 2, "Failed to open handle ==> /posHandle/last_position5");

   hd6 = pclKeyHandleOpen(0xFF, "posHandle/last_position6", 0, 0);
   x_fail_unless(hd6 == 1, "Failed to open handle ==> /posHandle/last_position6");

   hd7 = pclKeyHandleOpen(0xFF, "posHandle/last_position7", 0, 0);
   x_fail_unless(hd7 == 4, "Failed to open handle ==> /posHandle/last_position7");

   hd8 = pclKeyHandleOpen(0xFF, "posHandle/last_position8", 0, 0);
   x_fail_unless(hd8 == 5, "Failed to open handle ==> /posHandle/last_position8");

   hd9 = pclKeyHandleOpen(0xFF, "posHandle/last_position9", 0, 0);
   x_fail_unless(hd9 == 6, "Failed to open handle ==> /posHandle/last_position9");

   // close handles ---------------------------------------------------
   ret = pclKeyHandleClose(hd4);
   x_fail_unless(ret != -1, "Failed to close handle!!");

   ret = pclKeyHandleClose(hd5);
   x_fail_unless(ret != -1, "Failed to close handle!!");

   ret = pclKeyHandleClose(hd6);
   x_fail_unless(ret != -1, "Failed to close handle!!");

   ret = pclKeyHandleClose(hd7);
   x_fail_unless(ret != -1, "Failed to close handle!!");

   ret = pclKeyHandleClose(hd8);
   x_fail_unless(ret != -1, "Failed to close handle!!");

   ret = pclKeyHandleClose(hd9);
   x_fail_unless(ret != -1, "Failed to close handle!!");
#endif
   pclDeinitLibrary();
}
END_TEST



START_TEST(test_Plugin)
{
   X_TEST_REPORT_TEST_NAME("persistence_client_library_test");
   X_TEST_REPORT_COMP_NAME("libpersistence_client_library");
   X_TEST_REPORT_REFERENCE("NONE");
   X_TEST_REPORT_DESCRIPTION("Test of plugins");
   X_TEST_REPORT_TYPE(GOOD);

	int ret = 0;
	unsigned char buffer[READ_SIZE]  = {0};

	unsigned int shutdownReg = PCL_SHUTDOWN_TYPE_FAST | PCL_SHUTDOWN_TYPE_NORMAL;

   ret = pclInitLibrary(gTheAppId, shutdownReg);
   x_fail_unless(ret <= 1, "Failed to init PCL");

#if 1

	ret = pclKeyReadData(0xFF, "secured",           0, 0, buffer, READ_SIZE);
	//printf("B U F F E R - secure: \"%s\" => ist: %d | soll: %d\n", buffer, ret, strlen("Custom plugin -> plugin_get_data: secure!"));
	x_fail_unless(ret == strlen("Custom plugin -> plugin_get_data: secure!") );
   x_fail_unless(strncmp((char*)buffer,"Custom plugin -> plugin_get_data: secure!",
                 strlen((char*)buffer)) == 0, "Buffer SECURE not correctly read");
	memset(buffer, 0, READ_SIZE);

	ret = pclKeyReadData(0xFF, "early",     0, 0, buffer, READ_SIZE);
	//printf("B U F F E R - early: \"%s\" => ist: %d | soll: %d\n", buffer, ret, strlen("Custom plugin -> plugin_get_data: early!"));
	x_fail_unless(ret == strlen("Custom plugin -> plugin_get_data: early!"));
   x_fail_unless(strncmp((char*)buffer,"Custom plugin -> plugin_get_data: early!",
               strlen((char*)buffer)) == 0, "Buffer EARLY not correctly read");
	memset(buffer, 0, READ_SIZE);

	ret = pclKeyReadData(0xFF, "emergency", 0, 0, buffer, READ_SIZE);
	//printf("B U F F E R - emergency: \"%s\" => ist: %d | soll: %d\n", buffer, ret, strlen("Custom plugin -> plugin_get_data: emergency!"));
	x_fail_unless(ret == strlen("Custom plugin -> plugin_get_data: emergency!"));
   x_fail_unless(strncmp((char*)buffer,"Custom plugin -> plugin_get_data: emergency!",
               strlen((char*)buffer)) == 0, "Buffer EMERGENCY not correctly read");
	memset(buffer, 0, READ_SIZE);

	ret = pclKeyReadData(0xFF, "hwinfo",   0, 0, buffer, READ_SIZE);
	//printf("B U F F E R - hwinfo: \"%s\" => ist: %d | soll: %d\n", buffer, ret, strlen("Custom plugin -> plugin_get_data: hwinfo!"));
	x_fail_unless(ret != EPERS_NOT_INITIALIZED);
   x_fail_unless(strncmp((char*)buffer,"Custom plugin -> plugin_get_data: hwinfo!",
               strlen((char*)buffer)) == 0, "Buffer HWINFO not correctly read");
	memset(buffer, 0, READ_SIZE);

   ret = pclKeyReadData(0xFF, "custom2",   0, 0, buffer, READ_SIZE);
   //printf("B U F F E R - custom2: \"%s\" => ist: %d | soll: %d\n", buffer, ret, strlen("Custom plugin -> plugin_get_data: custom2!"));
   x_fail_unless(ret == strlen("Custom plugin -> plugin_get_data: custom2!"));
   x_fail_unless(strncmp((char*)buffer,"Custom plugin -> plugin_get_data: custom2!",
               strlen((char*)buffer)) == 0, "Buffer CUSTOM 2 not correctly read");
   memset(buffer, 0, READ_SIZE);

   ret = pclKeyReadData(0xFF, "custom3",   0, 0, buffer, READ_SIZE);
   //printf("B U F F E R - custom3: \"%s\" => ist: %d | soll: %d\n", buffer, ret, strlen("Custom plugin -> plugin_get_data: custom3!"));
   x_fail_unless(ret == strlen("Custom plugin -> plugin_get_data: custom3!"));
   x_fail_unless(strncmp((char*)buffer,"Custom plugin -> plugin_get_data: custom3!",
                 strlen((char*)buffer)) == 0, "Buffer CUSTOM 3 not correctly read");
   memset(buffer, 0, READ_SIZE);

   ret = pclKeyWriteData(0xFF, "custom3",   0, 0, (unsigned char*)"This is a message to write", READ_SIZE);
   x_fail_unless(ret == 321456, "Failed to write custom data");	// plugin should return 321456


   ret = pclKeyGetSize(0xFF, "custom3",   0, 0);
   x_fail_unless(ret == 44332211, "Failed query custom data size");	// plugin should return 44332211


   ret = pclKeyDelete(0xFF, "custom3",   0, 0);
   x_fail_unless(ret == 13579, "Failed query custom data size");	// plugin should return 13579

#endif
	pclDeinitLibrary();
}
END_TEST





START_TEST(test_ReadDefault)
{
   X_TEST_REPORT_TEST_NAME("persistence_client_library_test");
   X_TEST_REPORT_COMP_NAME("libpersistence_client_library");
   X_TEST_REPORT_REFERENCE("NONE");
   X_TEST_REPORT_DESCRIPTION("Test of read default");
   X_TEST_REPORT_TYPE(GOOD);

   int ret = 0;
   unsigned char buffer[READ_SIZE]  = {0};

   unsigned int shutdownReg = PCL_SHUTDOWN_TYPE_FAST | PCL_SHUTDOWN_TYPE_NORMAL;

   ret = pclInitLibrary(gTheAppId, shutdownReg);
   x_fail_unless(ret <= 1, "Failed to init PCL");
#if 1
   ret = pclKeyReadData(0xFF, "statusHandle/default01", 3, 2, buffer, READ_SIZE);
   //printf(" --- test_ReadConfDefault => statusHandle/default01: %s => retIst: %d retSoll: %d\n", buffer, ret, strlen("DEFAULT_01!"));
   x_fail_unless(ret == strlen("DEFAULT_01!"));
   x_fail_unless(strncmp((char*)buffer,"DEFAULT_01!", strlen((char*)buffer)) == 0, "Buffer not correctly read");

   ret = pclKeyReadData(0xFF, "statusHandle/default02", 3, 2, buffer, READ_SIZE);
   //printf(" --- test_ReadConfDefault => statusHandle/default02: %s => retIst: %d retSoll: %d\n", buffer, ret, strlen("DEFAULT_02!"));
   x_fail_unless(ret == strlen("DEFAULT_02!"));
   x_fail_unless(strncmp((char*)buffer,"DEFAULT_02!", strlen((char*)buffer)) == 0, "Buffer not correctly read");

   ret = pclKeyGetSize(0xFF, "statusHandle/default01", 3, 2);
   x_fail_unless(ret == strlen("DEFAULT_01!"), "Invalid size");

#endif
   pclDeinitLibrary();
}
END_TEST



START_TEST(test_ReadConfDefault)
{
   X_TEST_REPORT_TEST_NAME("persistence_client_library_test");
   X_TEST_REPORT_COMP_NAME("libpersistence_client_library");
   X_TEST_REPORT_REFERENCE("NONE");
   X_TEST_REPORT_DESCRIPTION("Test of configurable default data");
   X_TEST_REPORT_TYPE(GOOD);

   int ret = 0;
   unsigned char buffer[READ_SIZE]  = {0};

   unsigned int shutdownReg = PCL_SHUTDOWN_TYPE_FAST | PCL_SHUTDOWN_TYPE_NORMAL;

   ret = pclInitLibrary(gTheAppId, shutdownReg);
   x_fail_unless(ret <= 1, "Failed to init PCL");
#if 1
   ret = pclKeyReadData(0xFF, "statusHandle/confdefault01",     3, 2, buffer, READ_SIZE);
   //printf(" --- test_ReadConfDefault => statusHandle/confdefault01: %s => retIst: %d retSoll: %d\n", buffer, ret, strlen("CONF_DEFAULT_01!"));
   x_fail_unless(ret == strlen("CONF_DEFAULT_01!"));
   x_fail_unless(strncmp((char*)buffer,"CONF_DEFAULT_01!", strlen((char*)buffer)) == 0, "Buffer not correctly read");

   ret = pclKeyReadData(0xFF, "statusHandle/confdefault02",     3, 2, buffer, READ_SIZE);
   //printf(" --- test_ReadConfDefault => statusHandle/confdefault02: %s => retIst: %d retSoll: %d\n", buffer, ret, strlen("CONF_DEFAULT_02!"));
   x_fail_unless(ret == strlen("CONF_DEFAULT_02!"));
   x_fail_unless(strncmp((char*)buffer,"CONF_DEFAULT_02!", strlen((char*)buffer)) == 0, "Buffer not correctly read");

   ret = pclKeyGetSize(0xFF, "statusHandle/confdefault02", 3, 2);
   x_fail_unless(ret == strlen("CONF_DEFAULT_02!"), "Invalid size");

#endif
   pclDeinitLibrary();
}
END_TEST



START_TEST(test_GetPath)
{
   X_TEST_REPORT_TEST_NAME("persistence_client_library_test");
   X_TEST_REPORT_COMP_NAME("libpersistence_client_library");
   X_TEST_REPORT_REFERENCE("NONE");
   X_TEST_REPORT_DESCRIPTION("Test of get path");
   X_TEST_REPORT_TYPE(GOOD);

   int ret = 0;
   char* path = NULL;
   const char* thePath = "/Data/mnt-c/lt-persistence_client_library_test/user/1/seat/1/media/mediaDB_create.db";
   unsigned int pathSize = 0;

   unsigned int shutdownReg = PCL_SHUTDOWN_TYPE_FAST | PCL_SHUTDOWN_TYPE_NORMAL;

   ret = pclInitLibrary(gTheAppId, shutdownReg);
   x_fail_unless(ret <= 1, "Failed to init PCL");
#if 1
   ret = pclFileCreatePath(0xFF, "media/mediaDB_create.db", 1, 1, &path, &pathSize);

   x_fail_unless(strncmp((char*)path, thePath, strlen((char*)path)) == 0, "Path not correct");
   x_fail_unless(pathSize == strlen((char*)path), "Path size not correct");

   pclFileReleasePath(ret);
#endif
   pclDeinitLibrary();
}
END_TEST



START_TEST(test_InitDeinit)
{
   int i = 0, rval = -1;
	unsigned int shutdownReg = PCL_SHUTDOWN_TYPE_FAST | PCL_SHUTDOWN_TYPE_NORMAL;

   for(i=1; i<20; i++)
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
   x_fail_unless(rval == EPERS_SHUTDOWN_NO_PERMIT, "Lifecycle set allowed, but should not");
   pclDeinitLibrary();


   pclInitLibrary(gTheAppId, PCL_SHUTDOWN_TYPE_NONE);

   rval = pclLifecycleSet(PCL_SHUTDOWN);
   x_fail_unless(rval != EPERS_SHUTDOWN_NO_PERMIT, "Lifecycle set NOT allowed, but should");


   rval = pclLifecycleSet(PCL_SHUTDOWN_CANEL);
   rval = pclLifecycleSet(PCL_SHUTDOWN_CANEL);
   rval = pclLifecycleSet(PCL_SHUTDOWN_CANEL);
   rval = pclLifecycleSet(PCL_SHUTDOWN_CANEL);
   rval = pclLifecycleSet(PCL_SHUTDOWN_CANEL);
   rval = pclLifecycleSet(PCL_SHUTDOWN_CANEL);
   //EPERS_COMMON

   pclDeinitLibrary();

}
END_TEST



START_TEST(test_NegHandle)
{
   int handle = -1, ret = 0;
   int negativeHandle = -17;
   unsigned int shutdownReg = PCL_SHUTDOWN_TYPE_FAST | PCL_SHUTDOWN_TYPE_NORMAL;

   unsigned char buffer[128] = {0};

   (void)pclInitLibrary(gTheAppId, shutdownReg);

   handle = pclKeyHandleOpen(0xFF, "posHandle/last_position", 0, 0);
   x_fail_unless(handle >= 0, "Failed to open handle ==> /posHandle/last_position");

   ret = pclKeyHandleReadData(negativeHandle, buffer, READ_SIZE);
   x_fail_unless(ret == EPERS_MAXHANDLE, "pclKeyHandleReadData => negative handle not detected");

   ret = pclKeyHandleClose(negativeHandle);
   x_fail_unless(ret == EPERS_MAXHANDLE, "pclKeyHandleClose => negative handle not detected");

   ret = pclKeyHandleGetSize(negativeHandle);
   x_fail_unless(ret == EPERS_MAXHANDLE, "pclKeyHandleGetSize => negative handle not detected");

   ret = pclKeyHandleReadData(negativeHandle, buffer, 128);
   x_fail_unless(ret == EPERS_MAXHANDLE, "pclKeyHandleReadData => negative handle not detected");

   ret = pclKeyHandleRegisterNotifyOnChange(negativeHandle, &myChangeCallback);
   x_fail_unless(ret == EPERS_MAXHANDLE, "pclKeyHandleRegisterNotifyOnChange => negative handle not detected");

   ret = pclKeyHandleWriteData(negativeHandle, (unsigned char*)"Whatever", strlen("Whatever"));
   x_fail_unless(ret == EPERS_MAXHANDLE, "pclKeyHandleWriteData => negative handle not detected");


   // close handle
   ret = pclKeyHandleClose(handle);

   pclDeinitLibrary();
}
END_TEST



START_TEST(test_utf8_string)
{
	int ret = 0, size = 0;
   unsigned int shutdownReg = PCL_SHUTDOWN_TYPE_FAST | PCL_SHUTDOWN_TYPE_NORMAL;
   const char* utf8StringBuffer = "String °^° Ñ text";
   unsigned char buffer[128] = {0};

   (void)pclInitLibrary(gTheAppId, shutdownReg);

   ret = pclKeyReadData(0xFF, "utf8String", 3, 2, buffer, READ_SIZE);
   x_fail_unless(ret == strlen(utf8StringBuffer), "Wrong read size");
   x_fail_unless(strncmp((char*)buffer, utf8StringBuffer, ret-1) == 0, "Buffer not correctly read => 1");

   size = pclKeyGetSize(0xFF, "utf8String", 3, 2);
   x_fail_unless(size == strlen(utf8StringBuffer), "Invalid size");

   pclDeinitLibrary();
}
END_TEST



START_TEST(test_Notifications)
{
	unsigned int shutdownReg = PCL_SHUTDOWN_TYPE_FAST | PCL_SHUTDOWN_TYPE_NORMAL;

	(void)pclInitLibrary(gTheAppId, shutdownReg);

	pclKeyRegisterNotifyOnChange(0x20, "address/home_address", 1, 1, myChangeCallback);
	pclKeyUnRegisterNotifyOnChange(0x20, "address/home_address", 1, 1, myChangeCallback);

	pclDeinitLibrary();
}
END_TEST



static Suite * persistencyClientLib_suite()
{
   Suite * s  = suite_create("Persistency client library");

   TCase * tc_persGetData = tcase_create("GetData");
   tcase_add_test(tc_persGetData, test_GetData);
   tcase_set_timeout(tc_persGetData, 1);

   TCase * tc_persSetData = tcase_create("SetData");
   tcase_add_test(tc_persSetData, test_SetData);
   tcase_set_timeout(tc_persSetData, 1);

   TCase * tc_persSetDataNoPRCT = tcase_create("SetDataNoPRCT");
   tcase_add_test(tc_persSetDataNoPRCT, test_SetDataNoPRCT);
   tcase_set_timeout(tc_persSetDataNoPRCT, 1);

   TCase * tc_persGetDataSize = tcase_create("GetDataSize");
   tcase_add_test(tc_persGetDataSize, test_GetDataSize);
   tcase_set_timeout(tc_persGetDataSize, 1);

   TCase * tc_persDeleteData = tcase_create("DeleteData");
   tcase_add_test(tc_persDeleteData, test_DeleteData);
   tcase_set_timeout(tc_persDeleteData, 1);

   TCase * tc_persGetDataHandle = tcase_create("GetDataHandle");
   tcase_add_test(tc_persGetDataHandle, test_GetDataHandle);
   tcase_set_timeout(tc_persGetDataHandle, 1);

   TCase * tc_persDataHandle = tcase_create("DataHandle");
   tcase_add_test(tc_persDataHandle, test_DataHandle);
   tcase_set_timeout(tc_persGetData, 1);

   TCase * tc_persDataHandleOpen = tcase_create("DataHandleOpen");
   tcase_add_test(tc_persDataHandleOpen, test_DataHandleOpen);
   tcase_set_timeout(tc_persDataHandleOpen, 1);

   TCase * tc_persDataFile = tcase_create("DataFile");
   tcase_add_test(tc_persDataFile, test_DataFile);
   tcase_set_timeout(tc_persDataFile, 1);

   TCase * tc_persDataFileBackupCreation = tcase_create("DataFileBackupCreation");
   tcase_add_test(tc_persDataFileBackupCreation, test_DataFileBackupCreation);
   tcase_set_timeout(tc_persDataFileBackupCreation, 1);

   TCase * tc_persDataFileRecovery = tcase_create("DataFileRecovery");
   tcase_add_test(tc_persDataFileRecovery, test_DataFileRecovery);
   tcase_set_timeout(tc_persDataFileRecovery, 1);

   TCase * tc_Plugin = tcase_create("Plugin");
   tcase_add_test(tc_Plugin, test_Plugin);
   tcase_set_timeout(tc_Plugin, 1);

   TCase * tc_ReadDefault = tcase_create("ReadDefault");
   tcase_add_test(tc_ReadDefault, test_ReadDefault);
   tcase_set_timeout(tc_ReadDefault, 1);

   TCase * tc_ReadConfDefault = tcase_create("ReadConfDefault");
   tcase_add_test(tc_ReadConfDefault, test_ReadConfDefault);
   tcase_set_timeout(tc_ReadConfDefault, 1);

   TCase * tc_GetPath = tcase_create("GetPath");
   tcase_add_test(tc_GetPath, test_GetPath);
   tcase_set_timeout(tc_GetPath, 1);

   TCase * tc_InitDeinit = tcase_create("InitDeinit");
   tcase_add_test(tc_InitDeinit, test_InitDeinit);
   tcase_set_timeout(tc_InitDeinit, 1);

   TCase * tc_NegHandle = tcase_create("NegHandle");
   tcase_add_test(tc_NegHandle, test_NegHandle);
   tcase_set_timeout(tc_NegHandle, 1);

   TCase * tc_utf8_string = tcase_create("UTF-8");
   tcase_add_test(tc_utf8_string, test_utf8_string);
   tcase_set_timeout(tc_utf8_string, 1);

   TCase * tc_Notifications = tcase_create("Notifications");
   tcase_add_test(tc_Notifications, test_Notifications);
   tcase_set_timeout(tc_Notifications, 1);

   suite_add_tcase(s, tc_persSetData);
   suite_add_tcase(s, tc_persGetData);
   suite_add_tcase(s, tc_persSetDataNoPRCT);
   suite_add_tcase(s, tc_persGetDataSize);
   suite_add_tcase(s, tc_persDeleteData);
   suite_add_tcase(s, tc_persGetDataHandle);
   suite_add_tcase(s, tc_persDataHandle);
   suite_add_tcase(s, tc_persDataHandleOpen);
   suite_add_tcase(s, tc_ReadDefault);
   suite_add_tcase(s, tc_ReadConfDefault);

   suite_add_tcase(s, tc_persDataFile);
   tcase_add_checked_fixture(tc_persDataFile, data_setupBlacklist, data_teardown);

   suite_add_tcase(s, tc_persDataFileBackupCreation);
   tcase_add_checked_fixture(tc_persDataFileBackupCreation, data_setupBackup, data_teardown);

   suite_add_tcase(s, tc_persDataFileRecovery);
   tcase_add_checked_fixture(tc_persDataFileRecovery, data_setupRecovery, data_teardown);

   suite_add_tcase(s, tc_GetPath);
   suite_add_tcase(s, tc_NegHandle);
   suite_add_tcase(s, tc_InitDeinit);
   suite_add_tcase(s, tc_utf8_string);
   suite_add_tcase(s, tc_Notifications);
   suite_add_tcase(s, tc_Plugin);

   return s;
}


int main(int argc, char *argv[])
{
   int nr_failed = 0,
          nr_run = 0,
               i = 0;
   //int fail = 0;

   TestResult** tResult;

   // assign application name
   strncpy(gTheAppId, "lt-persistence_client_library_test", MaxAppNameLen);
   gTheAppId[MaxAppNameLen-1] = '\0';

   /// debug log and trace (DLT) setup
   DLT_REGISTER_APP("PCLt","tests the persistence client library");

#if 1
   Suite * s = persistencyClientLib_suite();
   SRunner * sr = srunner_create(s);
   srunner_set_xml(sr, "/tmp/persistenceClientLibraryTest.xml");
   srunner_set_log(sr, "/tmp/persistenceClientLibraryTest.log");
   srunner_run_all(sr, CK_VERBOSE /*CK_NORMAL CK_VERBOSE*/);

   nr_failed = srunner_ntests_failed(sr);
   nr_run = srunner_ntests_run(sr);

   tResult = srunner_results(sr);
   for(i = 0; i< nr_run; i++)
   {
      (void)tr_rtype(tResult[i]);  // get status of each test
      //fail = tr_rtype(tResult[i]);  // get status of each test
      //printf("[%d] Fail: %d \n", i, fail);
   }

   srunner_free(sr);
#endif

   // unregister debug log and trace
   DLT_UNREGISTER_APP();

   dlt_free();

   return (0==nr_failed)?EXIT_SUCCESS:EXIT_FAILURE;

}

