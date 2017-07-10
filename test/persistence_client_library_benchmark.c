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
 * @ingroup        Persistence client library benchmark
 * @author         Ingo Huerner
 * @brief          Benchmark of persistence client library
 * @see
 */

#include "../include/persistence_client_library_key.h"
#include "../include/persistence_client_library_file.h"
#include "../include/persistence_client_library_error_def.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>     /* atoi */

#include <dlt.h>
#include <dlt_common.h>

#include <sys/time.h>
#include <sys/resource.h>
#include <pthread.h>


#define SECONDS2NANO 1000000000L
#define NANO2MIL        1000000L
#define MIL2SEC            1000L

#define BUFFER_SIZE  2048

// define for the used clock: "CLOCK_MONOTONIC" or "CLOCK_REALTIME"
#define CLOCK_ID  CLOCK_MONOTONIC

const char* gAppName = "lt-persistence_client_library_test";

extern const char* gWriteBuffer;
extern const char* gWriteBuffer2;


double gDurationMsWrite = 0, gSizeMbWrite = 0;
double gAverageBytesWrite = 0, gAverageTimeWrite = 0;
double gAverageBytesWriteSecond = 0, gAverageTimeWriteSecond = 0;
double gDurationRead = 0, gSizeRead = 0;
double gDurationReadSecond = 0, gSizeReadSecond = 0;
double gDurationInit = 0, gDurationDeinit = 0;


inline long long getNsDuration(struct timespec* start, struct timespec* end)
{
   return ((end->tv_sec * SECONDS2NANO) + end->tv_nsec) - ((start->tv_sec * SECONDS2NANO) + start->tv_nsec);
}


inline long getMsDuration(struct timespec* start, struct timespec* end)
{
   return (((end->tv_sec * SECONDS2NANO) + end->tv_nsec) - ((start->tv_sec * SECONDS2NANO) + start->tv_nsec))/NANO2MIL;
}



void init_benchmark(int numLoops)
{
   int i = 0;
   long long durationInit = 0;
   long long durationDeInit = 0;
   struct timespec initStart, initEnd;
   struct timespec deInitStart, deInitEnd;
   int shutdownReg = PCL_SHUTDOWN_TYPE_FAST | PCL_SHUTDOWN_TYPE_NORMAL;

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

   gDurationInit = (double)durationInit/(double)numLoops/(double)NANO2MIL;
   gDurationDeinit = (double)durationDeInit/(double)numLoops/(double)NANO2MIL;
}



void read_benchmark(int numLoops)
{
   int ret = 0, i = 0;
   long long duration = 0;
   long  size = 0;
   struct timespec readStart, readEnd;
   char key[128] = { 0 };
   unsigned char buffer[7168] = {0};   // 7kB
   int shutdownReg = PCL_SHUTDOWN_TYPE_NONE;

   (void)pclInitLibrary(gAppName , shutdownReg);

   //
   // populate data
   //
   for(i=0; i<numLoops; i++)
   {
      memset(key, 0, 128);
      snprintf(key, 128, "pos/last_position_w_bench%d",i);

      if(i%2)
      {
         ret = pclKeyWriteData(PCL_LDBID_LOCAL, key, 10, 10, (unsigned char*)gWriteBuffer, (int)strlen(gWriteBuffer) );
      }
      else
      {
         ret = pclKeyWriteData(PCL_LDBID_LOCAL, key, 10, 10, (unsigned char*)gWriteBuffer2, (int)strlen(gWriteBuffer2) );
      }
   }
   pclLifecycleSet(PCL_SHUTDOWN);
   (void)pclDeinitLibrary();


   //
   // read data
   //
   (void)pclInitLibrary(gAppName , shutdownReg);

   duration = 0;
   memset(buffer, 0, 7168);
   size = 0;

   for(i=0; i<numLoops; i++)
   {
      memset(key, 0, 128);

      snprintf(key, 128, "pos/last_position_w_bench%d",i);

      clock_gettime(CLOCK_ID, &readStart);
      ret = pclKeyReadData(PCL_LDBID_LOCAL, key, 10, 10, buffer, 7168);
      clock_gettime(CLOCK_ID, &readEnd);

      size += ret;
      duration += getNsDuration(&readStart, &readEnd);

      memset(buffer, 0, 7168);
   }

   pclLifecycleSet(PCL_SHUTDOWN);
   (void)pclDeinitLibrary();

   gDurationRead = (double)duration/(double)numLoops;
   gSizeRead     = (double)size/(double)numLoops/(double)1024;
}


void write_benchmark(int numLoops)
{
   int ret = 0, i = 0;
   long duration = 0,  size = 0;
   struct timespec readStart, readEnd;
   char key[128] = { 0 };
   unsigned char buffer[7168] = {0};   // 7kB

   int shutdownReg = PCL_SHUTDOWN_TYPE_NONE;

   (void)pclInitLibrary(gAppName , shutdownReg);

   duration = 0;
   memset(buffer, 0, 7168);
   size = 0;

   // write the first time to the data
   for(i=0; i <numLoops; i++)
   {
      memset(key, 0, 128);
      snprintf(key, 128, "pos/last_position_w_bench%d",i);

      if(i%2)
      {
         clock_gettime(CLOCK_ID, &readStart);
         ret = pclKeyWriteData(PCL_LDBID_LOCAL, key, 20, 20, (unsigned char*)gWriteBuffer, (int)strlen(gWriteBuffer) );
         clock_gettime(CLOCK_ID, &readEnd);
      }
      else
      {
         clock_gettime(CLOCK_ID, &readStart);
         ret = pclKeyWriteData(PCL_LDBID_LOCAL, key, 20, 20, (unsigned char*)gWriteBuffer2, (int)strlen(gWriteBuffer2));
         clock_gettime(CLOCK_ID, &readEnd);
      }

      size += ret;
      duration += getNsDuration(&readStart, &readEnd);

      memset(buffer, 0, 7168);
   }
   gAverageBytesWrite = (double)size/(double)numLoops/(double)1024;
   gAverageTimeWrite = (double)duration/(double)numLoops;


   // write the second time to the data
   duration = 0;
   size = 0;
   for(i=0; i <numLoops; i++)
   {
      memset(key, 0, 128);
      snprintf(key, 128, "pos/last_position_w_bench%d",i);

      if(i%2)
      {
         clock_gettime(CLOCK_ID, &readStart);
         ret = pclKeyWriteData(PCL_LDBID_LOCAL, key, 20, 20, (unsigned char*)gWriteBuffer2, (int)strlen(gWriteBuffer2) );
         clock_gettime(CLOCK_ID, &readEnd);
      }
      else
      {
         clock_gettime(CLOCK_ID, &readStart);
         ret = pclKeyWriteData(PCL_LDBID_LOCAL, key, 20, 20, (unsigned char*)gWriteBuffer, (int)strlen(gWriteBuffer));
         clock_gettime(CLOCK_ID, &readEnd);
      }

      size += ret;
      duration += getNsDuration(&readStart, &readEnd);

      memset(buffer, 0, 7168);
   }
   gAverageBytesWriteSecond = (double)size/(double)numLoops/(double)1024;
   gAverageTimeWriteSecond = (double)duration/(double)numLoops;


   // measure the write back time
   duration = 0;

   clock_gettime(CLOCK_ID, &readStart);
   pclLifecycleSet(PCL_SHUTDOWN);
   clock_gettime(CLOCK_ID, &readEnd);

   duration = getNsDuration(&readStart, &readEnd);
   gDurationMsWrite = (double)duration/(double)NANO2MIL;
                                 //  B to kB  -  kB to MB
   gSizeMbWrite     = (double)size/(double)1024/(double)1024;


   (void)pclDeinitLibrary();
}



void printAppManual()
{
   printf("\n\n==================================================================================\n");
   printf("NAME\n");
   printf("   ./persistence_client_library_benchmark - run PCL benchmarks");

   printf("\nSYNOPSIS\n");
   printf("   persistence_client_library_benchmark [-l loop] [-irwh]\n");

   printf("\nDESCRIPTION\n");
   printf("   Run persistence client library benchmarks.\n");
   printf("   If no option is passed, all benchmarks are run with default (1024) number of loops.\n");

   printf("\nOPTIONS\n");
   printf("   -l   number of loops for each test (init benchmark is bound to 10 loops)\n");
   printf("   -i   Run init/deinit benchmarks\n");
   printf("   -r   Run read benchmarks\n");
   printf("   -w   Run write benchmarks\n");
   printf("   -h   Display this help\n");
   printf("==================================================================================\n");
}



int main(int argc, char *argv[])
{
   int ret = 0;

   int numLoops = 1024;          // number of default loops
   long long resolution = 0;

   struct timespec clockRes;

   int opt = 0, doInit = 0, doRead = 0, doWrite = 0, printManual = 0;

   const char* envVariable = "PERS_CLIENT_LIB_CUSTOM_LOAD";

   setenv(envVariable, "/etc/pclCustomLibConfigFileTest.cfg", 1);

   if(argc <= 1)
   {
      // if no parameter, run all tests with default loops
      doInit  = 1;
      doRead  = 1;
      doWrite = 1;
      printManual = 1;
   }


   while ((opt = getopt(argc, argv, "l:irwh")) != -1)
   {
      switch (opt)
      {
         case 'l':
            numLoops = atoi(optarg);
            break;
         case 'i':
            doInit = 1;
            break;
         case 'r':
            doRead = 1;
            break;
         case 'w':
            doWrite = 1;
            break;
         case 'h':
            printManual = 1;
         break;
        }
    }

   /// debug log and trace (DLT) setup
   DLT_REGISTER_APP("PCLb","tests the persistence client library");

   clock_getres(CLOCK_ID, &clockRes);
   resolution = ((clockRes.tv_sec * SECONDS2NANO) + clockRes.tv_nsec);

   //
   // run benchmarks
   //

   if(doInit == 1)
      init_benchmark(10); // static number of loops, otherwise it takes to long

   if(doRead == 1)
      read_benchmark(numLoops);

   if(doWrite == 1)
      write_benchmark(numLoops);


   if(printManual == 1)
   {
      printAppManual();
   }

   printf("\n\n==================================================================================\n");
   printf("  PCL benchmark - num loop: %d - clock resolution: %f\n", numLoops, (double)((double)resolution));
   printf("==================================================================================\n");
   if(doInit == 1)
   {
      printf("Init benchmark\n");
      printf("  Init      => %.2f ms \n", gDurationInit);
      printf("  Deinit    => %.2f ms \n", gDurationDeinit);
   }
   else
   {
      printf("Init benchmark - not activated.\n");
   }
   printf("==================================================================================\n");
   if(doRead == 1)
   {
      printf("Read benchmark\n");
      printf("  Read      => %.0f ns for \t [%.2f Kilobytes item]\n", gDurationRead, gSizeRead);
      printf("  Read      => %.0f ns for \t [1 Kilobyte item]\n", (double)gDurationRead/(double)gSizeRead);
   }
   else
   {
      printf("Read benchmark - not activated.\n");
   }
   printf("==================================================================================\n");
   if(doWrite == 1)
   {
      printf("Write benchmark\n");
      printf("  Write 1.  => %.0f ns for \t [%.2f Kilobytes item]\n", gAverageTimeWrite, gAverageBytesWrite);
      printf("  Write 2.  => %.0f ns for \t [%.2f Kilobytes item]\n", gAverageTimeWriteSecond, gAverageBytesWriteSecond);
      printf("  Write 1.  => %.0f ns for \t [1 Kilobyte item]\n",    gAverageTimeWrite/gAverageBytesWrite);
      printf("  Write 2.  => %.0f ns for \t [1 Kilobyte item]\n",    gAverageTimeWriteSecond/gAverageBytesWriteSecond);
      printf("  Writeback => %.3f ms for \t [%.2f Megabytes item]\n", gDurationMsWrite, gSizeMbWrite);
      printf("  Writeback => %.3f ms for \t [1 Megabyte item]\n", (double)gDurationMsWrite/(double)gSizeMbWrite);

      printf("Explanation:\n");
      printf("  Writing the first time (1.) to an item takes more time.\n");
      printf("  As we use the copy an write procedure, we create an entry for this item \n");
      printf("  in the cache and are writing then to the cache.\n");
      printf("  Further writes (2.) are faster.\n");
      printf("  When the database will be close, all modified items will be written back\n");
      printf("  to non-volatile memory device.\n");
   }
   else
   {
      printf("Write benchmark - not activated.\n");
   }
   printf("==================================================================================\n");

   // unregister debug log and trace
   DLT_UNREGISTER_APP();

   dlt_free();

   return ret;
}



const char* gWriteBuffer =
   "---------> F I R S T write buffer starting block "
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
   "Die heiße Zypernsonne quälte Max und Victoria ja böse auf dem Weg bis zur Küste"
   "F I R S T  write buffer ending block <---------";

const char* gWriteBuffer2 =
      "---------> S E C O N D  write buffer starting block "
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
      "Die heiße Zypernsonne quälte Max und Victoria ja böse auf dem Weg bis zur Küste"
      "S E C O N D  write buffer ending block <---------";



