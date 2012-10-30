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
 * @file           persistence_client_library_file.c
 * @ingroup        Persistence client library
 * @author         Ingo Huerner
 * @brief          Implementation of the persistence client library.
 *                 Library provides an API to access persistent data
 * @see            
 */

#include "persistence_client_library_file.h"
#include "persistence_client_library.h"
#include "persistence_client_library_data_access.h"
#include "persistence_client_library_pas_interface.h"
#include "persistence_client_library_handle.h"
#include "persistence_client_library_access_helper.h"

#include <fcntl.h>   // for open flags
#include <errno.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>



int file_close(int fd)
{
   int rval = -1;

   rval = close(fd);
   if(fd < maxPersHandle)
   {
      __sync_fetch_and_sub(&gOpenFdArray[fd], FileClosed);   // set closed flag
   }

   return rval;
}



int file_get_size(int fd)
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



void* file_map_data(void* addr, long size, long offset, int fd)
{
   void* ptr = 0;

   if(accessNoLock != isAccessLocked() ) // check if access to persistent data is locked
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



int file_open(unsigned char ldbid, char* resource_id, unsigned char user_no, unsigned char seat_no)
{
   int handle = -1;

   int shared_DB = 0,
       flags = O_RDWR;

   char dbKey[dbKeyMaxLen];      // database key
   char dbPath[dbPathMaxLen];    // database location

   memset(dbKey, 0, dbKeyMaxLen);
   memset(dbPath, 0, dbPathMaxLen);

   // get database context: database path and database key
   shared_DB = get_db_context(ldbid, resource_id, user_no, seat_no, resIsFile, dbKey, dbPath);

   if(shared_DB != -1)  // check valid database context
   {
      handle = open(dbPath, flags);

      if(handle == -1)
      {
         printf("file_open ERROR: %s \n", strerror(errno) );
      }
      else
      {
         if(handle < maxPersHandle)
         {
            __sync_fetch_and_add(&gOpenFdArray[handle], FileOpen); // set open flag
         }
         else
         {
            handle = EPERS_MAXHANDLE;
         }
      }
   }

   return handle;
}



int file_read_data(int fd, void * buffer, unsigned long buffer_size)
{
   return read(fd, buffer, buffer_size);
}



int file_remove(unsigned char ldbid, char* resource_id, unsigned char user_no, unsigned char seat_no)
{
   int rval = 0;

   if(accessNoLock != isAccessLocked() ) // check if access to persistent data is locked
   {
      int shared_DB = 0;

      char dbKey[dbKeyMaxLen];      // database key
      char dbPath[dbPathMaxLen];    // database location

      memset(dbKey, 0, dbKeyMaxLen);
      memset(dbPath, 0, dbPathMaxLen);

      // get database context: database path and database key
      shared_DB = get_db_context(ldbid, resource_id, user_no, seat_no, resIsFile, dbKey, dbPath);

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



int file_seek(int fd, long int offset, int whence)
{
   int rval = 0;

   if(accessNoLock == isAccessLocked() ) // check if access to persistent data is locked
   {
      rval = lseek(fd, offset, whence);
   }
   else
   {
      rval = EPERS_LOCKFS;
   }

   return rval;
}



int file_unmap_data(void* address, long size)
{
   int rval = 0;

   if(accessNoLock != isAccessLocked() ) // check if access to persistent data is locked
   {
      rval =  munmap(address, size);
   }
   else
   {
      rval = EPERS_LOCKFS;
   }

   return rval;
}



int file_write_data(int fd, const void * buffer, unsigned long buffer_size)
{
   int size = 0;

   if(accessNoLock != isAccessLocked() ) // check if access to persistent data is locked
   {
      size = write(fd, buffer, buffer_size);
   }
   else
   {
      size = EPERS_LOCKFS;
   }

   return size;
}


