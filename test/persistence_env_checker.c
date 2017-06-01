/******************************************************************************
 * Project         Persistence
 * (c) copyright   2017
 * Company         Mentor Graphics
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

#include <dirent.h>
#include <sys/types.h>


static const char* gProcCmdLineTemplate = "/proc/%ld/cmdline";
static const char* gPersFolderCache     = "/Data/mnt-c";
static const char* gPersFolderWt        = "/Data/mnt-wt";
static const char* gConfigFilePathname  = "/etc/pclCustomLibConfigFileTest.cfg";


pid_t find_process_running(const char* processName)
{
    DIR* dir;
    struct dirent* ent;
    char* endptr;
    char buf[256] = {0};

    if (!(dir = opendir("/proc")))
    {
        printf("Failed to open /proc: %s\n", strerror(errno));
        return -1;
    }

    while((ent = readdir(dir)) != NULL)
    {
       FILE* fp = NULL;
        long lpid = strtol(ent->d_name, &endptr, 10);

        if (*endptr != '\0')
        {
            continue;
        }

        memset(buf, 0, 256);
        snprintf(buf, sizeof(buf), gProcCmdLineTemplate, lpid);

        fp = fopen(buf, "r");
        if(fp)
        {
            if(fgets(buf, sizeof(buf), fp) != NULL)
            {
                char* first = strtok(buf, " ");

                char* lastDelimiter = strrchr(first, '/');
                if( 0 != lastDelimiter )
                {
                  first = lastDelimiter+1;
                }

                if (!strcmp(first, processName))
                {
                    fclose(fp);
                    closedir(dir);
                    return (pid_t)lpid;
                }
            }
            fclose(fp);
        }
    }

    closedir(dir);
    return -1;
}


int check_NSM()
{
   if(find_process_running("NodeStateManager") == -1 )
   {
      printf("  \nFAILURE: GENIVI Node State Manager (NSM) is NOT running\n");
      printf("         Start with \"NodeStateManager\" (needs ROOT privileges)\n");

      return -1;
   }

   return 1;
}


int check_PAS()
{
   if(find_process_running("pers_admin_svc") == -1)
   {
      printf("\nFAILURE: GENIVI Persistence Administration Service (PAS) is NOT running\n");
      printf("         Start with \"pers_admin_svc\" (needs ROOT privileges)\n");

      return -1;
   }

   return 1;
}


int check_test_data()
{
   int rval = 1, doMoutcheck = 1;

   if(access("/Data/mnt-c/lt-persistence_client_library_test/cached.itz", F_OK))
   {
      printf("\nFAILURE: Test data for cached data is not available\n");
      printf("         Run \"persadmin_tool install /fullPathTofile/PAS_data.tar.gz\"\n");
      printf("         Make sure persistence data partition is available and mounted to /Data/mnt-c\n");

      rval = -1;
   }

   if(access("/Data/mnt-wt/lt-persistence_client_library_test/wt.itz", F_OK))
   {
      printf("\nFAILURE: Test data for write through is not available\n");
      printf("         Run \"persadmin_tool install /fullPathTofile/PAS_data.tar.gz\"\n");
      printf("         Make sure a persistence data partition is available and mounted to /Data/mnt-c\n");

      rval = -1;
   }

   if(doMoutcheck)
   {
      char str1[24] = {0}, str2[24] = {0}, str3[24] = {0},  str4[96] = {0}, str5[12] = {0}, str6[12] = {0};
      char deviceWt[24] = {0}, deviceC[24] = {0};

      FILE * fp = fopen("/proc/mounts", "r");
      if(fp != NULL)
      {
         memset(deviceWt, 0, 24);
         deviceWt[23] = '\0';

         memset(deviceC, 0, 24);
         deviceC[23] = '\0';

         while( fscanf(fp, "%23s %23s %23s %95s %11s %11s", str1, str2, str3, str4, str5, str6) !=  EOF )
         {
#if 0
            printf("1. : |%s|\n", str1 );
            printf("2. : |%s|\n", str2 );
            printf("3. : |%s|\n", str3 );
            printf("4. : |%s|\n", str4 );
            printf("5. : |%s|\n", str5 );
            printf("6. : |%s|\n\n", str6 );
#endif

            if(strncmp(gPersFolderCache, str2, strlen(gPersFolderCache)) == 0)
            {
               strncpy(deviceC, str1, strlen(str1));
            }

            if(strncmp(gPersFolderWt, str2, strlen(gPersFolderWt)) == 0)
            {
               strncpy(deviceWt, str1, strlen(str1));
            }
         }

         if(strncmp(deviceC, deviceWt, strlen(deviceWt)) != 0)
         {
            printf("FAILURE: Same partition must be mounted to /Data/mnt-c AND /Data/mnt-wt\n");
            printf("         The persistence partition: \"%s\" - \"%s\" \n", deviceWt, deviceC);

            rval = -1;
         }
      }
   }

   if(rval == -1)
   {
      printf("\nNOTE: There must be ONE partition available for persistent data\n");
      printf("      which must be mounted to /Data/mnt-c AND also to /Data/mnt-wt\n\n");
   }

   return rval;
}


int check_plugin_config()
{
   int rval = 1;

   if(access(gConfigFilePathname, F_OK))
   {
      printf("\nFAILURE: Config file for plugins not available\n");
      printf("           Please make sure the provided config file is available uner /etc/\n");
      printf("           Config file \"pclCustomLibConfigFileTest.cfg\"is provided in the folder \"config\"\n");

      rval = -1;
   }
   else
   {
      char plugType[24], libNameAndPath[64], str3[24],  str4[24];

      FILE * fp = fopen (gConfigFilePathname, "r");
      if(fp != NULL)
      {
         while( fscanf(fp, "%23s %63s %23s %23s", plugType, libNameAndPath, str3, str4) !=  EOF )
         {
            if(strncmp("anInvalidEntry", plugType, strlen("anInvalidEntry"))) // ignore invalid test data entry
            {
               if(access(libNameAndPath, F_OK))
               {
                  printf("\nFAILURE: plugin library not found: \"%s\"\n", libNameAndPath);
                  rval = -1;
               }
            }
#if 0
            else
            {
               printf("Found invalid test data: %s\n",plugType );
            }

            printf("Read type      : |%s|\n", plugType );
            printf("Read lib       : |%s|\n", libNameAndPath );
            printf("Read load type : |%s|\n", str3 );
            printf("Read sync/async: |%s|\n\n", str4 );
#endif
         }
         fclose(fp);
      }
      else
      {
         printf("Failed to open config file: %s\n", strerror(errno));
      }
   }

   if(rval == -1)
   {
      printf("\nNOTE: Please check config file under \"/etc/pclCustomLibConfigFileTest.cfg\"\n");
      printf("      and adjust config file or make sure requested pligin library is available under the correct path\n");
   }
   return rval;
}


int check_environment()
{
   int rval = 1;

   if(check_NSM() != 1)       // NSM running?
      rval = -1;

   if(check_PAS() != 1)          // PAS running?
      rval = -1;

   if(check_test_data() != 1)       // test data installed?
      rval = -1;

   if(check_plugin_config() != 1)   // plugin libraries available?
      rval = -1;

   return rval;
}
