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
 * @file           persistence_pfs_test.c
 * @ingroup        Persistence client library test
 * @author         Ingo Huerner
 * @brief          Persistence Power Fail Safe Test
 *                 For this test a computer controlled lab power supply will be used.
 *                 The persistence_pfs_test application sends a command to the power supply
 *                 using the serial console to switch power off for a couple of seconds to
 *                 reboot of the system and simulate unexpected power cut in order to proof
 *                 power fail save of persistence.
 *
 * @attention      To run the test the following things need to do to setup the target:
 *                 There is a script to start this test binary.
 *                 In otder to start this script on startup, copy the systemd service file "persistenc-pfs-test-start.service"
 *                 to /lib/systemd/system/.
 *                 Create also a link from /lib/systemd/system/multi-user.target.wants
 *                 to the persistence-pfs-test-start.service in /lib/systemd/system/
 *
 * @see
 */

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <pthread.h>

#include <dlt/dlt.h>
#include <dlt/dlt_common.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mount.h>

#include "../include/persistence_client_library.h"
#include "../include/persistence_client_library_key.h"
#include "../include/persistence_client_library_file.h"


#define LC_CNT_START 36
#define WR_CNT_START 43
#define BASE_STRING_END 31


DLT_DECLARE_CONTEXT(gPFSDLTContext);

static const char* gOrigPostfix   = "ORIG";
static const char* gBackupPostfix = "BACK";

static int gLifecycleCounter = 0;

static const char* gTestInProgressFlag = "/Data/mnt-c/persistence_pfst_test_in_progress";

static const char* gDefaultKeyValueResName[] =
{
   "keyValue_Resource_00",
   "keyValue_Resource_01",
   "keyValue_Resource_02",
   "keyValue_Resource_03",
   "keyValue_Resource_04",
   "keyValue_Resource_05",
   "keyValue_Resource_06",
   "keyValue_Resource_07",
   "keyValue_Resource_08",
   "keyValue_Resource_09",
   "keyValue_Resource_10",
   "keyValue_Resource_11",
   "keyValue_Resource_12",
   "keyValue_Resource_13",
   "keyValue_Resource_14",
   "keyValue_Resource_15",
   "keyValue_Resource_16",
   "keyValue_Resource_17",
   "keyValue_Resource_18",
   "keyValue_Resource_19",
};

static const char* gDefaultKeyValueTestData[] =
{
   "keyValue pair cache Zero     - %s : 000000 000000",
   "keyValue pair cache One      - %s : 000000 000000",
   "keyValue pair cache Two      - %s : 000000 000000",
   "keyValue pair cache Three    - %s : 000000 000000",
   "keyValue pair cache Four     - %s : 000000 000000",
   "keyValue pair cache Five     - %s : 000000 000000",
   "keyValue pair wt Six         - %s : 000000 000000",
   "keyValue pair wt Seven       - %s : 000000 000000",
   "keyValue pair wt eight       - %s : 000000 000000",
   "keyValue pair wt nine        - %s : 000000 000000",
   "keyValue pair wt ten         - %s : 000000 000000",
   "keyValue pair cache eleven   - %s : 000000 000000",
   "keyValue pair cache twelve   - %s : 000000 000000",
   "keyValue pair cache thirteen - %s : 000000 000000",
   "keyValue pair cache fourteen - %s : 000000 000000",
   "keyValue pair cache fifteen  - %s : 000000 000000",
   "keyValue pair wt sixteen     - %s : 000000 000000",
   "keyValue pair wt Seventeen   - %s : 000000 000000",
   "keyValue pair wt eighteen    - %s : 000000 000000",
   "keyValue pair wt nineteen    - %s : 000000 000000",
};



static const char* gDefaultFileAPITestData[] =
{
   "file API cache One   : 000000 000000",
   "file API cache Two   : 000000 000000",
   "file API cache Three : 000000 000000",
   "file API cache Four  : 000000 000000",
   "file API wt One      : 000000 000000",
   "file API wt Two      : 000000 000000",
   "file API wt Three    : 000000 000000",
   "file API wt Four     : 000000 000000"
};

static const char* gDefaultFileResNames[] =
{
   "file API_Resource_01",
   "file API_Resource_02",
   "file API_Resource_03",
   "file API_Resource_04",
   "file API_Resource_05",
   "file API_Resource_06",
   "file API_Resource_07",
   "file API_Resource_08"
};


pthread_cond_t  gPowerDownMtxCond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t gPowerDownMtx   = PTHREAD_MUTEX_INITIALIZER;



// forward declaration

/// verify written data form previous lifefycle
void verify_data_key_value();
void verify_data_file();



/// write data until power off occurs
void write_data_key_value(int numLoops, int counter);
void write_data_file(int numLoops);
int mount_persistence(const char* deviceName);
void unmount_persistence();



/// setup initial test data
int setup_test_data(const char* postfix)
{
   int i = 0, ret = 0;
   char databuffer[64] = {0};

   unsigned int shutdownReg = PCL_SHUTDOWN_TYPE_FAST | PCL_SHUTDOWN_TYPE_NORMAL;

   pclInitLibrary("pfs_test", shutdownReg);     // register to persistence client library

   // key/value data
   for(i=0; i<sizeof(gDefaultKeyValueTestData) / sizeof(char*); i++)
   {
      memset(databuffer, 0, 64);
      snprintf(databuffer, 64, gDefaultKeyValueTestData[i], postfix);
      printf(" setup_test_data - [%.2d] => %s\n", i, databuffer);
      ret = pclKeyWriteData(0xFF, gDefaultKeyValueResName[i], 1, 1, (unsigned char*)databuffer, strlen(databuffer));
      if(ret < 0)
      {
         printf("setup_test_data =>  failed to write data: %d\n", ret );
         //DLT_LOG(gPFSDLTContext, DLT_LOG_ERROR, DLT_STRING("write key/value - failed to write data"), DLT_INT(ret));
      }
   }

#if 0
   // file data
   for(i=0; i<sizeof(gDefaultFileAPITestData) / sizeof(char*); i++)
   {
      printf(" file - data[%d] => %s\n", i, gDefaultFileAPITestData[i]);
   }
#endif

   pclDeinitLibrary();                          // unregister from persistence client library
   sync();
   return 1;
}



void verify_test_setup()
{
   int i = 0, ret = 0;
   unsigned int shutdownReg = PCL_SHUTDOWN_TYPE_FAST | PCL_SHUTDOWN_TYPE_NORMAL;

   char buffer[64] = {0};

   pclInitLibrary("pfs_test", shutdownReg);     // register to persistence client library

   for(i=0; i<sizeof(gDefaultKeyValueTestData) / sizeof(char*); i++)
   {
      ret = pclKeyReadData(0xFF, gDefaultKeyValueResName[i], 1, 1, (unsigned char*)buffer, 64);
      if(ret < 0)
      {
         printf("verify_test_setup - key/value - pclKeyReadData FAILED: %s => \"%s\"\n", gDefaultKeyValueResName[i], buffer);
      }

      printf("verify_test_setup: [%.2d] => %s\n", i, buffer);

      memset(buffer, 0, 64);
   }

   pclDeinitLibrary();                          // unregister from persistence client library
}



int setup_serial_con(const char* ttyConsole)
{
   struct termios tty;
   int rval = -1;

   memset (&tty, 0, sizeof(tty));
   tty.c_iflag = IGNPAR;
   tty.c_oflag = 0;
   tty.c_cflag = CS8 | CREAD | CLOCAL; // 8n1, see termios.h for more information
   tty.c_lflag = 0;
   tty.c_cc[VMIN] = 1;
   tty.c_cc[VTIME] = 5;

   printf("Opening connection to %s\n", ttyConsole);
   rval = open(ttyConsole, O_RDWR  | O_NOCTTY | O_SYNC);
   if(rval != -1)
   {
      if (cfsetospeed (&tty, B115200) == -1)    // 115200 baud
      {
        printf("Failed to set cfsetospeed\n");
      }
      if(cfsetispeed (&tty, B115200))           // 115200 baud
      {
        printf("Failed to set cfsetispeed\n");
      }

      tcsetattr(rval, TCSANOW, &tty);

      printf("tty fd: %d\n", rval);
   }
   else
   {
      printf("Failed to open console: %s - %s\n", ttyConsole, strerror(errno));
   }

   return rval;
}



/// after this function has been called the system reboots
void send_serial_shutdown_cmd(int fd)
{
   // command for the computer controller power supply to power off and restart after 2.5 second again
   static const char data[] = { "USET 12.000; ISET 10.000; OUTPUT OFF; WAIT 0.500; OUTPUT ON\n" };

   if(write(fd, data, sizeof(data)) != -1)
   {
      fdatasync(fd);
      printf("Data: %s\n", data);
   }
   else
   {
      printf("Failed to write to tty - size: %d - error: %s\n", sizeof(data), strerror(errno));
   }

   close(fd);
}



int update_test_progress_flag(const char* filename, int *counter)
{
   int fd = -1, rval = 0;

   if(access(filename, F_OK) == 0)
   {
      fd = open(filename, O_RDWR);     // file exists, update content
      if(fd != -1)
      {
         int size = 0;
         char buffer[12] = {0};
         if((size = read(fd, buffer, 12)) != -1)
         {
            rval = atoi(buffer);
            *counter = rval;
            snprintf(buffer, 12, "%d", *counter + 1);

            lseek(fd, 0, SEEK_SET);
            if((size = write(fd, buffer, strlen(buffer))) == -1)
            {
               printf("   Failed to write to file: %s » %s\n", filename, strerror(errno));
            }
         }
         else
         {
           printf("   Failed to write to file: %s » %s\n", filename, strerror(errno));
           rval = -1;
         }
      }
      else
      {
         rval = -1;
      }
   }
   else
   {
      fd = open(filename, O_CREAT|O_RDWR|O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
      if(fd != -1)
      {
         int size = 0;
         char buffer[12] = {0};
         snprintf(buffer, 12, "%d", 1);
         if((size = write(fd, buffer, strlen(buffer))) == -1)
         {
           printf("   Failed to write to file: %s » %s\n", filename, strerror(errno));
           rval = -1;
         }
         *counter = 1;
      }
      else
      {
         rval = -1;
      }
   }
   syncfs(fd);
   close(fd);
   return rval;
}



void update_test_data(char* buffer, int lc_counter, int write_counter)
{
   int i = 0;
   char lc_cnt_buff[7] = {0};
   char wr_cnt_buff[7] = {0};

   snprintf(lc_cnt_buff, 7, "%.6d", lc_counter);
   snprintf(wr_cnt_buff, 7, "%.6d", write_counter);

   for(i=0; i<6; i++)
   {
      buffer[LC_CNT_START+i] = lc_cnt_buff[i];
      buffer[WR_CNT_START+i] = wr_cnt_buff[i];
   }
}


void* power_supply_shutdown(void* dataPtr)
{
   int fd = (int)(dataPtr);
   int secs = 100000;
   printf("Shutdown thread started fd: %d\n", fd);

   pthread_cond_wait(&gPowerDownMtxCond, &gPowerDownMtx);
   pthread_mutex_unlock(&gPowerDownMtx);

# if 1
   printf("   Send pwrOff command!\n");
#else
   printf("   Send pwrOff command but sleep first: %d !!!!\n", secs);
   usleep(secs);
   printf("     ==> no more sleeping!!!!\n");
#endif
   send_serial_shutdown_cmd(fd);
   printf("   Cut pwr OFF => ByBy\n");

   return NULL;
}


int get_lifecycle_count(char* buf)
{
   return atoi(&buf[LC_CNT_START]);
}

int get_write_count(char* buf)
{
   return atoi(&buf[WR_CNT_START]);
}


int main(int argc, char *argv[])
{
   int rVal = EXIT_SUCCESS;
   int ttyfd = -1;
   int* retval;
   char ttydevice[24] = {0};
   unsigned int shutdownReg = PCL_SHUTDOWN_TYPE_FAST | PCL_SHUTDOWN_TYPE_NORMAL;
   pthread_t powerShutdownThread;
   struct sched_param param;
   pthread_attr_t tattr;

   printf("------------------\n");
   printf("P F S - Test start\n");
   printf("------------------\n");

   /// debug log and trace (DLT) setup
   //DLT_REGISTER_APP("PFS","power fail safe test");

   //DLT_REGISTER_CONTEXT(gPFSDLTContext,"PFS","Context for PCL PFS test logging");

   if (argc < 2) {
       printf ("Please start with %s /dev/ttyS1 or /dev/ttyUSB0 (for example)\n", argv[0]);
       return EXIT_SUCCESS;
   }

   pthread_mutex_lock(&gPowerDownMtx);    // lock power down mutex and release when powser should be cut off


   // default serial console
   strncpy(ttydevice, "/dev/ttyUSB0", 24);

#if 0
   // mount persistence partitions
   if(-1 != mount_persistence("/dev/sdb") )
   {
#endif
      int numLoops = 1000000, opt = 0;

      while ((opt = getopt(argc, argv, "l:s:")) != -1)
      {
         switch (opt)
         {
            case 'l':
               numLoops = atoi(optarg);
               break;
            case 's':
               memset(ttydevice, 0, 24);
               strncpy(ttydevice, optarg, 24);
               break;
           }
       }

      // setup the serial connection to the power supply
      ttyfd = setup_serial_con(ttydevice);
      if(ttyfd == -1)
      {
         printf("Failed to setup serial console: \"%s\"\n", ttydevice);
         return EXIT_SUCCESS;
      }

      if(update_test_progress_flag(gTestInProgressFlag, &gLifecycleCounter) == 0)
      {
         printf("PFS test - first lifecycle, setup test data\n");
         //DLT_LOG(gPFSDLTContext, DLT_LOG_INFO, DLT_STRING("PFS test - first lifecycle, setup test data"));

         printf("--- setup data --- \n");
         setup_test_data(gBackupPostfix);
         setup_test_data(gOrigPostfix);

         printf("--- verify data setup --- \n");
         verify_test_setup();
      }

      printf("PFS test - Lifecycle counter: %d - number of test loops: %d\n", gLifecycleCounter, numLoops);
      /*
      DLT_LOG(gPFSDLTContext, DLT_LOG_INFO, DLT_STRING("PFS test - Lifecycle counter:"), DLT_INT(gLifecycleCounter),
                                            DLT_STRING("- number of write loops:"), DLT_INT(numLoops));
      */
      pthread_attr_init(&tattr);
      param.sched_priority = 49;
      pthread_attr_setschedparam(&tattr, &param);

       // create here the dbus connection and pass to main loop
      if(pthread_create(&powerShutdownThread, &tattr, power_supply_shutdown, (void*)ttyfd) == -1)
      {
        //DLT_LOG(gPFSDLTContext, DLT_LOG_ERROR, DLT_STRING("pthread_create( DBUS run_mainloop )") );
        return -1;
      }

      pclInitLibrary("pfs_test", shutdownReg);     // register to persistence client library

      // verify the data form previous lifecycle
      printf("--- Verify Data ---!!\n");
      verify_data_key_value();
      //verify_data_file();

      // write data
      printf("--- Write Data!! ---\n");
      write_data_key_value(numLoops, gLifecycleCounter); // on odd lifecycle numbers, corrupt db data, otherwise corrupt db header
      //write_data_file(numLoops);


      pthread_cond_signal(&gPowerDownMtxCond);
      pthread_mutex_unlock(&gPowerDownMtx);
      printf("Deinit library\n");
      pclDeinitLibrary();                          // unregister from persistence client library


#if 0
      unmount_persistence();

   }
   else
   {
      //DLT_LOG(gPFSDLTContext, DLT_LOG_ERROR, DLT_STRING("PFS test - failed to mount pers partition:"), DLT_INT(gLifecycleCounter) );
      printf("Mount Failed\n");
      rVal = EXIT_FAILURE;
   }
#endif

   printf("End of PFS app\n");

   // unregister debug log and trace
   //DLT_UNREGISTER_APP();

   //dlt_free();

   printf("Wait until shutdown thread has finished\n");
   // wait until the shutdown thread has ended
   pthread_join(powerShutdownThread, (void**)&retval);

   return rVal;
}



void verify_data_key_value()
{
   int i=0, ret = 0;
   char buffer[64] = {0};

   // read data from previous lifecycle - key/value
   for(i=0; i<sizeof(gDefaultKeyValueTestData) / sizeof(char*); i++)
   {
      memset(buffer, 0, 64);
      ret = pclKeyReadData(0xFF, gDefaultKeyValueResName[i], 1, 1, (unsigned char*)buffer, 64);
      if(ret < 0)
      {
         //DLT_LOG(gPFSDLTContext, DLT_LOG_WARN, DLT_STRING("verify - key/value - => failed to read data"), DLT_INT(ret), DLT_STRING(buffer));
         printf("verify - key/value - pclKeyReadData FAILED: %s => \"%s\"\n", gDefaultKeyValueResName[i], buffer);
      }
      else
      {
         int lc_count = get_lifecycle_count(buffer);

         // first verify base string
         if(0 == strncmp(gDefaultKeyValueTestData[i], buffer, BASE_STRING_END) )
         {
            char extendedStringOrig[64] = {0};
            char extendedStringBack[64] = {0};

            snprintf(extendedStringOrig, 64, gDefaultKeyValueTestData[i], gOrigPostfix);
            snprintf(extendedStringBack, 64, gDefaultKeyValueTestData[i], gBackupPostfix);


            /*printf("Reference: %s\n", extendedStringOrig);
            printf("    found: %s\n",buffer);*/

            // check if original or backup will be used
            if(0 == strncmp(extendedStringOrig, buffer, LC_CNT_START))
            {
               /*
               printf("ORIGINAL detected, everything OK:\n");
               printf("   desired: %s\n", extendedStringOrig);
               printf("   actual : %s\n", buffer);*/
            }
            else if(0 == strncmp(extendedStringBack, buffer, LC_CNT_START))
            {
               /*
               printf("BACKUP detected, everything OK:\n");
               printf("   desired: %s\n", extendedStringBack);
               printf("   actual : %s\n", buffer); */
            }
            else
            {

               printf("----------------------------------------------------\n");
               printf("!!!! F A I L U R E !!!!! => lifecycle count - current: %d - found: %d\n", gLifecycleCounter, lc_count);
               printf("   desired: %s\n", gDefaultKeyValueTestData[i]);
               printf("   actual : %s\n", buffer);
               printf("----------------------------------------------------\n");
            }


#if 0
            if(lc_count != (gLifecycleCounter-1) )
            {
               printf("   Failure - LC count - current: %d - previous: %d - \"%s\"\n", gLifecycleCounter, lc_count, buffer);
               /*DLT_LOG(gPFSDLTContext, DLT_LOG_ERROR, DLT_STRING("Failure - LC count - current:"), DLT_INT(gLifecycleCounter),
                                                      DLT_STRING("- previous:"), DLT_INT(lc_count),
                                                      DLT_STRING("- buf:"), DLT_STRING(buffer));*/
            }
#endif

         }
         else
         {
            printf("   Failure - base string does not match - actual: \"%s\" - desired: \"%s\"\n", buffer, gDefaultKeyValueTestData[i]);
            /*DLT_LOG(gPFSDLTContext, DLT_LOG_ERROR, DLT_STRING("Failure - base string does not match - actual: "), DLT_STRING(buffer),
                                                   DLT_STRING("- desired:"), DLT_STRING(gDefaultKeyValueTestData[i]));*/
         }

      }
   }
   printf("Persistence pfs test - verification ended!!!!\n");
}


void write_data_key_value(int numLoops, int counter)
{
   int i=0, k=0, ret = 0;

   if( (counter%2) == 1)
   {
      printf("Corrupt Data: numLoops: %d!!!!\n", numLoops);
   }
   else
   {
      printf("Do  N O T  corrupt data!!!\n");
   }
#if 1
   for(k=0; k<numLoops; k++)
   {
      // write key/value data

      for(i=0; i<sizeof(gDefaultKeyValueTestData) / sizeof(char*); i++)
      {
         char buffer[64] = {0};

         strncpy(buffer, gDefaultKeyValueTestData[i], 64);
         update_test_data(buffer, gLifecycleCounter, k);

         if(    (k == (int)numLoops/((counter%20)+1))
             && (counter%2 == 1))
         {
            // unlock mutex
            printf("Now POWER OFF => k: %d \n", k);
            pthread_cond_signal(&gPowerDownMtxCond);
            pthread_mutex_unlock(&gPowerDownMtx);
         }
         ret = pclKeyWriteData(0xFF, gDefaultKeyValueResName[i], 1, 1, (unsigned char*)buffer, strlen(buffer));
         if(ret < 0)
         {
            printf("  failed to write data: %d\n", ret );
            //DLT_LOG(gPFSDLTContext, DLT_LOG_ERROR, DLT_STRING("write key/value - failed to write data"), DLT_INT(ret));
         }

         //printf("write data - key/value - \"%s\"\n", buffer);
      }
   }  //num writes per lifecycle

   printf("End of Test\n");
#endif
}



void verify_data_file()
{
   int i=0, ret = 0;
   int handles[128] = {0};
   char buffer[64] = {0};

   // open files
   for(i=0; i<sizeof(gDefaultFileAPITestData) / sizeof(char*); i++)
   {
      handles[i] = pclFileOpen(0xFF, gDefaultFileResNames[i], 1, 0);
   }

   // read data from previous lifecycle - file
   for(i=0; i<sizeof(gDefaultFileAPITestData) / sizeof(char*); i++)
   {
      memset(buffer, 0, 64);
      ret = pclFileReadData(handles[i], buffer, 64);
      if(ret < 0)
      {
         //DLT_LOG(gPFSDLTContext, DLT_LOG_ERROR, DLT_STRING("verify data - file - failed to read data"), DLT_INT(ret));
      }
      else
      {
         printf("verify file - file - \"%s\"\n", buffer);
         //DLT_LOG(gPFSDLTContext, DLT_LOG_INFO, DLT_STRING("verify file: "), DLT_STRING(buffer));
      }
   }

   // close fd's
   for(i=0; i<sizeof(gDefaultFileAPITestData) / sizeof(char*); i++)
   {
      ret = pclFileClose(handles[i]);
      if(ret != 0)
      {
         //DLT_LOG(gPFSDLTContext, DLT_LOG_ERROR, DLT_STRING("close file - failed to close"), DLT_INT(ret));
      }
   }
}


void write_data_file(int numLoops)
{
   int i=0, k=0, ret = 0;
   int handles[128] = {0};

   // open files
   for(i=0; i<sizeof(gDefaultFileAPITestData) / sizeof(char*); i++)
   {
      handles[i] = pclFileOpen(0xFF, gDefaultFileResNames[i], 1, 1);
   }

   for(k=0; k<numLoops; k++)
   {
      // write file data

      for(i=0; i<sizeof(gDefaultFileAPITestData) / sizeof(char*); i++)
      {
         char buffer[64] = {0};

         strncpy(buffer, gDefaultFileAPITestData[i], 64);
         update_test_data(buffer, gLifecycleCounter, k);

         //DLT_LOG(gPFSDLTContext, DLT_LOG_INFO, DLT_STRING("- write file:"), DLT_STRING(buffer));
         printf("write data - file - \"%s\"\n", buffer);

         ret = pclFileWriteData(handles[i], buffer, strlen(buffer));
         if(ret < 0)
         {
            //DLT_LOG(gPFSDLTContext, DLT_LOG_ERROR, DLT_STRING("write file - failed to write data"), DLT_INT(ret));
         }
         usleep((int)(rand()/100000)); // sleep a time random period
         pclFileSeek(handles[i], 0, SEEK_SET);
      }

   }  // writes per lifecycle

   // close fd's
   for(i=0; i<sizeof(gDefaultFileAPITestData) / sizeof(char*); i++)
   {
      ret = pclFileClose(handles[i]);
      if(ret != 0)
      {
         //DLT_LOG(gPFSDLTContext, DLT_LOG_ERROR, DLT_STRING("close file - failed to close"), DLT_INT(ret));
      }
   }
}



void unmount_persistence()
{
   if(umount2("/Data/mnt-c", MNT_DETACH) == -1)
   {
      printf("unmount /Data/mnt-c - FAILED: %s\n", strerror(errno));
   }

   if(umount2("/Data/mnt-wt", MNT_DETACH) == -1)
   {
      printf("unmount /Data/mnt-wt - FAILED: %s\n", strerror(errno));
   }

}

int mount_persistence(const char* deviceName)
{
   int rval = 0;

   printf("Mount - /Data/mnt-c\n");
   if(-1 != mount(deviceName, "/Data/mnt-c",  "ext4", 0, ""))
   {
      printf("Mount - /Data/mnt-wt\n");
      if(-1 == mount(deviceName, "/Data/mnt-wt", "ext4", 0, ""))
      {
         printf("Mount - FAILED - mnt-wt: %s\n", strerror(errno));
         rval = -1;
      }
      else
      {
         printf("Mount - SUCCESS\n");
      }
   }
   else
   {
      printf("Mount - FAILED - mnt-c: %s\n", strerror(errno));
      rval = -1;
   }

   return rval;
}

