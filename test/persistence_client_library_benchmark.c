/******************************************************************************
 * Project         Persistency
 * (c) copyright   2014
 * Company         XS Embedded GmbH
 *****************************************************************************/
/******************************************************************************
 * This Source Code Form is subject to the terms of the
 * Mozilla Public License, v. 2.0. If a  copy of the MPL was not distributed
 * with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
******************************************************************************/
 /**
 * @file           persistence_client_library_benchmark.c
 * @ingroup        Persistence client library test
 * @author         Ingo Huerner
 * @brief          Test of persistence client library
 * @see            
 */

#include "../include/persistence_client_library_key.h"
#include "../include/persistence_client_library_file.h"
#include "../include/persistence_client_library_error_def.h"

#include <stdio.h>
#include <string.h>

#include <dlt/dlt.h>
#include <dlt/dlt_common.h>

#include <sys/time.h>
#include <sys/resource.h>
#include <pthread.h>


#define SECONDS2NANO 1000000000L
#define NANO2MIL        1000000L
#define MIL2SEC            1000L

#define BUFFER_SIZE  1024

// define for the used clock: "CLOCK_MONOTONIC" or "CLOCK_REALTIME"
#define CLOCK_ID  CLOCK_MONOTONIC


const char* gAppName = "lt-persistence_client_library_test";

// definition of weekday to generate random string
char* dayOfWeek[] = { "Sunday   ",
                      "Monday   ",
                      "Tuesday  ",
                      "Wednesday",
                      "Thursday ",
                      "Friday   ",
                      "Saturday "};


char sysTimeBuffer[BUFFER_SIZE];
char sysTimeBuffer2[BUFFER_SIZE];



inline long long getNsDuration(struct timespec* start, struct timespec* end)
{
   return ((end->tv_sec * SECONDS2NANO) + end->tv_nsec) - ((start->tv_sec * SECONDS2NANO) + start->tv_nsec);
}

inline double getMsDuration(struct timespec* start, struct timespec* end)
{
   return (double)((end->tv_sec * SECONDS2NANO) + end->tv_nsec) - ((start->tv_sec * SECONDS2NANO) + start->tv_nsec)/NANO2MIL;
}



void init_benchmark(int numLoops)
{
   int i = 0;
   long long durationInit = 0;
   long long durationDeInit = 0;
   struct timespec initStart, initEnd;
   struct timespec deInitStart, deInitEnd;
   unsigned int shutdownReg = PCL_SHUTDOWN_TYPE_FAST | PCL_SHUTDOWN_TYPE_NORMAL;

   printf("\nTest  i n i t / d e i n i t   performance: %d times\n", numLoops);

   // init
   clock_gettime(CLOCK_ID, &initStart);
   (void)pclInitLibrary(gAppName , shutdownReg);
   clock_gettime(CLOCK_ID, &initEnd);
   durationInit += getNsDuration(&initStart, &initEnd);

   // deinit
   clock_gettime(CLOCK_ID, &deInitStart);
   pclDeinitLibrary();
   clock_gettime(CLOCK_ID, &deInitEnd);
   durationDeInit += getNsDuration(&deInitStart, &deInitEnd);

   printf(" Init   (single)  => %f ms \n",   (double)((double)durationInit/NANO2MIL));
   printf(" Deinit (single)  => %f ms \n", (double)((double)durationDeInit/NANO2MIL));

   durationInit = 0;
   durationDeInit = 0;


   clock_gettime(CLOCK_ID, &initStart);
   (void)pclInitLibrary(gAppName , shutdownReg);
   clock_gettime(CLOCK_ID, &initEnd);
   durationInit += getNsDuration(&initStart, &initEnd);

   // deinit
   clock_gettime(CLOCK_ID, &deInitStart);
   pclDeinitLibrary();
   clock_gettime(CLOCK_ID, &deInitEnd);
   durationDeInit += getNsDuration(&deInitStart, &deInitEnd);

   printf(" Init   (single)  => %f ms \n",   (double)((double)durationInit/NANO2MIL));
   printf(" Deinit (single)  => %f ms \n", (double)((double)durationDeInit/NANO2MIL));

   durationInit = 0;
   durationDeInit = 0;

   for(i=0; i<numLoops; i++)
   {
      // init
      clock_gettime(CLOCK_ID, &initStart);
      (void)pclInitLibrary(gAppName , shutdownReg);
      clock_gettime(CLOCK_ID, &initEnd);
      durationInit += getNsDuration(&initStart, &initEnd);

      // deinit
      clock_gettime(CLOCK_ID, &deInitStart);
      (void)pclDeinitLibrary();
      clock_gettime(CLOCK_ID, &deInitEnd);
      durationDeInit += getNsDuration(&deInitStart, &deInitEnd);
   }

   printf(" Init             => %f ms \n",   (double)((double)durationInit/NANO2MIL/numLoops));
   printf(" Deinit           => %f ms \n", (double)((double)durationDeInit/NANO2MIL/numLoops));

}



void read_benchmark(int numLoops)
{
   int ret = 0, i = 0;
   long long duration = 0;
   struct timespec readStart, readEnd;

   unsigned char buffer[BUFFER_SIZE] = {0};

   printf("\nTest  r e a d  performance: %d times\n", numLoops);

   clock_gettime(CLOCK_ID, &readStart);
   ret = pclKeyReadData(0xFF, "pos/last_position_ro_bench",      1, 2, buffer, BUFFER_SIZE);
   clock_gettime(CLOCK_ID, &readEnd);
   duration += getNsDuration(&readStart, &readEnd);
   printf(" INITIAL read 1 \"pos/last_position_ro_bench\"  => %f ms [%d bytes]\n", (double)((double)duration/NANO2MIL), ret);

   duration = 0;
   memset(buffer, 0, BUFFER_SIZE);
   for(i=0; i<numLoops; i++)
   {
      clock_gettime(CLOCK_ID, &readStart);
      ret = pclKeyReadData(0xFF, "pos/last_position_ro_bench",      1, 2, buffer, BUFFER_SIZE);
      clock_gettime(CLOCK_ID, &readEnd);

      duration += getNsDuration(&readStart, &readEnd);
   }
   printf(" Further read 1 \"pos/last_position_ro_bench\"  => %f ms [%d bytes]\n", (double)((double)duration/NANO2MIL/numLoops), ret);


   duration = 0;
   memset(buffer, 0, BUFFER_SIZE);
   for(i=0; i<numLoops; i++)
   {
      clock_gettime(CLOCK_ID, &readStart);
      ret = pclKeyReadData(0xFF, "pos/last_position_ro_bench",      1, 2, buffer, BUFFER_SIZE);
      clock_gettime(CLOCK_ID, &readEnd);

      duration += getNsDuration(&readStart, &readEnd);
   }
   printf(" Further read 1 \"pos/last_position_ro_bench\"  => %f ms [%d bytes]\n", (double)((double)duration/NANO2MIL)/numLoops, ret);


   duration = 0;
   memset(buffer, 0, BUFFER_SIZE);

      clock_gettime(CLOCK_ID, &readStart);
      ret = pclKeyReadData(0xFF, "pos/last_position_ro_bench2",      1, 2, buffer, BUFFER_SIZE);
      clock_gettime(CLOCK_ID, &readEnd);

   duration = getNsDuration(&readStart, &readEnd);
   printf(" INITIAL read 2 \"pos/last_position_ro_bench2\" => %f ms [%d bytes]\n", (double)((double)duration/NANO2MIL), ret);


   duration = 0;
   memset(buffer, 0, BUFFER_SIZE);
   for(i=0; i<numLoops; i++)
   {
      clock_gettime(CLOCK_ID, &readStart);
      ret = pclKeyReadData(0xFF, "pos/last_position_ro_bench2",      1, 2, buffer, BUFFER_SIZE);
      clock_gettime(CLOCK_ID, &readEnd);

      duration += getNsDuration(&readStart, &readEnd);
   }
   printf(" Further read 2 \"pos/last_position_ro_bench2\" => %f ms [%d bytes]\n", (double)((double)duration/NANO2MIL)/numLoops, ret);


   duration = 0;
   memset(buffer, 0, BUFFER_SIZE);
   for(i=0; i<numLoops; i++)
   {
      clock_gettime(CLOCK_ID, &readStart);
      ret = pclKeyReadData(0xFF, "pos/last_position_ro_bench",      1, 2, buffer, BUFFER_SIZE);
      clock_gettime(CLOCK_ID, &readEnd);

      duration += getNsDuration(&readStart, &readEnd);
   }
   printf(" Further read 1 \"pos/last_position_ro_bench\"  => %f ms [%d bytes]\n", (double)((double)duration/NANO2MIL)/numLoops, ret);


#if 0
   printf(" Size [pos/last_position_ro_bench] : %d bytes\n", pclKeyGetSize(0xFF, "pos/last_position_ro_bench",  1, 2));
   printf(" Size [pos/last_position_ro_bench2]: %d bytes\n", pclKeyGetSize(0xFF, "pos/last_position_ro_bench2", 1, 2));
#endif

}



void write_benchmark(int numLoops)
{
   int ret = 0, ret2 = 0, i = 0;
   long long duration = 0;
   struct timespec writeStart, writeEnd;
   unsigned char buffer[BUFFER_SIZE] = {0};

   printf("\nTest  w r i t e  performance: %d times\n", numLoops);

   clock_gettime(CLOCK_ID, &writeStart);
   ret = pclKeyWriteData(0xFF, "pos/last_position_w_bench", 1, 2, (unsigned char*)sysTimeBuffer, strlen(sysTimeBuffer));
   clock_gettime(CLOCK_ID, &writeEnd);
   duration = getNsDuration(&writeStart, &writeEnd);
   printf(" Initial Write 1 => %f ms [%d bytes]\n", (double)((double)duration/NANO2MIL), ret);

   duration = 0;
   for(i=0; i<numLoops; i++)
   {
      clock_gettime(CLOCK_ID, &writeStart);
      ret = pclKeyWriteData(0xFF, "pos/last_position_w_bench", 1, 2, (unsigned char*)sysTimeBuffer, strlen(sysTimeBuffer));
      clock_gettime(CLOCK_ID, &writeEnd);

      duration += getNsDuration(&writeStart, &writeEnd);
   }
   printf(" Further Write 1 => %f ms [%d bytes]\n", (double)((double)duration/NANO2MIL)/numLoops, ret);


   duration = 0;
   for(i=0; i<numLoops; i++)
   {
      clock_gettime(CLOCK_ID, &writeStart);
      ret2 = pclKeyWriteData(0xFF, "pos/last_position_w_bench2", 1, 2, (unsigned char*)sysTimeBuffer2, strlen(sysTimeBuffer2));
      clock_gettime(CLOCK_ID, &writeEnd);

      duration += getNsDuration(&writeStart, &writeEnd);
   }
   printf(" Initial Write 2 => %f ms [%d bytes]\n", (double)((double)duration/NANO2MIL)/numLoops, ret2);


   duration = 0;
   for(i=0; i<numLoops; i++)
   {
      clock_gettime(CLOCK_ID, &writeStart);
      ret2 = pclKeyWriteData(0xFF, "pos/last_position_w_bench2", 1, 2, (unsigned char*)sysTimeBuffer2, strlen(sysTimeBuffer2));
      clock_gettime(CLOCK_ID, &writeEnd);

      duration += getNsDuration(&writeStart, &writeEnd);
   }
   printf(" Further Write 2 => %f ms [%d bytes]\n", (double)((double)duration/NANO2MIL)/numLoops, ret2);


#if 0
   printf(" Size [pos/last_position_w_bench]: %d\n", ret);
   printf(" Size [pos/last_position_w_bench]: %d\n", ret2);
#endif

   clock_gettime(CLOCK_ID, &writeStart);
   ret = pclKeyReadData(0xFF, "pos/last_position_w_bench2",      1, 2, buffer, BUFFER_SIZE);
   clock_gettime(CLOCK_ID, &writeEnd);
   duration = getNsDuration(&writeStart, &writeEnd);
   printf(" Write verification, pclKeyReadData => %f ms [%d bytes]\n", (double)((double)duration/NANO2MIL), ret);


#if 0
   printf(" Buffer [pos/last_position_w_bench2]:\n %s \n\n", buffer);
#endif
}



void handle_benchmark(int numLoops)
{
   int hdl = 0, hdl2 = 0, hdl3 = 0, hdl4 = 0, ret = 0, i = 0;
   long long duration = 0;
   struct timespec openStart, openEnd;
   unsigned char buffer[BUFFER_SIZE] = {0};

   printf("\nTest  h a n d l e  performance: %d times\n", numLoops);

   duration = 0;
   for(i=0; i<1; i++)
   {
      clock_gettime(CLOCK_ID, &openStart);
      hdl = pclKeyHandleOpen(0xFF, "handlePos/last_position_ro_bench", 1, 2);
      clock_gettime(CLOCK_ID, &openEnd);

      //pclKeyHandleClose(hdl);

      duration += getNsDuration(&openStart, &openEnd);
   }
   printf(" Open 1 => %f ms\n", (double)((double)duration/NANO2MIL));

   duration = 0;
   for(i=0; i<2; i++)
   {
      clock_gettime(CLOCK_ID, &openStart);
      hdl2 = pclKeyHandleOpen(0xFF, "handlePos/last_position_ro_bench2", 1, 2);
      clock_gettime(CLOCK_ID, &openEnd);

      //pclKeyHandleClose(hdl2);

      duration += getNsDuration(&openStart, &openEnd);
   }
   printf(" Open 2 => %f ms\n", (double)((double)duration/NANO2MIL));

   /*
   hdl  = pclKeyHandleOpen(0xFF, "handlePos/last_position_ro_bench", 1, 2);
   hdl2 = pclKeyHandleOpen(0xFF, "handlePos/last_position_ro_bench2", 1, 2);
   */


   duration = 0;
   for(i=0; i<numLoops; i++)
   {
      clock_gettime(CLOCK_ID, &openStart);
      ret = pclKeyHandleReadData(hdl, buffer, BUFFER_SIZE);
      clock_gettime(CLOCK_ID, &openEnd);

      duration += getNsDuration(&openStart, &openEnd);
   }
   printf(" Read 1 => %f ms [%d bytes]\n", (double)((double)duration/NANO2MIL)/numLoops, ret);
#if 0
   printf(" Buffer [handlePos/last_position_ro_bench] => %d :\n %s \n\n", ret,  buffer);
#endif



   duration = 0;
   for(i=0; i<numLoops; i++)
   {
      clock_gettime(CLOCK_ID, &openStart);
      ret = pclKeyHandleReadData(hdl2, buffer, BUFFER_SIZE);
      clock_gettime(CLOCK_ID, &openEnd);

      duration += getNsDuration(&openStart, &openEnd);
   }
   printf(" Read 2 => %f ms [%d bytes]\n", (double)((double)duration/NANO2MIL)/numLoops, ret);
#if 0
   printf(" Buffer [handlePos/last_position_ro_bench2] => %d :\n %s \n\n", ret,  buffer);
#endif




   clock_gettime(CLOCK_ID, &openStart);
   hdl3 = pclKeyHandleOpen(0xFF, "handlePos/last_position_w_bench", 1, 2);
   clock_gettime(CLOCK_ID, &openEnd);
   duration = getNsDuration(&openStart, &openEnd);
   printf(" Open 3 => %f ms\n", (double)((double)duration/NANO2MIL));


   clock_gettime(CLOCK_ID, &openStart);
   hdl4 = pclKeyHandleOpen(0xFF, "handlePos/last_position_w_bench2", 1, 2);
   clock_gettime(CLOCK_ID, &openEnd);
   duration = getNsDuration(&openStart, &openEnd);
   printf(" Open 4 => %f ms\n", (double)((double)duration/NANO2MIL));


   pclKeyHandleClose(hdl);
   pclKeyHandleClose(hdl2);
   pclKeyHandleClose(hdl3);
   pclKeyHandleClose(hdl4);
}


void* do_something(void* dataPtr)
{
   int i = 0;
   int value = *((int*)dataPtr);
   unsigned int shutdownReg = PCL_SHUTDOWN_TYPE_FAST | PCL_SHUTDOWN_TYPE_NORMAL;

   // init library
   (void)pclInitLibrary(gAppName , shutdownReg);

   for(i=0; i < 5000; i++)
   {
      switch(value)
      {
      case 1:
         (void)pclKeyWriteData(0x84, "links/last_link2",  2, 1, (unsigned char*)"Test notify shared data", strlen("Test notify shared data"));
      case 2:
         (void)pclKeyWriteData(0x84, "links/last_link3",  3, 2, (unsigned char*)"Test notify shared data", strlen("Test notify shared data"));
      case 3:
         (void)pclKeyWriteData(0x84, "links/last_link4",  4, 1, (unsigned char*)"Test notify shared data", strlen("Test notify shared data"));
         break;
      default:
         printf("Nothing!\n;");
         break;
      }
   }

   // deinit library
   pclDeinitLibrary();

   return NULL;
}


int main(int argc, char *argv[])
{
   int ret = 0;

#if 0
   unsigned int shutdownReg = PCL_SHUTDOWN_TYPE_FAST | PCL_SHUTDOWN_TYPE_NORMAL;
   int numLoops = 5000;
   long long resolution = 0;

   struct timespec clockRes;
#else

   int toThread1 = 0, toThread2 = 0, toThread3 = 0;
   int* retval1, retval2, retval3;

   pthread_t thread1, thread2, thread3;

#endif


   struct tm *locTime;

   time_t t = time(0);
   locTime = localtime(&t);
   snprintf(sysTimeBuffer, BUFFER_SIZE, "The benchmark string to do write benchmarking: \"%s %.2d.%.2d.%d - %d:%.2d:%.2d Uhr\" [time and date]", dayOfWeek[locTime->tm_wday],
                                         locTime->tm_mday, (locTime->tm_mon)+1, (locTime->tm_year+1900),
                                         locTime->tm_hour, locTime->tm_min, locTime->tm_sec);

   snprintf(sysTimeBuffer2, BUFFER_SIZE, "The benchmark string to do write benchmarking: \"%s %.2d.%.2d.%d - %d:%.2d:%.2d Uhr\" [time and date]  ==> The benchmark string to do write benchmarking: The quick brown fox jumps over the lazy dog !!!",
                                         dayOfWeek[locTime->tm_wday],
                                         locTime->tm_mday, (locTime->tm_mon)+1, (locTime->tm_year+1900),
                                         locTime->tm_hour, locTime->tm_min, locTime->tm_sec);


   /// debug log and trace (DLT) setup
   DLT_REGISTER_APP("noty","tests the persistence client library");


#if 0
   printf("\n\n============================\n");
   printf("      PCL benchmark\n");
   printf("============================\n\n");

   clock_getres(CLOCK_ID, &clockRes);
   resolution = ((clockRes.tv_sec * SECONDS2NANO) + clockRes.tv_nsec);
   printf("Clock resolution  => %f ms\n\n", (double)((double)resolution/NANO2MIL));


   init_benchmark(1000);


   // init library
   (void)pclInitLibrary(gAppName , shutdownReg);

   read_benchmark(numLoops);

   write_benchmark(numLoops);

   handle_benchmark(numLoops);


   // deinit library
   pclDeinitLibrary();

#else

   toThread1 = 1;

   ret = pthread_create(&thread1, NULL, do_something, &toThread1);
   pthread_setschedprio(thread1, sched_get_priority_max(SCHED_OTHER));

   toThread2 = 2;
   ret = pthread_create(&thread2, NULL, do_something, &toThread2);
   pthread_setschedprio(thread2, sched_get_priority_max(SCHED_OTHER));

   toThread3 = 3;
   ret = pthread_create(&thread3, NULL, do_something, &toThread3);
   pthread_setschedprio(thread3, sched_get_priority_max(SCHED_OTHER));


   // wait until the dbus mainloop has ended
   pthread_join(thread1, (void**)&retval1);
   // wait until the dbus mainloop has ended
   pthread_join(thread2, (void**)&retval2);
   // wait until the dbus mainloop has ended
   pthread_join(thread3, (void**)&retval3);
#endif



   // unregister debug log and trace
   DLT_UNREGISTER_APP();

   dlt_free();

   return ret;
}
