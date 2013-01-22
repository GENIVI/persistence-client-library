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
 * @file           persistence_client_library_file.c
 * @ingroup        Persistence client library
 * @author         Ingo Huerner
 * @brief          Implementation of the persistence client library.
 *                 Library provides an API to access persistent data
 * @see            
 */

#include "persistence_client_library_file.h"
#include "../include_protected/persistence_client_library_data_organization.h"
#include "../include_protected/persistence_client_library_db_access.h"

#include "persistence_client_library_pas_interface.h"
#include "persistence_client_library_handle.h"
#include "persistence_client_library_prct_access.h"

#include <fcntl.h>   // for open flags
#include <errno.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>



int pclFileClose(int fd)
{
   int rval = -1;

   if(fd < MaxPersHandle)
   {
      __sync_fetch_and_sub(&gOpenFdArray[fd], FileClosed);   // set closed flag
      rval = close(fd);
   }
   return rval;
}



int pclFileGetSize(int fd)
{
   int rval = -1;

   struct stat buf;
   int ret = 0;
   ret = fstat(fd, &buf);

   if(ret != -1)
   {
      rval = buf.st_size;
   }
   return rval;
}



void* pclFileMapData(void* addr, long size, long offset, int fd)
{
   void* ptr = 0;

   if(AccessNoLock != isAccessLocked() ) // check if access to persistent data is locked
   {
      int mapFlag = PROT_WRITE | PROT_READ;
      ptr = mmap(addr,size, mapFlag, MAP_SHARED, fd, offset);
   }
   else
   {
      ptr = EPERS_MAP_LOCKFS;
   }
   return ptr;
}



int pclFileOpen(unsigned int ldbid, const char* resource_id, unsigned int user_no, unsigned int seat_no)
{
   int handle = -1, shared_DB = 0, flags = O_RDWR;

   PersistenceInfo_s dbContext;

   char dbKey[DbKeyMaxLen];      // database key
   char dbPath[DbPathMaxLen];    // database location

   memset(dbKey, 0, DbKeyMaxLen);
   memset(dbPath, 0, DbPathMaxLen);

   dbContext.context.ldbid   = ldbid;
   dbContext.context.seat_no = seat_no;
   dbContext.context.user_no = user_no;

   // get database context: database path and database key
   shared_DB = get_db_context(&dbContext, resource_id, ResIsFile, dbKey, dbPath);

   if(shared_DB != -1)  // check valid database context
   {
      handle = open(dbPath, flags);
      if(handle != -1)
      {
         if(handle < MaxPersHandle)
         {
            __sync_fetch_and_add(&gOpenFdArray[handle], FileOpen); // set open flag
         }
         else
         {
            close(handle);
            handle = EPERS_MAXHANDLE;
         }
      }
      else
      {
         // file does not exist, create file and folder

         const char* delimiters = "/\n";   // search for blank and end of line
         char* tokenArray[24];
         char createPath[DbPathMaxLen];
         int numTokens = 0;
         int i = 0;
         int validPath = 1;

         tokenArray[numTokens++] = strtok(dbPath, delimiters);
         while(tokenArray[numTokens-1] != NULL )
         {
           tokenArray[numTokens] = strtok(NULL, delimiters);
           if(tokenArray[numTokens] != NULL)
           {
              numTokens++;
              if(numTokens >= 24)
              {
                 validPath = 0;
                 break;
              }
           }
           else
           {
              break;
           }
         }

         if(validPath == 1)
         {
            memset(createPath, 0, DbPathMaxLen);
            snprintf(createPath, DbPathMaxLen, "/%s",tokenArray[0] );
            for(i=1; i<numTokens-1; i++)
            {
               // create folders
               strncat(createPath, "/", DbPathMaxLen-1);
               strncat(createPath, tokenArray[i], DbPathMaxLen-1);
               mkdir(createPath, 0744);
            }
            // finally create the file
            strncat(createPath, "/", DbPathMaxLen-1);
            strncat(createPath, tokenArray[i], DbPathMaxLen-1);
            handle = open(createPath, O_CREAT|O_RDWR |O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
            if(handle != -1)
            {
               if(handle < MaxPersHandle)
               {
                  __sync_fetch_and_add(&gOpenFdArray[handle], FileOpen); // set open flag
               }
               else
               {
                  close(handle);
                  handle = EPERS_MAXHANDLE;
               }
            }
         }
         else
         {
            printf("file_open ==> no valid path to create: %s\n", dbPath);
         }
      }
   }

   return handle;
}



int pclFileReadData(int fd, void * buffer, int buffer_size)
{
   return read(fd, buffer, buffer_size);
}



int pclFileRemove(unsigned int ldbid, const char* resource_id, unsigned int user_no, unsigned int seat_no)
{
   int rval = 0;

   if(AccessNoLock != isAccessLocked() ) // check if access to persistent data is locked
   {
      int shared_DB = 0;
      PersistenceInfo_s dbContext;

      char dbKey[DbKeyMaxLen];      // database key
      char dbPath[DbPathMaxLen];    // database location

      memset(dbKey, 0, DbKeyMaxLen);
      memset(dbPath, 0, DbPathMaxLen);

      dbContext.context.ldbid   = ldbid;
      dbContext.context.seat_no = seat_no;
      dbContext.context.user_no = user_no;

      // get database context: database path and database key
      shared_DB = get_db_context(&dbContext, resource_id, ResIsFile, dbKey, dbPath);

      if(shared_DB != -1)  // check valid database context
      {
         rval = remove(dbPath);
         if(rval == -1)
         {
            printf("file_remove ERROR: %s \n", strerror(errno) );
         }
      }
   }
   else
   {
      rval = EPERS_LOCKFS;
   }

   return rval;
}



int pclFileSeek(int fd, long int offset, int whence)
{
   int rval = 0;

   if(AccessNoLock == isAccessLocked() ) // check if access to persistent data is locked
   {
      rval = lseek(fd, offset, whence);
   }
   else
   {
      rval = EPERS_LOCKFS;
   }

   return rval;
}



int pclFileUnmapData(void* address, long size)
{
   int rval = 0;

   if(AccessNoLock != isAccessLocked() ) // check if access to persistent data is locked
   {
      rval =  munmap(address, size);
   }
   else
   {
      rval = EPERS_LOCKFS;
   }

   return rval;
}



int pclFileWriteData(int fd, const void * buffer, int buffer_size)
{
   int size = 0;

   if(AccessNoLock != isAccessLocked() ) // check if access to persistent data is locked
   {
      size = write(fd, buffer, buffer_size);
   }
   else
   {
      size = EPERS_LOCKFS;
   }

   return size;
}


