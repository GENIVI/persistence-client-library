/******************************************************************************
 * Project         Persistency
 * (c) copyright   2016
 * Company         Mentor Graphics
 *****************************************************************************/
/******************************************************************************
 * This Source Code Form is subject to the terms of the
 * Mozilla Public License, v. 2.0. If a  copy of the MPL was not distributed
 * with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
******************************************************************************/
/**
 * @file           persistence_db_viewer.c
 * @ingroup        Persistence client library tools
 * @author         Ingo Huerner
 *
 * @see
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>

#include <dlt.h>
#include <dlt_common.h>

#include <persComRct.h>
#include <persComDbAccess.h>


DltContext gPclTestDLTContext;

/// debug log and trace (DLT) setup
DLT_DECLARE_CONTEXT(gPclTestDLTContext)

#define STRING_SIZE    512
#define APPNAME_SIZE    64

#define FILE_DIR_NOT_SELF_OR_PARENT(s) ((s)[0]!='.'&&(((s)[1]!='.'||(s)[2]!='\0')||(s)[1]=='\0'))

static char* policy[]     = {"cache",     "wt",       "na" };
static char* storage[]    = {"local",     "shared",   "custom",    "na"};
static char* permission[] = {"ReadWrite", "ReadOnly", "WriteOnly", "na"};


typedef enum dbType_
{
   dbTypeNone         = 0x00,
   dbTypeCached       = 0x01,
   dbTypeWriteThrough = 0x02,
   dbTypeDefault      = 0x04,
   dbTypeConfDefault  = 0x08,
   dbTypeRCT          = 0x10,
   dbTypeAll          = 0x1F,
   dbTypeLast         = 0x20,
} dbType;


unsigned int gDbMaskArray[] = {
   dbTypeNone,
   dbTypeCached,
   dbTypeWriteThrough,
   dbTypeDefault,
   dbTypeConfDefault,
   dbTypeRCT
};

const char* gDbNameArray[] = {
   "none",
   "cached",
   "wt",
   "default",
   "confDefault",
   "RCT"
};


void printRCTcfg(PersistenceConfigurationKey_s config)
{
   if(config.policy <= 2)
      printf("   PersistencePolicy_e    : %s\n", policy[(unsigned int)config.policy]);

   if(config.storage <= 3)
      printf("   PersistenceStorage_e   : %s\n", storage[(unsigned int)config.storage]);

   if(config.type <= 3)
      printf("   PersistencePermission_e: %s\n", permission[(unsigned int)config.type]);

   printf("   max_size               : %d\n", config.max_size);
   printf("   reponsible             : %s\n", config.reponsible);
   printf("   custom_name            : %s\n", config.custom_name);
   printf("   customID               : %s\n\n", config.customID);
}



void printDBcontent(const char* appname, dbType type)
{
   int handle = 0,  listSize = 0, ret = 0;

   char* resourceList = NULL;
   char filename[512] = { 0 };

   static const char* nameTemplate = "/Data/mnt-c/%s/%s";
   static const char* cache   = "cached.itz";
   static const char* wt      = "wt.itz";
   static const char* def     = "default-data.itz";
   static const char* confDef = "configurable-default-data.itz";
   static const size_t bufSize = 8192;

   printf("---------------------------------------------------------------------\n");
   if(type == dbTypeCached)
   {
      snprintf(filename, 512, nameTemplate, appname, cache);
      printf("Cachded DB keys: %s\n", filename);
   }
   else if(type == dbTypeWriteThrough)
   {
      snprintf(filename, 512, nameTemplate, appname, wt);
      printf("Write Through DB keys: %s\n", filename);
   }
   else if(type == dbTypeDefault)
   {
      snprintf(filename, 512, nameTemplate, appname, def);
      printf("Default DB keys: %s\n", filename);
   }
   else if(type == dbTypeConfDefault)
   {
      snprintf(filename, 512, nameTemplate, appname, confDef);
      printf("Default Config DB keys: %s\n", filename);
   }
   else
   {
      printf("printDBcontent ==> wrong type, valid is 0 (cached) or 1 (write through)\n");
      return;
   }
   printf("---------------------------------------------------------------------\n");

   handle = persComDbOpen(filename, 0x0); //create rct.db if not present
   if(handle >= 0)
   {
      listSize = persComDbGetSizeKeysList(handle);
      if(listSize > 0)
      {
         resourceList = (char*)malloc((size_t)listSize);
         memset(resourceList, 0, (size_t)listSize);

         if(resourceList != NULL)
         {
            ret = persComDbGetKeysList(handle, resourceList, listSize);
            if(ret != 0)
            {
               int i = 0, idx = 0, numResources = 0;
               int resourceStartIdx[256] = { 0 };
               char buffer[bufSize];

               for(i = 1; i < listSize; ++i)
               {
                  if(resourceList[i] == '\0')
                  {
                     numResources++;
                     resourceStartIdx[++idx] = i + 1;
                  }
               }

               printf("NumOf resources: %d \n", numResources);

               for(i = 0; i < numResources; ++i)
               {
                  printf("Key[%d]: %s\n", i, &resourceList[resourceStartIdx[i]]);

                  memset(buffer, 0, sizeof(buffer));
                  persComDbReadKey(handle,  &resourceList[resourceStartIdx[i]], buffer, (int)sizeof(buffer));

                  printf(" value: %s\n\n", buffer);
               }
            }

            free(resourceList);
            printf("---------------------------------------------------------------------\n");
         }
      }
      else
      {
         printf("* Database is empty!! *\n\n");
      }

      ret = persComDbClose(handle);
      if(ret != 0)
         printf("Failed to close db\n");
   }
   else
   {
      printf("Failed to open\n");
   }
}



void printRCTcontent(const char* appname, int full)
{
   int handle = -1, listSize = 0, ret = 0;

   char* resourceList = NULL;
   const char* filenamneTemplate = "/Data/mnt-c/%s/resource-table-cfg.itz";
   char filename[512] = {0};

   memset(filename, 0, 512);

   snprintf(filename, 512, filenamneTemplate, appname);

   handle = persComRctOpen(filename, 0x0); //don't create rct.db if not present
   if(handle >= 0)
   {
      listSize = persComRctGetSizeResourcesList(handle);
      if(listSize > 0)
      {
         resourceList = (char*)malloc((size_t)listSize);
         memset(resourceList, 0, (size_t)listSize);

         if(resourceList != NULL)
         {
            ret = persComRctGetResourcesList(handle, resourceList, listSize);
            if(ret != 0)
            {
               int i = 0, idx = 0, numResources = 0;
               int resourceStartIdx[256] = { 0 };

               for(i = 1; i < listSize; ++i)
               {
                  if(resourceList[i]  == '\0')
                  {
                     numResources++;
                     resourceStartIdx[++idx] = i + 1;
                  }
               }

               printf("---------------------------------------------------------------------\n");
               printf("RCT keys [%d]: %s \n", numResources, filename);
               printf("---------------------------------------------------------------------\n");

               for(i = 0; i < numResources; ++i)
               {
                  PersistenceConfigurationKey_s psConfig_out;
                  printf("RCT[%d]                    : \"%s\"\n", i, &resourceList[resourceStartIdx[i]]);

                  if(full == 1)
                  {
                     persComRctRead(handle, &resourceList[resourceStartIdx[i]], &psConfig_out);
                     printRCTcfg(psConfig_out);
                  }
               }
            }

            free(resourceList);
            printf("---------------------------------------------------------------------\n");
         }
      }
      else
      {
         printf("* Database is empty!! *\n\n");
      }

      ret = persComRctClose(handle);
      if(ret != 0)
         printf("Failed to close db\n");
   }
   else
   {
      printf("Failed to open\n");
   }
}



void printAppManual()
{
   printf("\nNAME\n");
   printf("   ./persistence_db_viewer - display database content of different persistence applications\n");

   printf("\nSYNOPSIS\n");
   printf("   persistence_db_viewer [-p path] [-lacwdfr] -n appname \n");

   printf("\nDESCRIPTION\n");
   printf("   Display content of persistent databases like cached, write through and default value database,\n");
   printf("   as well as content of resource configuration table\n");

   printf("\nOPTIONS\n");
   printf("   -p   Path to the persistence storage location [default location is \"/Data/mnt-c/\"]\n");
   printf("   -n   Name of the application to display the database [use \"all\" to display content of all available applications]\n");
   printf("   -l   List all available applications under persistence storage locations\n");
   printf("   -a   Display content of all database types\n");
   printf("   -c   Display cached database content\n");
   printf("   -w   Display writhe though database content\n");
   printf("   -d   Display default value database content\n");
   printf("   -f   Display configurable default value database content\n");
   printf("   -r   Display resource configuration table database content\n");

   printf("\n");
}



void printSingleApplicationDBs(const char* appname, const char* thePath, unsigned int databaseTypes)
{
   printf("=====================================================================================\n");
   printf("* %s - %s\n", appname, thePath);

   if(databaseTypes != dbTypeNone)
   {
      int i = 0;
      for(i=1; i<=5; i++)
      {
         if(databaseTypes & gDbMaskArray[i]) // check if db content needs to be printed
         {
            if(gDbMaskArray[i] != dbTypeRCT)
            {
               printDBcontent(appname, gDbMaskArray[i]);
            }
            else
            {
               printRCTcontent(appname, 1);
            }
         }
      }
   }
   else
   {
      printf("Print no database database content\n");
   }

   printf("=====================================================================================\n\n");
}



void printAllApplicationDBs(const char* thePath, unsigned int databaseTypes)
{
   struct dirent *dirent = NULL;

   DIR *dir = opendir(thePath);
   if(NULL != dir)
   {
      for(dirent = readdir(dir); NULL != dirent; dirent = readdir(dir))
      {
         if(FILE_DIR_NOT_SELF_OR_PARENT(dirent->d_name))
         {
            printSingleApplicationDBs(dirent->d_name, thePath, databaseTypes);
         }
      }
      closedir(dir);
   }
   else
   {
      printf("Print database content - Failed to open directory: \"%s\"\n\n", strerror(errno));
   }
}



void printAvailableApplication(const char* thePath)
{
   struct dirent *dirent = NULL;

   DIR *dir = opendir(thePath);
   if(NULL != dir)
   {
      printf("=====================================================================================\n\n");
      printf("Available applications: %s\n", thePath);
      printf("=====================================================================================\n");

      for(dirent = readdir(dir); NULL != dirent; dirent = readdir(dir))
      {
         if(FILE_DIR_NOT_SELF_OR_PARENT(dirent->d_name))
         {
            printf(" - %s\n", dirent->d_name);
         }
      }
      closedir(dir);

      printf("=====================================================================================\n\n");
   }
   else
   {
      printf("List applications - Failed to open directory: \"%s\"\n\n", strerror(errno));
   }
}



int main(int argc, char *argv[])
{
   int ret = 0, opt = 0, printAllAppsContent = -1, listAllApps = -1;
   unsigned int dbTypeAccum = dbTypeNone;
   static const char* defaultPersPath = "/Data/mnt-c/";

   char persPath[128] = {0};
   char singleAppName[APPNAME_SIZE] = {0};

   memset(persPath,0,128);
   strncpy(persPath, defaultPersPath, 128-1);

   /// debug log and trace (DLT) setup
   DLT_REGISTER_APP("PCLt","tests the persistence client library");

   DLT_REGISTER_CONTEXT(gPclTestDLTContext,"PTE","Context for testing persistence client library logging");

   while ((opt = getopt(argc, argv, "p:n:lacwdfr")) != -1)
   {
      switch (opt)
      {
         case 'p':
            memset(persPath,0,128);
            strncpy(persPath, optarg, APPNAME_SIZE-1);
         break;
         case 'l':
            listAllApps = 1;
            break;
         case 'n':
            if(strcmp(optarg, "all")  == 0)
            {
               printAllAppsContent = 1;
            }
            else
            {
               printAllAppsContent = 0;
               strncpy(singleAppName, optarg, APPNAME_SIZE-1);
            }
            break;
         case 'a': dbTypeAccum |= dbTypeAll;
            break;
         case 'c': dbTypeAccum |= dbTypeCached;
           break;
         case 'w': dbTypeAccum |= dbTypeWriteThrough;
           break;
         case 'd': dbTypeAccum |= dbTypeDefault;
           break;
         case 'f': dbTypeAccum |= dbTypeConfDefault;
           break;
         case 'r': dbTypeAccum |= dbTypeRCT;
           break;
         default: /* '?' */
            printAppManual();
            ret = -1;
      }
   }

   if(listAllApps == -1 && printAllAppsContent == -1)
   {
      printAppManual();
   }

   if(listAllApps == 1)
   {
      printAvailableApplication(persPath);
   }

   if(printAllAppsContent == 1)
   {
      printAllApplicationDBs(persPath, dbTypeAccum);
   }
   else if(printAllAppsContent == 0)
   {
      printSingleApplicationDBs(singleAppName, persPath, dbTypeAccum);
   }

   // unregister debug log and trace
   DLT_UNREGISTER_APP();

   dlt_free();

   return ret;
}
