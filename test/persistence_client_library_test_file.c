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
#include <dbus/dbus.h>
#include <dlt.h>
#include <dlt_common.h>
#include <pthread.h>

#include <check.h>

#include "../include/persistence_client_library_file.h"
#include "../include/persistence_client_library_key.h"
#include "../include/persistence_client_library.h"
#include "../include/persistence_client_library_error_def.h"


#define READ_SIZE       1024
#define MaxAppNameLen   256

#define NUM_THREADS     100
#define NUM_OF_WRITES   350
#define NAME_LEN     24

typedef struct s_threadData
{
   char threadName[NAME_LEN];
   int index;
   int fd1;
   int fd2;
} t_threadData;

static pthread_barrier_t gBarrierOne;

static const char* gPathSegemnts[] = {"user/", "1/", "seat/", "1/", "media", NULL };
static const char* gSourcePath   = "/Data/mnt-c/lt-persistence_client_library_test/";

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
      ssize_t ret = 0;
      int handle = open(backupBlacklist, O_CREAT|O_RDWR|O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);

      ret = write(handle, gBackupInfo, strlen(gBackupInfo));
      if(ret != (int)strlen(gBackupInfo))
      {
         printf("data_setupBlacklist => Wrong size written: %d", (int)ret);
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
   int fd = 0, i = 0, idx = 0;
   int size = 0, ret = 0, avail = 100;
   int writeSize = 16*1024;
   int fdArray[10] = {0};

   unsigned char buffer[READ_SIZE] = {0};
   unsigned char wBuffer[READ_SIZE] = {0};
   const char* refBuffer = "/Data/mnt-wt/lt-persistence_client_library_test/user/1/seat/1/media";
   char* writeBuffer;
   char* fileMap = NULL;

   writeBuffer = malloc((size_t)writeSize);

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
   fd = pclFileOpen(PCL_LDBID_LOCAL, "media/mediaDB.db", 1, 1);
   fail_unless(fd != -1, "Could not open file ==> /media/mediaDB.db");


   size = pclFileGetSize(fd);
   fail_unless(size == 68, "Wrong file size");


   size = pclFileReadData(fd, buffer, READ_SIZE);
   //printf("pclFileReadData:\n   ist : \"%s\"\n   soll: \"%s\" ==> ret: %d => fd: %d\n", buffer, refBuffer, size, fd);
   fail_unless(strncmp((char*)buffer, refBuffer, strlen(refBuffer)) == 0, "Buffer not correctly read => media/mediaDB.db");
   fail_unless(size == ((int)strlen(refBuffer)+1), "Wrong size returned");      // strlen + 1 ==> inlcude cr/lf

   ret = pclFileClose(fd);
   fail_unless(ret == 0, "Failed to close file");

   // open ------------------------------------------------------------
   fd = pclFileOpen(PCL_LDBID_LOCAL, "media/mediaDBWrite.db", 1, 1);
   fail_unless(fd != -1, "Could not open file ==> /media/mediaDBWrite.db");

   size = pclFileWriteData(fd, writeBuffer, (int)strlen(writeBuffer));
   fail_unless(size == (int)strlen(writeBuffer), "Failed to write data");
   ret = pclFileClose(fd);
   fail_unless(ret == 0, "Failed to close file");

   // remove ----------------------------------------------------------
   ret = pclFileRemove(PCL_LDBID_LOCAL, "media/mediaDBWrite.db", 1, 1);
   fail_unless(ret == 0, "File can't be removed ==> /media/mediaDBWrite.db");

   fd = open("/Data/mnt-wt/lt-persistence_client_library_test/user/1/seat/1/media/mediaDBWrite.db",O_RDWR);
   fail_unless(fd == -1, "Failed to remove file, file still exists");
   close(fd);

   // map file --------------------------------------------------------

   fd = pclFileOpen(PCL_LDBID_LOCAL, "media/mediaDB.db", 1, 1);

   size = pclFileGetSize(fd);
   pclFileMapData(fileMap, size, 0, fd);
   fail_unless(fileMap != MAP_FAILED, "Failed to map file");

   ret = pclFileUnmapData(fileMap, size);
   fail_unless(ret != -1, "Failed to unmap file");

   // file seek
   ret = pclFileSeek(fd, 0, SEEK_CUR);
   fail_unless(ret == 0, "Failed to seek file - pos 0");

   ret = pclFileSeek(fd, 8, SEEK_CUR);
   fail_unless(ret == 8, "Failed to seek file - pos 8");

   // negative test
   size = pclFileGetSize(1024);
   fail_unless(size < 0 , "Got size, but should not");

   ret = pclFileClose(fd);
   fail_unless(ret == 0, "Failed to close file");

   // test backup blacklist functionality
   fdArray[0] = pclFileOpen(PCL_LDBID_LOCAL, "media/doNotBackupMe.txt_START", 1, 1);
   fdArray[1] = pclFileOpen(PCL_LDBID_LOCAL, "media/doNotBackupMe.txt_START", 1, 2);
   fdArray[2] = pclFileOpen(PCL_LDBID_LOCAL, "media/doNotBackupMe.txt_START", 20, 10);
   fdArray[3] = pclFileOpen(PCL_LDBID_LOCAL, "media/doNotBackupMe.txt_START", 200, 100);

   fdArray[4] = pclFileOpen(PCL_LDBID_LOCAL, "media/doNotBackupMe_01.txt", 2, 1);
   fdArray[5] = pclFileOpen(PCL_LDBID_LOCAL, "media/doNotBackupMe_02.txt", 2, 1);
   fdArray[6] = pclFileOpen(PCL_LDBID_LOCAL, "media/doNotBackupMe_03.txt", 2, 1);
   fdArray[7] = pclFileOpen(PCL_LDBID_LOCAL, "media/doNotBackupMe_04.txt", 2, 1);

   fdArray[8] = pclFileOpen(PCL_LDBID_LOCAL, "media/iDontWantDoBeBackuped_04.txt", 2, 1);
   fdArray[9] = pclFileOpen(PCL_LDBID_LOCAL, "media/iDontWantDoBeBackuped_05.txt_END", 2, 1);

   for(i=0; i<10; i++)
   {
      snprintf( (char*)wBuffer, 1024, "Test - %d", i);
      pclFileWriteData(fdArray[i], wBuffer, (int)strlen((char*)wBuffer));
   }

   //
   // test if backup blacklist works correctly
   //
   avail = access("/Data/mnt-backup/lt-persistence_client_library_test/user/1/seat/1/media/doNotBackupMe.txt_START~", F_OK);
   fail_unless(avail == -1, "1. Failed backup => backup available, but should not");

   avail = access("/Data/mnt-backup/lt-persistence_client_library_test/user/1/seat/2/media/doNotBackupMe.txt_START~", F_OK);
   fail_unless(avail == -1, "2. Failed backup => backup available, but should not");

   avail = access("/Data/mnt-backup/lt-persistence_client_library_test/user/20/seat/10/media/doNotBackupMe.txt_START~", F_OK);
   fail_unless(avail == -1, "3. Failed backup => backup available, but should not");

   avail = access("/Data/mnt-backup/lt-persistence_client_library_test/user/200/seat/100/media/doNotBackupMe.txt_START~", F_OK);
   fail_unless(avail == -1, "4. Failed backup => backup available, but should not");

   avail = access("/Data/mnt-backup/lt-persistence_client_library_test/user/2/seat/1/media/doNotBackupMe_01.txt~", F_OK);
   fail_unless(avail == -1, "5. Failed backup => backup available, but should not");

   avail = access("/Data/mnt-backup/lt-persistence_client_library_test/user/2/seat/1/media/doNotBackupMe_02.txt~", F_OK);
   fail_unless(avail == -1, "6. Failed backup => backup available, but should not");

   avail = access("/Data/mnt-backup/lt-persistence_client_library_test/user/2/seat/1/media/doNotBackupMe_03.txt~", F_OK);
   fail_unless(avail == -1, "7. Failed backup => backup available, but should not");

   avail = access("/Data/mnt-backup/lt-persistence_client_library_test/user/2/seat/1/media/doNotBackupMe_04.txt~", F_OK);
   fail_unless(avail == -1, "8. Failed backup => backup available, but should not");

   avail = access("/Data/mnt-backup/lt-persistence_client_library_test/user/2/seat/1/media/iDontWantDoBeBackuped_04.txt~", F_OK);
   fail_unless(avail == -1, "9. Failed backup => backup available, but should not");

   avail = access("/Data/mnt-backup/lt-persistence_client_library_test/user/2/seat/1/media/iDontWantDoBeBackuped_05.txt_END~", F_OK);
   fail_unless(avail == -1, "10. Failed backup => backup available, but should not");

   for(i=0; i<10; i++)
   {
      pclFileClose(fdArray[i]);
   }

   // write to file not in RCT
   fd = pclFileOpen(PCL_LDBID_LOCAL, "nonRCT/aNonRctFile.db", 1, 1);
   size = pclFileGetSize(fd);
   size = pclFileWriteData(fd, "nonRCT/mediaDB.db", strlen("nonRCT/mediaDB.db"));

   free(writeBuffer);
}
END_TEST



START_TEST(test_DataFileConfDefault)
{
   int fd = 0;
   char readBuffer[READ_SIZE] = {0};
   char* refBuffer01 = "Some default file content: 01 Configurable default data 01.";
   char* refBuffer02 = "Some default file content: 02 Configurable default data 02.";

   // -- file interface ---
   memset(readBuffer, 0, READ_SIZE);
   fd = pclFileOpen(PCL_LDBID_LOCAL, "media/mediaData_01.configurable", 99, 99);
   (void)pclFileReadData(fd, readBuffer, READ_SIZE);
   fail_unless(strncmp(readBuffer, refBuffer01, strlen(refBuffer01)) == 0, "Buffer not correctly read => mediaData_01.configurable");
   (void)pclFileClose(fd);


   memset(readBuffer, 0, READ_SIZE);
   fd = pclFileOpen(PCL_LDBID_LOCAL, "media/mediaData_02.configurable", 99, 99);
   (void)pclFileReadData(fd, readBuffer, READ_SIZE);
   fail_unless(strncmp(readBuffer, refBuffer02, strlen(refBuffer02)) == 0, "Buffer not correctly read => mediaData_01.configurable");
   (void)pclFileClose(fd);
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

START_TEST(test_DataFileBackupCreation)
{
   int fd_RW = 0, fd_RO = 0, rval = -1, handle = -1;
   char* wBuffer = " ==> Appended: Test Data - test_DataFileRecovery! ";
   const char* path = "/Data/mnt-backup/lt-persistence_client_library_test/user/1/seat/1/media/mediaDB_ReadWrite.db~";
   char rBuffer[1024] = {0};

#if 1

   fd_RO = pclFileOpen(PCL_LDBID_LOCAL, "media/mediaDB_ReadOnly.db", 1, 1);
   fail_unless(fd_RO != -1, "Could not open file ==> /media/mediaDB_ReadOnly.db");

   fd_RW = pclFileOpen(PCL_LDBID_LOCAL, "media/mediaDB_ReadWrite.db", 1, 1);
   fail_unless(fd_RW != -1, "Could not open file ==> /media/mediaDB_ReadWrite.db");

   rval = pclFileReadData(fd_RW, rBuffer, 10);
   fail_unless(rval == 10, "Failed read 10 bytes");
   memset(rBuffer, 0, 1024);

   rval = pclFileReadData(fd_RW, rBuffer, 15);
   fail_unless(rval == 15, "Failed read 15 bytes");
   memset(rBuffer, 0, 1024);

   rval = pclFileReadData(fd_RW, rBuffer, 20);
   fail_unless(rval == 20, "Failed read 20 bytes");
   memset(rBuffer, 0, 1024);

   rval = pclFileWriteData(fd_RW, wBuffer, (int)strlen(wBuffer));
   fail_unless(rval == (int)strlen(wBuffer), "Failed write data");

   // verify the backup creation:
   handle = open(path,  O_RDWR);
   fail_unless(handle != -1, "Could not open file ==> failed to access backup file");

   rval = (int)read(handle, rBuffer, 1024);
   //printf(" * * * Backup: \nIst : %s \nSoll: %s\n", rBuffer, gWriteBackupTestData);
   fail_unless(strncmp((char*)rBuffer, gWriteBackupTestData, strlen(gWriteBackupTestData)) == 0, "Backup not correctly read");


   (void)close(handle);
   (void)pclFileClose(fd_RW);
   (void)pclFileClose(fd_RO);

#endif
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

   int shutdownReg = PCL_SHUTDOWN_TYPE_FAST | PCL_SHUTDOWN_TYPE_NORMAL;
   (void)pclInitLibrary(gTheAppId, shutdownReg);

   // create directory, even if exist
   snprintf(createPath, 128, "%s", gSourcePath );
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
   int handle = 0;
   unsigned char buffer[READ_SIZE] = {0};

   handle = pclFileOpen(PCL_LDBID_LOCAL, "media/mediaDB_DataRecovery.db", 1, 1);
   //printf("pclFileOpen => handle: %d\n", handle);
   fail_unless(handle != -1, "Could not open file ==> /media/mediaDB_DataRecovery.db");


   /*ret = */(void)pclFileReadData(handle, buffer, READ_SIZE);
   //printf(" ** pclFileReadData => ist-buffer : %s | size: %d\n", buffer, ret);
   //printf(" ** pclFileReadData => soll-buffer: %s | size: %d\n", gWriteRecoveryTestData, strlen(gWriteRecoveryTestData));
   fail_unless(strncmp((char*)buffer, gWriteRecoveryTestData, strlen(gWriteRecoveryTestData)) == 0, "Recovery failed");

   (void)pclFileClose(handle);
}
END_TEST



/*
 * The the handle function of the key and file interface.
 */
START_TEST(test_DataHandle)
{
   int handle1 = 0, handle2 = 0;
   int handleArray[4] = {0};
   int ret = 0;
   unsigned char buffer[READ_SIZE] = {0};

   // test multiple handles
   handleArray[0] = pclFileOpen(PCL_LDBID_LOCAL, "media/mediaDB_write_01.db", 1, 1);
   fail_unless(handle1 != -1, "Could not open file ==> /media/mediaDB_write_01.db");

   handleArray[1] = pclFileOpen(PCL_LDBID_LOCAL, "media/mediaDB_write_02.db", 1, 1);
   fail_unless(handle1 != -1, "Could not open file ==> /media/mediaDB_write_02.db");

   handleArray[2] = pclFileOpen(PCL_LDBID_LOCAL, "media/mediaDB_write_03.db", 1, 1);
   fail_unless(handle1 != -1, "Could not open file ==> /media/mediaDB_write_03.db");

   handleArray[3] = pclFileOpen(PCL_LDBID_LOCAL, "media/mediaDB_write_04.db", 1, 1);
   fail_unless(handle1 != -1, "Could not open file ==> /media/mediaDB_write_04.db");

   memset(buffer, 0, READ_SIZE);
   ret = pclFileReadData(handleArray[0], buffer, READ_SIZE);
   fail_unless(ret >= 0, "Failed to read handle idx \"0\"!!");
   fail_unless(strncmp((char*)buffer, "/user/1/seat/1/media/mediaDB_write_01.db",
         strlen("/user/1/seat/1/media/mediaDB_write_01.db"))
         == 0, "Buffer not correctly read => mediaDB_write_01.db");

   memset(buffer, 0, READ_SIZE);
   ret = pclFileReadData(handleArray[1], buffer, READ_SIZE);
   fail_unless(strncmp((char*)buffer, "/user/1/seat/1/media/mediaDB_write_02.db",
         strlen("/user/1/seat/1/media/mediaDB_write_02.db"))
         == 0, "Buffer not correctly read => mediaDB_write_02.db");

   memset(buffer, 0, READ_SIZE);
   ret = pclFileReadData(handleArray[2], buffer, READ_SIZE);
   fail_unless(strncmp((char*)buffer, "/user/1/seat/1/media/mediaDB_write_03.db",
         strlen("/user/1/seat/1/media/mediaDB_write_03.db"))
         == 0, "Buffer not correctly read => mediaDB_write_03.db");

   memset(buffer, 0, READ_SIZE);
   (void)pclFileReadData(handleArray[3], buffer, READ_SIZE);
   fail_unless(strncmp((char*)buffer, "/user/1/seat/1/media/mediaDB_write_04.db",
         strlen("/user/1/seat/1/media/mediaDB_write_04.db"))
         == 0, "Buffer not correctly read => mediaDB_write_04.db");

   ret = pclKeyHandleClose(handleArray[0]);
   fail_unless(ret != -1, "Failed to close handle idx \"0\"!!");

   ret = pclKeyHandleClose(handleArray[1]);
   fail_unless(ret != -1, "Failed to close handle idx \"1\"!!");

   ret = pclKeyHandleClose(handleArray[2]);
   fail_unless(ret != -1, "Failed to close handle idx \"2\"!!");

   ret = pclKeyHandleClose(handleArray[3]);
   fail_unless(ret != -1, "Failed to close handle idx \"3\"!!");

   // test key handles
   handle2 = pclKeyHandleOpen(PCL_LDBID_LOCAL, "statusHandle/open_document", 3, 2);
   fail_unless(handle2 >= 0, "Failed to open handle /statusHandle/open_document");

   ret = pclKeyHandleClose(handle2);
   fail_unless(ret != -1, "Failed to close handle!!");

   ret = pclKeyHandleClose(1024);
   fail_unless(ret == EPERS_MAXHANDLE, "Max handle!!");


   // test file handles
   handle1 = pclFileOpen(PCL_LDBID_LOCAL, "media/mediaDB.db", 1, 1);
   fail_unless(handle1 != -1, "Could not open file ==> /media/mediaDB.db");

   ret = pclFileClose(handle1);
   fail_unless(handle1 != -1, "Could not closefile ==> /media/mediaDB.db");

   ret = pclFileClose(1024);
   fail_unless(ret == EPERS_MAXHANDLE, "1. Could close file, but should not!!");
}
END_TEST


START_TEST(test_WriteConfDefault)
{
   int ret = 0, fd = 0;
   unsigned char writeBuffer[]  = "This is a test string";
   unsigned char writeBuffer2[]  = "And this is a test string which is different form previous test string";
   unsigned char readBuffer[READ_SIZE]  = {0};

   // -- file  interface ---
   memset(readBuffer, 0, READ_SIZE);
   fd = pclFileOpen(PCL_LDBID_LOCAL, "media/mediaData.configurable", PCL_USER_DEFAULTDATA, 99);
   ret = pclFileWriteData(fd, writeBuffer,  (int)strlen((char*)writeBuffer));
   fail_unless(ret >= 0, "Failed pclFileWriteData");
   pclFileSeek(fd, 0, SEEK_SET);
   ret = pclFileReadData(fd, readBuffer, READ_SIZE);
   fail_unless(strncmp((char*)readBuffer, (char*)writeBuffer, strlen((char*)writeBuffer)) == 0, "Buffer not correctly read");
   (void)pclFileClose(fd);

   memset(readBuffer, 0, READ_SIZE);
   fd = pclFileOpen(PCL_LDBID_LOCAL, "media/mediaData.configurable", PCL_USER_DEFAULTDATA, 99);
   ret = pclFileWriteData(fd, writeBuffer2,  (int)strlen((char*)writeBuffer2));
   fail_unless(ret >= 0, "Failed pclFileWriteData");
   pclFileSeek(fd, 0, SEEK_SET);
   ret = pclFileReadData(fd, readBuffer, READ_SIZE);
   fail_unless(strncmp((char*)readBuffer, (char*)writeBuffer2, strlen((char*)writeBuffer2)) == 0, "Buffer2 not correctly read");
   (void)pclFileClose(fd);
}
END_TEST



START_TEST(test_GetPath)
{
   int ret = 0;
   char* path = NULL;
   const char* thePath = "/Data/mnt-c/lt-persistence_client_library_test/user/1/seat/1/media/mediaDB_create.db";
   unsigned int pathSize = 0;

   ret = pclFileCreatePath(PCL_LDBID_LOCAL, "media/mediaDB_create.db", 1, 1, &path, &pathSize);

   fail_unless(strncmp((char*)path, thePath, strlen((char*)path)) == 0, "Path not correct");
   fail_unless(pathSize == strlen((char*)path), "Path size not correct");

   pclFileReleasePath(ret);
}
END_TEST



START_TEST(test_InitDeinit)
{
   int i = 0, rval = -1, handle = 0;
   int shutdownReg = PCL_SHUTDOWN_TYPE_FAST | PCL_SHUTDOWN_TYPE_NORMAL;

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

   handle = pclFileOpen(PCL_LDBID_LOCAL, "media/mediaDB.db", 1, 1);
   //printf("pclFileOpen: %d\n", handle);
   fail_unless(handle >= 0, "Could not open file ==> /media/mediaDB.db");
   (void)pclFileClose(handle);

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
}
END_TEST



START_TEST(test_VerifyROnly)
{
   int fd = 0;
   int rval = 0;
   char* wBuffer = "This is a test string";

   fd = pclFileOpen(PCL_LDBID_LOCAL, "media/mediaDB_ReadOnly.db", 1, 1);
   fail_unless(fd != -1, "Could not open file ==> /media/mediaDB_ReadOnly.db");

   rval = pclFileWriteData(fd, wBuffer, (int)strlen(wBuffer));
   fail_unless(rval == EPERS_RESOURCE_READ_ONLY, "Write to read only file is possible, but should not ==> /media/mediaDB_ReadOnly.db");

   rval = pclFileClose(fd);
   fail_unless(rval == 0, "Failed to close file: media/mediaDB_ReadOnly.db");
}
END_TEST



void runTestSequence(const char* resourceID)
{
   int fd1 = 0, fd2 = 0, rval = 0;
      unsigned char buffer[READ_SIZE] = {0};
      unsigned char writebuffer[]  = {" _Updates file_ "};
      unsigned char writebuffer2[] = {" _New Data_ "};

      // part one: write to file
      // ------------------------------------------
      fd1 = pclFileOpen(PCL_LDBID_LOCAL, resourceID, 1, 1);
      fail_unless(fd1 != -1, "Could not open file ==> dataLoc/file.txt");

      (void)pclFileReadData(fd1, buffer, READ_SIZE);
      (void)pclFileWriteData(fd1, writebuffer, (int)strlen((char*)writebuffer));

   #if 1
      rval = pclFileClose(fd1);
      fail_unless(rval == 0, "Could not close file ==> dataLoc/file.txt");
   #else
      printf("\nN O  C L O S E\n\n");
   #endif

      // part two: remove file
      // ------------------------------------------
      rval = pclFileRemove(PCL_LDBID_LOCAL, resourceID, 1, 1);
      fail_unless(rval == 0, "Could not remove file ==> dataLoc/file.txt");


      // part three: open file again, and write to it
      // ------------------------------------------
      fd2 = pclFileOpen(PCL_LDBID_LOCAL, resourceID, 1, 1);
      fail_unless(fd1 != -1, "Could not open file ==> dataLoc/file.txt");

      (void)pclFileWriteData(fd2, writebuffer2, (int)strlen((char*)writebuffer2));

      rval = pclFileClose(fd2);
      fail_unless(rval == 0, "Could not close file ==> dataLoc/file.txt");
}


START_TEST(test_FileTest)
{
   int i = 0;
   const char* resourceID_01 = "dataLoc/fileB.txt";
   const char* resourceID_02 = "dataLoc/fileA.txt";
   int fdArray[10] = {0};

   const char* resourceIDArray[] = {"dataLoc/fileC.txt",
                                    "dataLoc/fileD.txt",
                                    "dataLoc/fileE.txt",
                                    "dataLoc/fileF.txt",
                                    "dataLoc/fileG.txt"};
#if 1
   const char* testStringsFirst[] = {"FIRST - - Test Data START - dataLoc/fileC.txt - Test Data END  ",
                                     "FIRST - - Test Data START - dataLoc/fileD.txt - Test Data END  ",
                                     "FIRST - - Test Data START - dataLoc/fileE.txt - Test Data END  ",
                                     "FIRST - - Test Data START - dataLoc/fileF.txt - Test Data END  ",
                                     "FIRST - - Test Data START - dataLoc/fileG.txt - Test Data END  "};

   const char* testStringsSecond[] = {"Second - - Test Data START - dataLoc/fileC.txt - Test Data END  ",
                                      "Second - - Test Data START - dataLoc/fileD.txt - Test Data END  ",
                                      "Second - - Test Data START - dataLoc/fileE.txt - Test Data END  ",
                                      "Second - - Test Data START - dataLoc/fileF.txt - Test Data END  ",
                                      "Second - - Test Data START - dataLoc/fileG.txt - Test Data END  "};
#endif
   // open files

   for(i=0; i<(int)sizeof(resourceIDArray) / (int)sizeof(char*); i++)
   {
      fdArray[i] = pclFileOpen(PCL_LDBID_LOCAL, resourceIDArray[i], 1, 1);
      //printf("********  test_FileTest => pclFileOpen: %s -- %d\n", resourceIDArray[i], fdArray[i] );
      fail_unless(fdArray[i] != -1, "Could not open file ==> file: %s", resourceIDArray[i]);
   }

   // write to files
   for(i=0; i<(int)sizeof(resourceIDArray) / (int)sizeof(char*); i++)
   {
      (void)pclFileWriteData(fdArray[i], testStringsFirst[i], (int)strlen((char*)testStringsFirst[i]));
   }

   runTestSequence(resourceID_01);
   runTestSequence(resourceID_02);


   // write to files again
   for(i=0; i<(int)sizeof(resourceIDArray) / (int)sizeof(char*); i++)
   {
      (void)pclFileWriteData(fdArray[i], testStringsSecond[i], (int)strlen((char*)testStringsSecond[i]));
   }


   // close files
   for(i=0; i<(int)sizeof(resourceIDArray) / (int)sizeof(char*); i++)
   {
      fdArray[i] = pclFileClose(fdArray[i]);
      fail_unless(fdArray[i] == 0, "Could not close file ==> file: %s - %d", resourceIDArray[i], fdArray[i]);
   }
}
END_TEST

void* fileWriteThread(void* userData)
{
   t_threadData* threadData = (t_threadData*)userData;

   static int i = 0;

   size_t bufferSize = strlen(gWriteBuffer);
   size_t bufferSize2 = strlen(gWriteBuffer2);

   unsigned char* readbuffer = malloc(bufferSize);
   unsigned char* readbuffer2 = malloc(bufferSize2);

   if(readbuffer != NULL && readbuffer2 != NULL)
   {
      pthread_barrier_wait(&gBarrierOne);
      usleep(5000);

      for(i=0; i<NUM_OF_WRITES; i++)
      {
         int wsize = 0, rsize = 0;
         int wsize2 = 0, rsize2 = 0;

         memset(readbuffer, 0, bufferSize);

         wsize = pclFileWriteData(threadData->fd1, gWriteBuffer, (int)bufferSize);
         ck_assert_int_ge(wsize, 0);
         pclFileSeek(threadData->fd1, 0, SEEK_SET);

         rsize = pclFileReadData(threadData->fd1, readbuffer, (int)bufferSize);
         ck_assert_int_eq(rsize, (int)bufferSize);

         usleep( (useconds_t)(50 * i * threadData->index));   // do some "random" sleep

         memset(readbuffer2, 0, bufferSize2);

         wsize2 = pclFileWriteData(threadData->fd2, gWriteBuffer2, (int)bufferSize2);
         ck_assert_int_ge(wsize2, 0);
         pclFileSeek(threadData->fd2, 0, SEEK_SET);

         rsize2 = pclFileReadData(threadData->fd2, readbuffer2, (int)bufferSize2);
         ck_assert_int_eq(rsize2, (int)bufferSize2);
      }

      free(readbuffer);
      free(readbuffer2);
   }

   return NULL;
}


START_TEST(test_MultFileReadWrite)
{
   int fd1 = -1;
   int fd2 = -1;

   fd1 = pclFileOpen(PCL_LDBID_LOCAL, "media/mediaDBWrite.db", 1, 1);
   fd2 = pclFileOpen(PCL_LDBID_LOCAL, "media/mediaDBWrite.db", 2, 1);

   if(fd1 != -1 && fd2 != 0)
   {
      pthread_t threads[NUM_THREADS];
      int* retval;
      int i=0;
      int success[NUM_THREADS] = {0};
      t_threadData threadData[NUM_THREADS];

      if(pthread_barrier_init(&gBarrierOne, NULL, NUM_THREADS) == 0)
      {
        for(i=0; i<NUM_THREADS; i++)
        {
           memset(threadData[i].threadName, 0, NAME_LEN);
           sprintf(threadData[i].threadName, "-MultWritefile-%3d-",  i);
           threadData[i].threadName[NAME_LEN-1] = '\0';
           threadData[i].index = i;
           threadData[i].fd1 = fd1;
           threadData[i].fd2 = fd2;

           if(pthread_create(&threads[i], NULL, fileWriteThread, &(threadData[i])) != -1)
           {
              (void)pthread_setname_np(threads[i], threadData[i].threadName);
              success[i] = 1;
           }
        }
        // wait
        for(i=0; i<NUM_THREADS; i++)
        {
           if(success[i] == 1)
              pthread_join(threads[i], (void**)&retval);    // wait until thread has ended
        }

        if(pthread_barrier_destroy(&gBarrierOne) != 0)
           printf("Failed to destroy barrier\n");
      }
      else
      {
        printf("Failed to init barrier\n");
      }

     (void)pclFileClose(fd1);
     (void)pclFileClose(fd2);
   }
   else
   {
     printf("Could not open file ==> /media/mediaDBWrite.db\n");
   }
}
END_TEST




static Suite * persistencyClientLib_suite()
{
   const char* testSuiteName = "Persistency Client Library (File-API)";

   Suite * s  = suite_create(testSuiteName);

   TCase * tc_persDataFile = tcase_create("DataFile");
   tcase_add_test(tc_persDataFile, test_DataFile);
   tcase_set_timeout(tc_persDataFile, 3);

   TCase * tc_DataFileConfDefault = tcase_create("DataFileConfDefault");
   tcase_add_test(tc_DataFileConfDefault, test_DataFileConfDefault);
   tcase_set_timeout(tc_DataFileConfDefault, 3);

   TCase * tc_persDataFileBackupCreation = tcase_create("DataFileBackupCreation");
   tcase_add_test(tc_persDataFileBackupCreation, test_DataFileBackupCreation);
   tcase_set_timeout(tc_persDataFileBackupCreation, 3);

   TCase * tc_persDataFileRecovery = tcase_create("DataFileRecovery");
   tcase_add_test(tc_persDataFileRecovery, test_DataFileRecovery);
   tcase_set_timeout(tc_persDataFileRecovery, 3);

   TCase * tc_WriteConfDefault = tcase_create("WriteConfDefault");
   tcase_add_test(tc_WriteConfDefault, test_WriteConfDefault);
   tcase_set_timeout(tc_WriteConfDefault, 3);

   TCase * tc_GetPath = tcase_create("GetPath");
   tcase_add_test(tc_GetPath, test_GetPath);
   tcase_set_timeout(tc_GetPath, 3);

   TCase * tc_InitDeinit = tcase_create("InitDeinit");
   tcase_add_test(tc_InitDeinit, test_InitDeinit);
   tcase_set_timeout(tc_InitDeinit, 3);

   TCase * tc_VerifyROnly = tcase_create("VerifyROnly");
   tcase_add_test(tc_VerifyROnly, test_VerifyROnly);
   tcase_set_timeout(tc_VerifyROnly, 3);

   TCase * tc_FileTest = tcase_create("FileTest");
   tcase_add_test(tc_FileTest, test_FileTest);
   tcase_set_timeout(tc_FileTest, 3);

   TCase * tc_DataHandle= tcase_create("DataHandle");
   tcase_add_test(tc_DataHandle, test_DataHandle);
   tcase_set_timeout(tc_DataHandle, 3);

   TCase * tc_MultiFileReadWrite = tcase_create("MultFileReadWrite");
   tcase_add_test(tc_MultiFileReadWrite, test_MultFileReadWrite);
   tcase_set_timeout(tc_MultiFileReadWrite, 20);


   suite_add_tcase(s, tc_WriteConfDefault);
   tcase_add_checked_fixture(tc_WriteConfDefault, data_setup, data_teardown);

   suite_add_tcase(s, tc_persDataFile);
   tcase_add_checked_fixture(tc_persDataFile, data_setup, data_teardown);

   suite_add_tcase(s, tc_persDataFileBackupCreation);
   tcase_add_checked_fixture(tc_persDataFileBackupCreation, data_setupBackup, data_teardown);

   suite_add_tcase(s, tc_persDataFileRecovery);
   tcase_add_checked_fixture(tc_persDataFileRecovery, data_setupRecovery, data_teardown);

   suite_add_tcase(s, tc_GetPath);
   tcase_add_checked_fixture(tc_GetPath, data_setup, data_teardown);

   suite_add_tcase(s, tc_VerifyROnly);
   tcase_add_checked_fixture(tc_VerifyROnly, data_setup, data_teardown);

   suite_add_tcase(s, tc_DataFileConfDefault);
   tcase_add_checked_fixture(tc_DataFileConfDefault, data_setup, data_teardown);

   suite_add_tcase(s, tc_FileTest);
   tcase_add_checked_fixture(tc_FileTest, data_setup_browser, data_teardown);

   suite_add_tcase(s, tc_InitDeinit);

   suite_add_tcase(s, tc_DataHandle);
   tcase_add_checked_fixture(tc_DataHandle, data_setup, data_teardown);

   suite_add_tcase(s, tc_MultiFileReadWrite);
   tcase_add_checked_fixture(tc_MultiFileReadWrite, data_setup, data_teardown);

   return s;
}


int main(int argc, char *argv[])
{
   int nr_failed = 0;
   (void)argv;
   (void)argc;

   // assign application name
   strncpy(gTheAppId, "lt-persistence_client_library_test", MaxAppNameLen);
   gTheAppId[MaxAppNameLen-1] = '\0';

   /// debug log and trace (DLT) setup
   DLT_REGISTER_APP("PCLT", "PCL tests");

   DLT_REGISTER_CONTEXT(gPcltDLTContext, "PCLt", "Context for PCL testing");

   DLT_LOG(gPcltDLTContext, DLT_LOG_INFO, DLT_STRING("Starting PCL test"));

   data_setupBlacklist();

   Suite * s = persistencyClientLib_suite();
   SRunner * sr = srunner_create(s);
   srunner_set_fork_status(sr, CK_NOFORK);

   srunner_run_all(sr, CK_VERBOSE /*CK_NORMAL CK_VERBOSE CK_SUBUNIT*/);

   nr_failed = srunner_ntests_failed(sr);
   srunner_ntests_run(sr);
   srunner_free(sr);

   DLT_LOG(gPcltDLTContext, DLT_LOG_INFO, DLT_STRING("End of PCL test"));

   // unregister debug log and trace
   DLT_UNREGISTER_CONTEXT(gPcltDLTContext);
   DLT_UNREGISTER_APP();

   return (0==nr_failed)?EXIT_SUCCESS:EXIT_FAILURE;

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

