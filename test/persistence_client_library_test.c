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
 * @ingroup        Persistence client library test
 * @author         Ingo Huerner
 * @brief          Test of persistence client library
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

#include "../include/persistence_client_library_key.h"
#include "../include/persistence_client_library_file.h"
#include "../include/persistence_client_library_error_def.h"

// protected header, should be used only be persistence components
#include "../include_protected/persistence_client_library_db_access.h"


#define BUF_SIZE     64
#define NUM_OF_FILES 3
#define READ_SIZE    1024


char* dayOfWeek[] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};



START_TEST (test_GetData)
{
   int ret = 0;
   unsigned char buffer[READ_SIZE];

   memset(buffer, 0, READ_SIZE);
   ret = key_read_data(0xFF, "language/country_code",         0, 0, buffer, READ_SIZE);
   fail_unless(strncmp((char*)buffer, "Custom plugin -> plugin_get_data_handle",
               strlen((char*)buffer)) == 0, "Buffer not correctly read");
   fail_unless(ret = strlen("Custom plugin -> plugin_get_data_handle"));

   memset(buffer, 0, READ_SIZE);
   ret = key_read_data(0xFF, "pos/last_position",         0, 0, buffer, READ_SIZE);
   fail_unless(strncmp((char*)buffer, "CACHE_ +48° 10' 38.95\", +8° 44' 39.06\"",
               strlen((char*)buffer)) == 0, "Buffer not correctly read");
   fail_unless(ret = strlen("CACHE_ +48° 10' 38.95\", +8° 44' 39.06\""));

   memset(buffer, 0, READ_SIZE);
   ret = key_read_data(0,    "language/current_language", 3, 0, buffer, READ_SIZE);
   fail_unless(strncmp((char*)buffer, "CACHE_ Kisuaheli", strlen((char*)buffer)) == 0, "Buffer not correctly read");

   memset(buffer, 0, READ_SIZE);
   ret = key_read_data(0xFF, "status/open_document",      3, 2, buffer, READ_SIZE);
   fail_unless(strncmp((char*)buffer, "WT_ /var/opt/user_manual_climateControl.pdf", strlen((char*)buffer)) == 0, "Buffer not correctly read");

   memset(buffer, 0, READ_SIZE);
   ret = key_read_data(0x20, "address/home_address",      4, 0, buffer, READ_SIZE);
   fail_unless(strncmp((char*)buffer, "WT_ 55327 Heimatstadt, Wohnstrasse 31", strlen((char*)buffer)) == 0, "Buffer not correctly read");

   memset(buffer, 0, READ_SIZE);
   ret = key_read_data(0xFF, "pos/last_satellites",       0, 0, buffer, READ_SIZE);
   fail_unless(strncmp((char*)buffer, "WT_ 17", strlen((char*)buffer)) == 0, "Buffer not correctly read");

   memset(buffer, 0, READ_SIZE);
   ret = key_read_data(0x84, "links/last_link",           2, 0, buffer, READ_SIZE);
   fail_unless(strncmp((char*)buffer, "CACHE_ /last_exit/brooklyn", strlen((char*)buffer)) == 0, "Buffer not correctly read");

   memset(buffer, 0, READ_SIZE);
   ret = key_read_data(0x84, "links/last_link",           2, 1, buffer, READ_SIZE);
   fail_unless(strncmp((char*)buffer, "CACHE_ /last_exit/queens", strlen((char*)buffer)) == 0, "Buffer not correctly read");
}
END_TEST



START_TEST (test_GetDataHandle)
{
   int ret = 0, handle = 0, handle2 = 0, handle3 = 0, handle4 = 0, size = 0;
   unsigned char buffer[READ_SIZE];
   struct tm *locTime;
   time_t t = time(0);

   char sysTimeBuffer[128];
   memset(buffer, 0, READ_SIZE);

   locTime = localtime(&t);

   snprintf(sysTimeBuffer, 128, "TimeAndData: \"%s %d.%d.%d - %d:%.2d:%.2d Uhr\"", dayOfWeek[locTime->tm_wday], locTime->tm_mday, locTime->tm_mon, (locTime->tm_year+1900),
                                                                  locTime->tm_hour, locTime->tm_min, locTime->tm_sec);
   // open handle ---------------------------------------------------
   handle = key_handle_open(0xFF, "posHandle/last_position", 0, 0);
   fail_unless(handle >= 0, "Failed to open handle ==> /posHandle/last_position");

   ret = key_handle_read_data(handle, buffer, READ_SIZE);
   fail_unless(strncmp((char*)buffer, "WT_ H A N D L E: +48° 10' 38.95\", +8° 44' 39.06\"", ret-1) == 0, "Buffer not correctly read");

   size = key_handle_get_size(handle);
   fail_unless(size = strlen("WT_ H A N D L E: +48° 10' 38.95\", +8° 44' 39.06\""));


   // open handle ---------------------------------------------------
   handle2 = key_handle_open(0xFF, "statusHandle/open_document", 3, 2);
   fail_unless(handle2 >= 0, "Failed to open handle /statusHandle/open_document");

   size = key_handle_write_data(handle2, (unsigned char*)sysTimeBuffer, strlen(sysTimeBuffer));
   fail_unless(size = strlen(sysTimeBuffer));
   // close
   ret = key_handle_close(handle2);


   // open handle ---------------------------------------------------
   memset(buffer, 0, READ_SIZE);
   handle4 = key_handle_open(0xFF, "language/country_code", 0, 0);
   fail_unless(handle4 >= 0, "Failed to open handle /language/country_code");

   ret = key_handle_read_data(handle4, buffer, READ_SIZE);
   fail_unless(strncmp((char*)buffer, "Custom plugin -> plugin_get_data_handle", -1) == 0, "Buffer not correctly read");

   size = key_handle_get_size(handle4);
   fail_unless(size = strlen("Custom plugin -> plugin_get_data_handle"));

   ret = key_handle_write_data(handle4, (unsigned char*)"Only dummy implementation behind custom library", READ_SIZE);


   // open handle ---------------------------------------------------
   handle3 = key_handle_open(0xFF, "statusHandle/open_document", 3, 2);
   fail_unless(handle3 >= 0, "Failed to open handle /statusHandle/open_document");

   ret = key_handle_read_data(handle3, buffer, READ_SIZE);
   fail_unless(strncmp((char*)buffer, sysTimeBuffer, strlen(sysTimeBuffer)) == 0, "Buffer not correctly read");

   size = key_handle_get_size(handle3);
   fail_unless(size = strlen(sysTimeBuffer));

   // close handle
   ret = key_handle_close(handle);
   ret = key_handle_close(handle3);
   ret = key_handle_close(handle4);


}
END_TEST



START_TEST(test_SetData)
{
   int ret = 0;
   unsigned char buffer[READ_SIZE];
   char write1[READ_SIZE];
   char write2[READ_SIZE];
   char sysTimeBuffer[256];

   struct tm *locTime;
   time_t t = time(0);

   locTime = localtime(&t);
   memset(buffer, 0, READ_SIZE);
   memset(write1, 0, READ_SIZE);
   memset(write2, 0, READ_SIZE);

   // write data
   snprintf(sysTimeBuffer, 128, "\"%s %d.%d.%d - %d:%.2d:%.2d Uhr\"", dayOfWeek[locTime->tm_wday], locTime->tm_mday, locTime->tm_mon, (locTime->tm_year+1900),
                                                                 locTime->tm_hour, locTime->tm_min, locTime->tm_sec);

   ret = key_write_data(0xFF, "69", 1, 2, (unsigned char*)sysTimeBuffer, strlen(sysTimeBuffer));
   fail_unless(ret == strlen(sysTimeBuffer), "Wrong write size");

   snprintf(write1, 128, "%s %s", "/70",  sysTimeBuffer);
   ret = key_write_data(0xFF, "70", 1, 2, (unsigned char*)write1, strlen(write1));
   fail_unless(ret == strlen(write1), "Wrong write size");

   snprintf(write2, 128, "%s %s", "/key_70",  sysTimeBuffer);
   ret = key_write_data(0xFF, "key_70", 1, 2, (unsigned char*)write2, strlen(write2));
   fail_unless(ret == strlen(write2), "Wrong write size");

   // read data again and and verify datat has been written correctly
   memset(buffer, 0, READ_SIZE);
   ret = key_read_data(0xFF, "69", 1, 2, buffer, READ_SIZE);
   fail_unless(strncmp((char*)buffer, sysTimeBuffer, strlen(sysTimeBuffer)) == 0, "Buffer not correctly read");
   fail_unless(ret == strlen(sysTimeBuffer), "Wrong read size");

   memset(buffer, 0, READ_SIZE);
   ret = key_read_data(0xFF, "70", 1, 2, buffer, READ_SIZE);
   fail_unless(strncmp((char*)buffer, write1, strlen(write1)) == 0, "Buffer not correctly read");
   fail_unless(ret == strlen(write1), "Wrong read size");

   memset(buffer, 0, READ_SIZE);
   ret = key_read_data(0xFF, "key_70", 1, 2, buffer, READ_SIZE);
   fail_unless(strncmp((char*)buffer, write2, strlen(write2)) == 0, "Buffer not correctly read");
   fail_unless(ret == strlen(write2), "Wrong read size");

}
END_TEST



START_TEST(test_GetDataSize)
{
   int size = 0;

   size = key_get_size(0xFF, "status/open_document", 3, 2);
   fail_unless(size == strlen("WT_ /var/opt/user_manual_climateControl.pdf"), "Invalid size");

   size = key_get_size(0x84, "links/last_link", 2, 1);
   fail_unless(size == strlen("CACHE_ /last_exit/queens"), "Invalid size");
}
END_TEST



START_TEST(test_DeleteData)
{
   int rval = 0;
   unsigned char buffer[READ_SIZE];

   // delete key
   rval = key_delete(0xFF, "key_70", 1, 2);
   fail_unless(rval == 0, "Failed to delete key");
   // reading from key must fail now
   rval = key_read_data(0xFF, "key_70", 1, 2, buffer, READ_SIZE);
   fail_unless(rval == EPERS_NOKEY, "Read form key key_70 works, but should fail");


   rval = key_delete(0xFF, "70", 1, 2);
   fail_unless(rval == 0, "Failed to delete key");
   rval = key_read_data(0xFF, "70", 1, 2, buffer, READ_SIZE);
   fail_unless(rval == EPERS_NOKEY, "Read form key 70 works, but should fail");
}
END_TEST



START_TEST(test_DataFile)
{
   int fd = 0, i = 0, idx = 0;
   int size = 0, ret = 0;
   int writeSize = 16*1024;
   unsigned char buffer[READ_SIZE];
   const char* refBuffer = "/Data/mnt-wt/lt-persistence_client_library_test/user/1/seat/1/media";
   char* writeBuffer;
   char* fileMap = NULL;
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
   memset(buffer, 0, READ_SIZE);

   // create file
   fd = open("/Data/mnt-wt/lt-persistence_client_library_test/user/1/seat/1/media/mediaDBWrite.db",
             O_CREAT|O_RDWR|O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
   close(fd);

   // open ----------------------------------------------------------
   fd = file_open(0xFF, "media/mediaDB.db", 1, 1);
   fail_unless(fd != -1, "Could not open file ==> /media/mediaDB.db");

   size = file_get_size(fd);
   fail_unless(size == 68, "Wrong file size");

   size = file_read_data(fd, buffer, READ_SIZE);
   fail_unless(strncmp((char*)buffer, refBuffer, strlen(refBuffer)) == 0, "Buffer not correctly read");
   fail_unless(size == (strlen(refBuffer)+1), "Wrong size returned");      // strlen + 1 ==> inlcude cr/lf

   ret = file_close(fd);
   fail_unless(ret == 0, "Failed to close file");


   // open ----------------------------------------------------------
   fd = file_open(0xFF, "media/mediaDBWrite.db", 1, 1);
   fail_unless(fd != -1, "Could not open file ==> /media/mediaDBWrite.db");

   size = file_write_data(fd, writeBuffer, strlen(writeBuffer));
   fail_unless(size == strlen(writeBuffer), "Failed to write data");

   ret = file_close(fd);
   fail_unless(ret == 0, "Failed to close file");


   // remove ----------------------------------------------------------
   ret = file_remove(0xFF, "media/mediaDBWrite.db", 1, 1);
   fail_unless(ret == 0, "File can't be removed ==> /media/mediaDBWrite.db");

   fd = file_open(0xFF, "media/mediaDBWrite.db", 1, 1);
   fail_unless(fd == -1, "File can be opend, but should not ==> /media/mediaDBWrite.db");


   // map file ------------------------------------------------------
   fd = file_open(0xFF, "media/mediaDB.db", 1, 1);

   size = file_get_size(fd);
   file_map_data(fileMap, size, 0, fd);
   fail_unless(fileMap != MAP_FAILED, "Failed to map file");

   ret = file_unmap_data(fileMap, size);
   fail_unless(ret != -1, "Failed to unmap file");

   // negative test
   size = file_get_size(1024);
   fail_unless(ret == 0, "Got size, but should not");


   free(writeBuffer);
}
END_TEST



START_TEST(test_DataHandle)
{
   int handle1 = 0, handle2 = 0;
   int ret = 0;

   // test file handles
   handle1 = file_open(0xFF, "media/mediaDB.db", 1, 1);
   fail_unless(handle1 != -1, "Could not open file ==> /media/mediaDB.db");

   ret = file_close(handle1);
   fail_unless(handle1 != -1, "Could not closefile ==> /media/mediaDB.db");

   ret = file_close(1024);
   fail_unless(ret == -1, "Could close file, but should not!!");

   ret = file_close(17);
   fail_unless(ret == -1, "Could close file, but should not!!");


   // test key handles
   handle2 = key_handle_open(0xFF, "statusHandle/open_document", 3, 2);
   fail_unless(handle2 >= 0, "Failed to open handle /statusHandle/open_document");

   ret = key_handle_close(handle2);
   fail_unless(ret != -1, "Failed to close handle!!");

   ret = key_handle_close(1024);
   fail_unless(ret == -1, "Could close, but should not!!");
}
END_TEST



START_TEST(test_DataHandleOpen)
{
   int hd1 = -2, hd2 = -2, hd3 = -2, hd4 = -2, hd5 = -2, hd6 = -2, hd7 = -2, hd8 = -2, hd9 = -2, ret = 0;

   // open handles ----------------------------------------------------
   hd1 = key_handle_open(0xFF, "posHandle/last_position1", 0, 0);
   fail_unless(hd1 == 1, "Failed to open handle ==> /posHandle/last_position1");

   hd2 = key_handle_open(0xFF, "posHandle/last_position2", 0, 0);
   fail_unless(hd2 == 2, "Failed to open handle ==> /posHandle/last_position2");

   hd3 = key_handle_open(0xFF, "posHandle/last_position3", 0, 0);
   fail_unless(hd3 == 3, "Failed to open handle ==> /posHandle/last_position3");

   // close handles ---------------------------------------------------
   ret = key_handle_close(hd1);
   fail_unless(ret != -1, "Failed to close handle!!");

   ret = key_handle_close(hd2);
   fail_unless(ret != -1, "Failed to close handle!!");

   ret = key_handle_close(hd3);
   fail_unless(ret != -1, "Failed to close handle!!");

   // open handles ----------------------------------------------------
   hd4 = key_handle_open(0xFF, "posHandle/last_position4", 0, 0);
   fail_unless(hd4 == 3, "Failed to open handle ==> /posHandle/last_position4");

   hd5 = key_handle_open(0xFF, "posHandle/last_position5", 0, 0);
   fail_unless(hd5 == 2, "Failed to open handle ==> /posHandle/last_position5");

   hd6 = key_handle_open(0xFF, "posHandle/last_position6", 0, 0);
   fail_unless(hd6 == 1, "Failed to open handle ==> /posHandle/last_position6");

   hd7 = key_handle_open(0xFF, "posHandle/last_position7", 0, 0);
   fail_unless(hd7 == 4, "Failed to open handle ==> /posHandle/last_position7");

   hd8 = key_handle_open(0xFF, "posHandle/last_position8", 0, 0);
   fail_unless(hd8 == 5, "Failed to open handle ==> /posHandle/last_position8");

   hd9 = key_handle_open(0xFF, "posHandle/last_position9", 0, 0);
   fail_unless(hd9 == 6, "Failed to open handle ==> /posHandle/last_position9");

   // close handles ---------------------------------------------------
   ret = key_handle_close(hd4);
   fail_unless(ret != -1, "Failed to close handle!!");

   ret = key_handle_close(hd5);
   fail_unless(ret != -1, "Failed to close handle!!");

   ret = key_handle_close(hd6);
   fail_unless(ret != -1, "Failed to close handle!!");

   ret = key_handle_close(hd7);
   fail_unless(ret != -1, "Failed to close handle!!");

   ret = key_handle_close(hd8);
   fail_unless(ret != -1, "Failed to close handle!!");

   ret = key_handle_close(hd9);
   fail_unless(ret != -1, "Failed to close handle!!");
}
END_TEST



START_TEST(test_Cursor)
{
   int handle = -1, rval = 0, size = 0, handle1 = 0;
   char bufferKeySrc[READ_SIZE];
   char bufferDataSrc[READ_SIZE];
   char bufferKeyDst[READ_SIZE];
   char bufferDataDst[READ_SIZE];

   memset(bufferKeySrc, 0, READ_SIZE);
   memset(bufferDataSrc, 0, READ_SIZE);

   memset(bufferKeyDst, 0, READ_SIZE);
   memset(bufferDataDst, 0, READ_SIZE);

   // create cursor
   handle = pers_db_cursor_create("/Data/mnt-c/lt-persistence_client_library_test/cached.itz");

   fail_unless(handle != -1, "Failed to create cursor!!");

   // create cursor
   handle1 = pers_db_cursor_create("/Data/mnt-c/lt-persistence_client_library_test/wt.itz");

   fail_unless(handle1 != -1, "Failed to create cursor!!");

   do
   {
      memset(bufferKeySrc, 0, READ_SIZE);
      memset(bufferDataSrc, 0, READ_SIZE);
      memset(bufferKeyDst, 0, READ_SIZE);
      memset(bufferDataDst, 0, READ_SIZE);

      // get key
      rval = pers_db_cursor_get_key(handle, bufferKeySrc, 128);
      fail_unless(rval != -1, "Cursor failed to get key!!");
      // get data
      rval = pers_db_cursor_get_data(handle, bufferDataSrc, 128);
      fail_unless(rval != -1, "Cursor failed to get data!!");
      // get size
      size = pers_db_cursor_get_data_size(handle);
      fail_unless(size != -1, "Cursor failed to get size!!");
      //printf("1. Key: %s | Data: %s » Size: %d \n", bufferKeySrc, bufferDataSrc, size);

      // get key
      rval = pers_db_cursor_get_key(handle1, bufferKeyDst, 128);
      fail_unless(rval != -1, "Cursor failed to get key!!");
      // get data
      rval = pers_db_cursor_get_data(handle1, bufferDataDst, 128);
      fail_unless(rval != -1, "Cursor failed to get data!!");
      // get size
      size = pers_db_cursor_get_data_size(handle1);
      fail_unless(size != -1, "Cursor failed to get size!!");
      //printf("  2. Key: %s | Data: %s » Size: %d \n", bufferKeyDst, bufferDataDst, size);
   }
   while( (pers_db_cursor_next(handle) == 0) && (pers_db_cursor_next(handle1) == 0) ); // next cursor

   // destory cursor
   rval = pers_db_cursor_destroy(handle);
   fail_unless(rval != -1, "Failed to destroy cursor!!");

   rval = pers_db_cursor_destroy(handle1);
   fail_unless(rval != -1, "Failed to destroy cursor!!");
}
END_TEST



static Suite * persistencyClientLib_suite()
{
   Suite * s  = suite_create("Persistency client library");

   TCase * tc_persGetData = tcase_create("GetData");
   tcase_add_test(tc_persGetData, test_GetData);

   TCase * tc_persSetData = tcase_create("SetData");
   tcase_add_test(tc_persSetData, test_SetData);

   TCase * tc_persGetDataSize = tcase_create("GetDataSize");
   tcase_add_test(tc_persGetDataSize, test_GetDataSize);

   TCase * tc_persDeleteData = tcase_create("DeleteData");
   tcase_add_test(tc_persDeleteData, test_DeleteData);

   TCase * tc_persGetDataHandle = tcase_create("GetDataHandle");
   tcase_add_test(tc_persGetDataHandle, test_GetDataHandle);

   TCase * tc_persDataHandle = tcase_create("DataHandle");
   tcase_add_test(tc_persDataHandle, test_DataHandle);

   TCase * tc_persDataHandleOpen = tcase_create("DataHandleOpen");
   tcase_add_test(tc_persDataHandleOpen, test_DataHandleOpen);

   TCase * tc_persDataFile = tcase_create("DataFile");
   tcase_add_test(tc_persDataFile, test_DataFile);

   TCase * tc_Cursor = tcase_create("Cursor");
   tcase_add_test(tc_Cursor, test_Cursor);

   suite_add_tcase(s, tc_persGetData);
   suite_add_tcase(s, tc_persSetData);
   suite_add_tcase(s, tc_persGetDataSize);
   suite_add_tcase(s, tc_persDeleteData);
   suite_add_tcase(s, tc_persGetDataHandle);
   suite_add_tcase(s, tc_persDataHandle);
   suite_add_tcase(s, tc_persDataHandleOpen);
   suite_add_tcase(s, tc_persDataFile);
   suite_add_tcase(s, tc_Cursor);

   return s;
}




int main(int argc, char *argv[])
{
   int nr_failed = 0;

   Suite * s = persistencyClientLib_suite();
   SRunner * sr = srunner_create(s);
   srunner_run_all(sr, CK_VERBOSE);
   nr_failed = srunner_ntests_failed(sr);

   srunner_free(sr);
   return (0==nr_failed)?EXIT_SUCCESS:EXIT_FAILURE;

}

