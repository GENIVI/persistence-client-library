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
 * @file           persistence_client_tool.c
 * @ingroup        Persistence client library test tool
 * @author         Ingo Huerner
 * @brief          Persistence Client Library Test Tool
 * @see            
 */


#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <persistence_client_library_file.h>
#include <persistence_client_library_key.h>
#include <persistence_client_library.h>
#include <persistence_client_library_error_def.h>

#include <dlt.h>
#include <dlt_common.h>


#define PCLT_VERSION "0.1"

typedef enum eOperationMode_
{
  modeReadKey = 0,
  modeWriteKey,
  modeDeleteKey,
  modeGetKeySize,
  modeInvalid,
} eOperationMode;


// forward declaration
void printSynopsis();



void printHexDump(unsigned char* buffer, int formatNumPerRow)
{
   int i=0;
   size_t len = strlen((const char*)buffer);
   if(len != 0)
   {
      int j=0;
      printf("\n   HEXDUMP:\n");
      printf("   ---------------------------------------------------------\n   [%3d] - ", formatNumPerRow * (j));

      for(i=0; i<(int)len; i++)
      {
         printf("%x ", buffer[i]);
         if((i+1) % formatNumPerRow == 0)
         {
            j++;
            printf("\n   [%3d] - ", formatNumPerRow * (j));
         }
      }
      printf("\n   ---------------------------------------------------------\n\n");
   }
}



int readKey(char * resource_id, unsigned int user, unsigned int seat, unsigned int ldbid, unsigned int doHexdump, unsigned char* buffer, unsigned int size)
{
   int rval = 0;
   int numPerRow = 8;

   printf("- %s - User: %u - Seat: %u - ldbid: 0x%x\n\n", __FUNCTION__,  user, seat, ldbid);
   printf("   ResourceID: \"%s\" \n", resource_id);

   rval = pclKeyReadData(ldbid, resource_id, user, seat, buffer, size);
   if(rval >=0)
      printf("   Data      : \"%s \"\n", buffer);
   else
      printf("Failed to read: %d\n", rval);

   if(doHexdump == 1)
   {
      if(size >= 120)
         numPerRow = 16;

      printHexDump(buffer, numPerRow);
   }

   return rval;
}



int writeKey(char * resource_id, unsigned int user, unsigned int seat, unsigned int ldbid, unsigned char* buffer, unsigned int doHexdump)
{
   int rval = 0, size = 0;
   int numPerRow = 8;

   printf("- %s - User: %u - Seat: %u - ldbid: 0x%x\n\n", __FUNCTION__,  user, seat, ldbid);
   printf("   ResourceID: \"%s\" \n", resource_id);
   printf("   Data      : \"%s \"\n", buffer);

   size = pclKeyWriteData(ldbid, resource_id, user, seat, buffer, strlen((char*)buffer));

   if(doHexdump == 1)
   {
      if(size >= 120)
         numPerRow = 16;

      printHexDump(buffer, numPerRow);
   }

   return rval;
}



int deletekey(char * resource_id, unsigned int user, unsigned int seat, unsigned int ldbid)
{
   int rval = 0;

   printf("%s - ResourceID: \"%s\" - User: %u - Seat: %u - ldbid: 0x%x\n", __FUNCTION__, resource_id, user, seat, ldbid);

   rval = pclKeyDelete(ldbid, resource_id, user, seat);

   if(rval < 0)
      printf("Failed to deletekey: %d\n", rval);

   return rval;
}



int getkeysize(char * resource_id, unsigned int user, unsigned int seat, unsigned int ldbid)
{
   int rval = 0;

   printf("- %s - User: %u - Seat: %u - ldbid: 0x%x\n\n", __FUNCTION__,  user, seat, ldbid);
   printf("   ResourceID: \"%s\" \n", resource_id);

   rval = pclKeyGetSize(ldbid, resource_id,user, seat);
   printf("   KeySize: %d\n", rval);

   return rval;
}



int writeDataToFile(char* fileName, unsigned char* buffer, int size)
{
   int rval = 0;

   int fd = open(fileName, O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);

   if(fd != -1)
   {
      rval = ftruncate(fd, size);
      if(rval != -1)
      {
         rval = write(fd, buffer, size);

         if(rval == -1)
         {
            printf("Failed to write to file: %s - %s\n", fileName, strerror(errno));
         }
      }

      close(fd);
      fd = -1;
   }
   else
   {
      printf("Failed to open file: %s - %s\n", fileName, strerror(errno));
   }

   return rval;
}


unsigned char* readDataFromFile(char* fileName)
{
   unsigned char* writeBuffer = NULL;
   struct stat buffer;

   memset(&buffer, 0, sizeof(buffer));
   if(stat(fileName, &buffer) != -1)
   {
     if(buffer.st_size > 0)     // check for empty file
     {
        int fd = open(fileName, O_RDONLY);
        if(fd != -1)
        {
           int readSize = 0;
           writeBuffer =  malloc(buffer.st_size );
           if(writeBuffer != NULL)
           {
              readSize = read(fd, writeBuffer, buffer.st_size-1);   //-1 - just read content, not line endings
              if(readSize < 0)
              {
                 printf("Failed to read data: %d\n", readSize);
              }
           }
           else
           {
              printf("Failed to allocate memory\n");
              exit(EXIT_FAILURE);
           }
           close(fd);
           fd = -1;
        }
        else
        {
           printf("Failed to open file: %s - %s\n", fileName, strerror(errno));
        }
     }
     else
     {
        printf("Empty file\n");
        exit(EXIT_FAILURE);
     }
   }
   else
   {
     printf("Failed to stat file\n");
     exit(EXIT_FAILURE);
   }

   return writeBuffer;
}



int main(int argc, char *argv[])
{
	int opt;
	char* appName = NULL;
	char* resourceID = NULL;
	char* payloadBuffer = NULL;
	char* fileName = NULL;
	unsigned char* writeBuffer = NULL;
	eOperationMode opMode = modeInvalid;
	unsigned int user_no = 0, seat_no = 0;
	unsigned int ldbid = 0xFF;    // default value
	unsigned int doHexdump = 0;

	printf("\n");
   /// debug log and trace (DLT) setup
   DLT_REGISTER_APP("Ptool","persistence client library tools");


	while ((opt = getopt(argc, argv, "hVo:a:u:s:r:-l:p:f:H")) != -1)
	{
		switch (opt)
		{
		   case 'o':      // option
		      if(strcmp(optarg, "readkey")  == 0)
            {
		         opMode = modeReadKey;
            }
		      else if(strcmp(optarg, "writekey")  == 0)
            {
		         opMode = modeWriteKey;
            }
		      else if(strcmp(optarg, "deletekey")  == 0)
            {
		         opMode = modeDeleteKey;
            }
		      else if(strcmp(optarg, "getkeysize")  == 0)
            {
		         opMode = modeGetKeySize;
            }
		      else
		      {
		         printf("Unsupported Unsupported mode: %s\"\n\"", optarg);
		         printSynopsis();
	            exit(EXIT_FAILURE);
		      }
		      break;
		   case 'a':   // application name
		   {
		      size_t len = strlen(optarg);
            appName = malloc(len);
            if(appName != NULL)
            {
               memset(appName, 0, len);
               strncpy(appName, optarg, len);
            }
		   }
		      break;
		   case 'r':   // resource ID
         {
            size_t len = strlen(optarg);
            resourceID = malloc(len);
            if(resourceID != NULL)
            {
               memset(resourceID, 0, len);
               strncpy(resourceID, optarg, len);
            }
         }
            break;
         case 'p':   // payload to write
         {
            size_t len = strlen(optarg);
            payloadBuffer = malloc(len);
            if(payloadBuffer != NULL)
            {
               memset(payloadBuffer, 0, len);
               strncpy(payloadBuffer, optarg, len);
            }
         }
            break;
         case 'f':   // filename to read data from, write data to
         {
            size_t len = strlen(optarg);
            fileName = malloc(len);
            if(fileName != NULL)
            {
               memset(fileName, 0, len);
               strncpy(fileName, optarg, len);
            }
         }
            break;
		   case 'u':   // user number
		      user_no = (unsigned int)atoi(optarg);
		      break;
		   case 's':   // seat number
            seat_no = (unsigned int)atoi(optarg);
            break;
		   case 'l':
		      ldbid = (unsigned int)strtol(optarg, NULL, 16);
		      break;
		   case 'H':   // hexdump of data
		      doHexdump = 1;
		      break;
	   	case 'h':   // help
	   	   printSynopsis();
	         break;
	   	case 'v':   // version
	   		printf("Version: %s\n", PCLT_VERSION);
	         break;
	   	default: /* '?' */
	      	printSynopsis();
	         exit(EXIT_FAILURE);
	         break;
		}
   }


	if(appName != NULL && resourceID != NULL)
	{
	   printf("Application name: %s\n", appName);

	   unsigned int shutdownReg = PCL_SHUTDOWN_TYPE_FAST | PCL_SHUTDOWN_TYPE_NORMAL;
	   (void)pclInitLibrary(appName, shutdownReg);

      switch(opMode)
      {
         case modeReadKey:
         {
            unsigned char* buffer = NULL;
            int keysize = pclKeyGetSize(ldbid, resourceID,user_no, seat_no);

            if(keysize > 0)
            {
               buffer = malloc(keysize);
               if(buffer != NULL)
               {
                  memset(buffer, 0, keysize-1);
                  readKey(resourceID, user_no, seat_no, ldbid, doHexdump, buffer, keysize);

                  if(fileName != NULL)
                     (void)writeDataToFile(fileName, buffer, keysize);

                  free(buffer);
               }
            }
            else
            {
               printf("readkey: key is empty: %d\n", keysize);
            }
            break;
         }
         case modeWriteKey:
            if(fileName != NULL)    // if filename is available, read data from file
            {
               writeBuffer = readDataFromFile(fileName);
            }
            else
            {
               writeBuffer = (unsigned char*)payloadBuffer;    // use data from payload parameter
            }

            if(writeBuffer != NULL)
            {
               writeKey(resourceID, user_no, seat_no, ldbid, writeBuffer, doHexdump);
            }
            else
            {
               printf("No Data to write to key\n");
            }
            break;
         case modeDeleteKey:
            deletekey(resourceID, user_no, seat_no, ldbid);
            break;
         case modeGetKeySize:
            getkeysize(resourceID, user_no, seat_no, ldbid);
            break;
         default:
            printSynopsis();
            break;
      }

      if(appName != NULL)
         free(appName);

      if(resourceID != NULL)
         free(resourceID);

      if(writeBuffer != NULL)
         free(writeBuffer);

      if(fileName != NULL)
         free(fileName);


      pclDeinitLibrary();
	}
	else
	{
	   printf("Invalid application name or resourceID\n");
	   exit(EXIT_FAILURE);
	}

   // unregister debug log and trace
   DLT_UNREGISTER_APP();
   dlt_free();

   printf("\n");

   return 0;
}



void printSynopsis()
{
	printf("Usage: persclient_tool [-o <action to do>] [-a <application name>] [-r <resource id>] [-l <logical db id>]\n");
	printf("                       [-u <user no>] [-s <seat no>] [-f <file>] [-p <payload>] [-H] [-h] [-v]\n");

	printf("\n");
	printf("-o, --option=<action to do>   The possible actions are:\n");
	printf("                              readkey    - read out the given key\n");
	printf("                              writekey   - write something to a key\n");
	printf("                              deletekey  - delete a key\n");
	printf("                              getkeysize - get the size of a key\n");
	printf("-a, --appname=<application name>   The Application Name used for the initialization of the PCL\n");
	printf("-f, --file=<filename>              The file for data import or export\n");
	printf("-p, --payload=<payload>            The data to be written to a key if no file is specified as source.\n");
	printf("-r, --resource_id=<resource id>    The resource ID is the name of the key to process\n");
	printf("-l, --ldbid=<logical db id>        Logical Database ID in hex notation! e.g. 0xFF. If not specified the default value '0xFF' is used\n");
	printf("-u, --user_no=<user no>            The user number. If not specified the default value '0' is used\n");
	printf("-s, --seat_no=<seat no>            The seat number. If not specified the default value '0' is used\n");
	printf("-H, --forcehexdump                 Force print out a HexDump of the written/read data\n");
	printf("-h, --help                         Print help message\n");
	printf("-V, --version                      Print program version\n");

	printf("\n");
	printf("Examples:\n");
   printf("1.) Read a Key and show value as HexDump:\n");
	printf("    persclient_tool -o readkey -a MyApplication -r MyKey                   optional parameters: [-l 0xFF -u 0 -s 0]\n");
	printf("2.) Read a Key into a file:\n");
	printf("    persclient_tool -o readkey -a MyApplication -r MyKey -f outfile.bin    optional parameters: [-l 0xFF -u 0 -s 0 -H]\n");
	printf("3.) Write a Key and use the <payload> as the data source.:\n");
	printf("    persclient_tool -o writekey -a MyApplication -r MyKey -p 'Hello World' optional parameters: [-l 0xFF -u 0 -s 0 -H]\n");
	printf("4.) Write a Key and use a file as the data source.:\n");
	printf("    persclient_tool -o writekey -a MyApplication -r MyKey -f infile.bin    optional parameters: [-l 0xFF -u 0 -s 0 -H]\n");
	printf("5.) Get the size of a key [bytes]:\n");
	printf("    persclient_tool -o getkeysize -a MyApplication -r MyKey                optional parameters: [-l 0xFF -u 0 -s 0]\n");
	printf("6.) Delete a key:\n");
	printf("    persclient_tool -o deletekey -a MyApplication -r MyKey                 optional parameters: [-l 0xFF -u 0 -s 0]\n");
}

