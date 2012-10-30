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


#define BUF_SIZE     64
#define NUM_OF_FILES 3
#define READ_SIZE    1024


char* dayOfWeek[] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};



START_TEST (test_persGetData)
{
   int ret = 0;
   unsigned char buffer[READ_SIZE];

   memset(buffer, 0, READ_SIZE);
   ret = key_read_data(0xFF, "/language/country_code",         0, 0, buffer, READ_SIZE);   // "/Data/mnt-c/Appl-1/cached.gvdb"             => "/Node/pos/last position"
   fail_unless(strncmp((char*)buffer, "Custom plugin -> plugin_get_data_handle", strlen((char*)buffer)) == 0, "Buffer not correctly read");
   fail_unless(ret = strlen("Custom plugin -> plugin_get_data_handle"));

   memset(buffer, 0, READ_SIZE);
   ret = key_read_data(0xFF, "/pos/last_position",         0, 0, buffer, READ_SIZE);   // "/Data/mnt-c/Appl-1/cached.gvdb"             => "/Node/pos/last position"
   fail_unless(strncmp((char*)buffer, "+48° 10' 38.95\", +8° 44' 39.06\"", strlen((char*)buffer)) == 0, "Buffer not correctly read");
   fail_unless(ret = strlen("+48° 10' 38.95\", +8° 44' 39.06\""));

   memset(buffer, 0, READ_SIZE);
   ret = key_read_data(0,    "/language/current_language", 3, 0, buffer, READ_SIZE);   // "/Data/mnt-wt/Shared/Public/wt.dconf"        => "/User/3/language/current_language"
   fail_unless(strncmp((char*)buffer, "S H A R E D   D A T A  => not implemented yet", strlen((char*)buffer)) == 0, "Buffer not correctly read");

   memset(buffer, 0, READ_SIZE);
   ret = key_read_data(0xFF, "/status/open_document",      3, 2, buffer, READ_SIZE);   // "/Data/mnt-c/Appl-1/cached.gvdb"             => "/User/3/Seat/2/status/open_document"
   fail_unless(strncmp((char*)buffer, "/var/opt/user_manual_climateControl.pdf", strlen((char*)buffer)) == 0, "Buffer not correctly read");

   memset(buffer, 0, READ_SIZE);
   ret = key_read_data(0x20, "/address/home_address",      4, 0, buffer, READ_SIZE);   // "/Data/mnt-c/Shared/Group/20/cached.dconf"   => "/User/4/address/home_address"
   fail_unless(strncmp((char*)buffer, "S H A R E D   D A T A  => not implemented yet", strlen((char*)buffer)) == 0, "Buffer not correctly read");

   memset(buffer, 0, READ_SIZE);
   ret = key_read_data(0xFF, "/pos/last satellites",       0, 0, buffer, READ_SIZE);   // "/Data/mnt-wt/Appl-1/wt.gvdb"                => "/Node/pos/last satellites"
   fail_unless(strncmp((char*)buffer, "17", strlen((char*)buffer)) == 0, "Buffer not correctly read");

   memset(buffer, 0, READ_SIZE);
   ret = key_read_data(0x84, "/links/last link",           2, 0, buffer, READ_SIZE);   // "/Data/mnt-wt/Appl-2/wt.gvdb"                => "/84/User/2/links/last link"
   fail_unless(strncmp((char*)buffer, "/last_exit/brooklyn", strlen((char*)buffer)) == 0, "Buffer not correctly read");

   memset(buffer, 0, READ_SIZE);
   ret = key_read_data(0x84, "/links/last link",           2, 1, buffer, READ_SIZE);   // "/Data/mnt-wt/Appl-2/wt.gvdb"                => "/84/User/2/links/last link"
   fail_unless(strncmp((char*)buffer, "/last_exit/queens", strlen((char*)buffer)) == 0, "Buffer not correctly read");
}
END_TEST



START_TEST (test_persGetDataHandle)
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
   handle = key_handle_open(0xFF, "/posHandle/last position", 0, 0);
   fail_unless(handle >= 0, "Failed to open handle ==> /posHandle/last position");

   ret = key_handle_read_data(handle, buffer, READ_SIZE);
   fail_unless(strncmp((char*)buffer, "H A N D L E: +48° 10' 38.95\", +8° 44' 39.06\"", ret-1) == 0, "Buffer not correctly read");

   size = key_handle_get_size(handle);
   fail_unless(size = strlen("H A N D L E: +48° 10' 38.95\", +8° 44' 39.06\""));


   // open handle ---------------------------------------------------
   handle2 = key_handle_open(0xFF, "/statusHandle/open_document", 3, 2);
   fail_unless(handle2 >= 0, "Failed to open handle /statusHandle/open_document");

   size = key_handle_write_data(handle2, (unsigned char*)sysTimeBuffer, strlen(sysTimeBuffer));
   fail_unless(size = strlen(sysTimeBuffer));
   // close
   ret = key_handle_close(handle2);


   // open handle ---------------------------------------------------
   memset(buffer, 0, READ_SIZE);
   handle4 = key_handle_open(0xFF, "/language/country_code", 0, 0);
   fail_unless(handle4 >= 0, "Failed to open handle /language/country_code");

   ret = key_handle_read_data(handle4, buffer, READ_SIZE);
   fail_unless(strncmp((char*)buffer, "Custom plugin -> plugin_get_data_handle", -1) == 0, "Buffer not correctly read");

   size = key_handle_get_size(handle4);
   fail_unless(size = strlen("Custom plugin -> plugin_get_data_handle"));

   ret = key_handle_write_data(handle4, (unsigned char*)"Only dummy implementation behind custom library", READ_SIZE);


   // open handle ---------------------------------------------------
   handle3 = key_handle_open(0xFF, "/statusHandle/open_document", 3, 2);
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



START_TEST(test_persSetData)
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
   ret = key_write_data(0xFF, "/69", 1, 2, (unsigned char*)sysTimeBuffer, strlen(sysTimeBuffer));
   fail_unless(ret == strlen(sysTimeBuffer), "Wrong write size");

   snprintf(write1, 128, "%s %s", "/70",  sysTimeBuffer);
   ret = key_write_data(0xFF, "/70", 1, 2, (unsigned char*)write1, strlen(write1));
   fail_unless(ret == strlen(write1), "Wrong write size");

   snprintf(write2, 128, "%s %s", "/key_70",  sysTimeBuffer);
   ret = key_write_data(0xFF, "/key_70", 1, 2, (unsigned char*)write2, strlen(write2));
   fail_unless(ret == strlen(write2), "Wrong write size");

   // read data again and and verify datat has been written correctly
   memset(buffer, 0, READ_SIZE);
   ret = key_read_data(0xFF, "/69", 1, 2, buffer, READ_SIZE);
   fail_unless(strncmp((char*)buffer, sysTimeBuffer, strlen(sysTimeBuffer)) == 0, "Buffer not correctly read");
   fail_unless(ret == strlen(sysTimeBuffer), "Wrong read size");

   memset(buffer, 0, READ_SIZE);
   ret = key_read_data(0xFF, "/70", 1, 2, buffer, READ_SIZE);
   fail_unless(strncmp((char*)buffer, write1, strlen(write1)) == 0, "Buffer not correctly read");
   fail_unless(ret == strlen(write1), "Wrong read size");

   memset(buffer, 0, READ_SIZE);
   ret = key_read_data(0xFF, "/key_70", 1, 2, buffer, READ_SIZE);
   fail_unless(strncmp((char*)buffer, write2, strlen(write2)) == 0, "Buffer not correctly read");
   fail_unless(ret == strlen(write2), "Wrong read size");

}
END_TEST



START_TEST(test_persGetDataSize)
{
   int size = 0;

   size = key_get_size(0xFF, "/status/open_document", 3, 2);
   fail_unless(size == strlen("/var/opt/user_manual_climateControl.pdf"), "Invalid size");

   size = key_get_size(0x84, "/links/last link", 2, 1);
   fail_unless(size == strlen("/last_exit/queens"), "Invalid size");
}
END_TEST



START_TEST(test_persDeleteData)
{
   int rval = 0;
   unsigned char buffer[READ_SIZE];

   // delete key
   rval = key_delete(0xFF, "/key_70", 1, 2);
   fail_unless(rval == 0, "Failed to delete key");
   // reading from key must fail now
   rval = key_read_data(0xFF, "/key_70", 1, 2, buffer, READ_SIZE);
   fail_unless(rval == EPERS_NOKEY, "Read form key key_70 works, but should fail");


   rval = key_delete(0xFF, "/70", 1, 2);
   fail_unless(rval == 0, "Failed to delete key");
   rval = key_read_data(0xFF, "/70", 1, 2, buffer, READ_SIZE);
   fail_unless(rval == EPERS_NOKEY, "Read form key 70 works, but should fail");
}
END_TEST



START_TEST(test_persDataFile)
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
   fd = file_open(0xFF, "/media/mediaDB.db", 1, 1);
   fail_unless(fd != -1, "Could not open file ==> /media/mediaDB.db");

   size = file_get_size(fd);
   fail_unless(size == 68, "Wrong file size");

   size = file_read_data(fd, buffer, READ_SIZE);
   fail_unless(strncmp((char*)buffer, refBuffer, strlen(refBuffer)) == 0, "Buffer not correctly read");
   fail_unless(size == (strlen(refBuffer)+1), "Wrong size returned");      // strlen + 1 ==> inlcude cr/lf

   ret = file_close(fd);
   fail_unless(ret == 0, "Failed to close file");


   // open ----------------------------------------------------------
   fd = file_open(0xFF, "/media/mediaDBWrite.db", 1, 1);
   fail_unless(fd != -1, "Could not open file ==> /media/mediaDBWrite.db");

   size = file_write_data(fd, writeBuffer, strlen(writeBuffer));
   fail_unless(size == strlen(writeBuffer), "Failed to write data");

   ret = file_close(fd);
   fail_unless(ret == 0, "Failed to close file");


   // remove ----------------------------------------------------------
   ret = file_remove(0xFF, "/media/mediaDBWrite.db", 1, 1);
   fail_unless(ret == 0, "File can't be removed ==> /media/mediaDBWrite.db");

   fd = file_open(0xFF, "/media/mediaDBWrite.db", 1, 1);
   fail_unless(fd == -1, "File can be opend, but should not ==> /media/mediaDBWrite.db");


   // map file ------------------------------------------------------
   fd = file_open(0xFF, "/media/mediaDB.db", 1, 1);

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



START_TEST(test_persDataHandle)
{
   int handle1 = 0, handle2 = 0;
   int ret = 0;

   // test file handles
   handle1 = file_open(0xFF, "/media/mediaDB.db", 1, 1);
   fail_unless(handle1 != -1, "Could not open file ==> /media/mediaDB.db");

   ret = file_close(handle1);
   fail_unless(handle1 != -1, "Could not open file ==> /media/mediaDB.db");

   ret = file_close(1024);
   fail_unless(ret == -1, "Could close file, but should not!!");

   ret = file_close(17);
   fail_unless(ret == -1, "Could close file, but should not!!");


   // test key handles
   handle2 = key_handle_open(0xFF, "/statusHandle/open_document", 3, 2);
   fail_unless(handle2 >= 0, "Failed to open handle /statusHandle/open_document");

   ret = key_handle_close(handle2);
   fail_unless(ret != -1, "Failed to close handle!!");

   ret = key_handle_close(1024);
   fail_unless(ret == -1, "Could close, but should not!!");
}
END_TEST



static Suite * persistencyClientLib_suite()
{
   Suite * s  = suite_create("Persistency client library");

   TCase * tc_persGetData = tcase_create("persGetData");
   tcase_add_test(tc_persGetData, test_persGetData);

   TCase * tc_persSetData = tcase_create("persSetData");
   tcase_add_test(tc_persSetData, test_persSetData);

   TCase * tc_persGetDataSize = tcase_create("persGetDataSize");
   tcase_add_test(tc_persGetDataSize, test_persGetDataSize);

   TCase * tc_persDeleteData = tcase_create("persDeleteData");
   tcase_add_test(tc_persDeleteData, test_persDeleteData);

   TCase * tc_persGetDataHandle = tcase_create("persGetDataHandle");
   tcase_add_test(tc_persGetDataHandle, test_persGetDataHandle);

   TCase * tc_persDataHandle = tcase_create("persDataHandle");
   tcase_add_test(tc_persDataHandle, test_persDataHandle);

   TCase * tc_persDataFile = tcase_create("persDataFile");
   tcase_add_test(tc_persDataFile, test_persDataFile);


   suite_add_tcase(s, tc_persGetData);
   suite_add_tcase(s, tc_persSetData);
   suite_add_tcase(s, tc_persGetDataSize);
   suite_add_tcase(s, tc_persDeleteData);
   suite_add_tcase(s, tc_persGetDataHandle);
   suite_add_tcase(s, tc_persDataHandle);
   suite_add_tcase(s, tc_persDataFile);

   return s;
}




int main(int argc, char *argv[])
{
   int nr_failed;

   Suite * s = persistencyClientLib_suite();
   SRunner * sr = srunner_create(s);
   srunner_run_all(sr, CK_NORMAL);
   nr_failed = srunner_ntests_failed(sr);
   srunner_free(sr);

   return (0==nr_failed)?EXIT_SUCCESS:EXIT_FAILURE;

}

