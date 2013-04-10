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
#include "../include_protected/crc32.h"

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
#include <stdlib.h>


// header prototype definition of internal functions
int pcl_create_backup(const char* srcPath, int srcfd, const char* csumPath, const char* csumBuf);

int pcl_recover_from_backup(int backupFd, const char* original);

int pcl_calc_crc32_checksum(int fd, char crc32sum[]);

int pcl_verify_consistency(const char* origPath, const char* backupPath, const char* csumPath, int flags);
//-------------------------------------------------------------



int pclFileClose(int fd)
{
   int rval = -1;

   if(fd < MaxPersHandle)
   {
      // check if a backup and checksum file needs to bel deleted
      if( gFileHandleArray[fd].permission != PersistencePermission_ReadOnly)
      {
         // remove bakup file
         rval = remove(gFileHandleArray[fd].backupPath );

         // remove checksum file
         rval = remove(gFileHandleArray[fd].csumPath);
      }
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
   int handle = -1, shared_DB = 0;
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

   if(   (shared_DB >= 0)                                               // check valid database context
      && (dbContext.configKey.type == PersistenceResourceType_file) )   // check if type matches
   {
      int flags = dbContext.configKey.permission;
      char backupPath[DbKeyMaxLen];    // backup file
      char csumPath[DbPathMaxLen];     // checksum file

      memset(backupPath, 0, DbKeyMaxLen);
      memset(csumPath, 0, DbPathMaxLen);

      // file will be opend writable, so check about data consistency
      if(dbContext.configKey.permission != PersistencePermission_ReadOnly)
      {
         snprintf(backupPath, DbPathMaxLen, "%s%s", dbPath, "~");
         snprintf(csumPath,   DbPathMaxLen, "%s%s", dbPath, "~.crc");

         if((handle = pcl_verify_consistency(dbPath, backupPath, csumPath, flags)) == -1)
         {
            printf("pclFileOpen: error => file inconsistent, recovery  N O T  possible!\n");
            return -1;
         }
      }

      if(handle <= 0)   // check if open is needed or already done in verifyConsistency
         handle = open(dbPath, flags);

      if(handle != -1)
      {
         if(handle < MaxPersHandle)
         {
            __sync_fetch_and_add(&gOpenFdArray[handle], FileOpen); // set open flag

            if(dbContext.configKey.permission != PersistencePermission_ReadOnly)
            {
               strcpy(gFileHandleArray[handle].backupPath, backupPath);
               strcpy(gFileHandleArray[handle].csumPath,   csumPath);
               gFileHandleArray[handle].backupCreated = 0;
               gFileHandleArray[handle].permission = dbContext.configKey.permission;
            }
         }
         else
         {
            close(handle);
            handle = EPERS_MAXHANDLE;
         }
      }
      else  // file does not exist, create file and folder
      {
         const char* delimiters = "/\n";   // search for blank and end of line
         char* tokenArray[24];
         char* thePath = dbPath;
         char createPath[DbPathMaxLen];
         int numTokens = 0, i = 0, validPath = 1;

         tokenArray[numTokens++] = strtok(thePath, delimiters);
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
            printf("pclFileOpen ==> no valid path to create: %s\n", dbPath);
         }
      }
   }
   else
   {
      handle = shared_DB;
      printf("pclFileOpen ==> no valid database context or resource no file\n");
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

      if(   (shared_DB >= 0)                                               // check valid database context
         && (dbContext.configKey.type == PersistenceResourceType_file) )   // check if type matches
      {
         rval = remove(dbPath);
         if(rval == -1)
         {
            printf("file_remove ERROR: %s \n", strerror(errno) );
         }
      }
      else
      {
         rval = shared_DB;
         printf("pclFileRemove ==> no valid database context or resource not a file\n");
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
      if(fd < MaxPersHandle)
      {
         // check if a backup file has to be created
         if(   gFileHandleArray[fd].permission != PersistencePermission_ReadOnly
            && gFileHandleArray[fd].backupCreated == 0)
         {
            char csumBuf[64];
            memset(csumBuf, 0, 64);

            // calculate checksum
            pcl_calc_crc32_checksum(fd, csumBuf);

            // create checksum and backup file
            pcl_create_backup(gFileHandleArray[fd].backupPath, fd, gFileHandleArray[fd].csumPath, csumBuf);

            gFileHandleArray[fd].backupCreated = 1;
         }

         size = write(fd, buffer, buffer_size);
      }
   }
   else
   {
      size = EPERS_LOCKFS;
   }

   return size;
}



/****************************************************************************************
 * Functions to create backup files
 ****************************************************************************************/

int pcl_verify_consistency(const char* origPath, const char* backupPath, const char* csumPath, int flags)
{
   int handle = 0, readSize = 0;
   int backupAvail = 0, csumAvail = 0;
   int fdCsum = 0, fdBackup = 0;

   char origCsumBuf[64];
   char backCsumBuf[64];
   char csumBuf[64];

   memset(origCsumBuf, 0, 64);
   memset(backCsumBuf, 0, 64);
   memset(csumBuf, 0, 64);

   // check if we have a backup and checksum file
   backupAvail = access(backupPath, F_OK);
   csumAvail   = access(csumPath, F_OK);

   //printf("verifyConsistency ==> backup: %d | csum: %d \n", backupAvail, csumAvail);

   // *************************************************
   // there is a backup file and a checksum
   // *************************************************
   if( (backupAvail == 0) && (csumAvail == 0) )
   {
      printf("verifyConsistency => there is a backup file AND a checksum\n");
      // calculate checksum form backup file
      fdBackup = open(backupPath,  O_RDONLY);
      if(fdBackup != -1)
      {
         pcl_calc_crc32_checksum(fdBackup, backCsumBuf);

         fdCsum = open(csumPath,  O_RDONLY);
         if(fdCsum != -1)
         {
            readSize = read(fdCsum, csumBuf, 64);
            if(readSize > 0)
            {
               if(strcmp(csumBuf, backCsumBuf)  == 0)
               {
                  // checksum matches ==> replace with original file
                  handle = pcl_recover_from_backup(fdBackup, origPath);
               }
               else
               {
                  // checksum does not match, check checksum with original file
                  handle = open(origPath, flags);
                  if(handle != -1)
                  {
                     pcl_calc_crc32_checksum(handle, origCsumBuf);
                     if(strcmp(csumBuf, origCsumBuf)  != 0)
                     {
                        close(handle);
                        handle = -1;  // error: file corrupt
                     }
                     // else case: checksum matches ==> keep original file ==> nothing to do

                  }
                  else
                  {
                     close(handle);
                     handle = -1;     // error: file corrupt
                  }
               }
            }
            else
            {
               printf("verifyConsistency ==> checksum has invalid size\n");
            }
            close(fdCsum);
         }
         else
         {
            close(fdCsum);
            handle = -1;     // error: file corrupt
         }
      }
      else
      {
         handle = -1;
      }
      close(fdBackup);
   }
   // *************************************************
   // there is ONLY a checksum file
   // *************************************************
   else if(csumAvail == 0)
   {
      //printf("verifyConsistency => there is ONLY a checksum file\n");

      fdCsum = open(csumPath,  O_RDONLY);
      if(fdCsum != -1)
      {
         readSize = read(fdCsum, csumBuf, 64);
         if(readSize != 64)
         {
            printf("verifyConsistency ==> read checksum: invalid readSize\n");
         }
         close(fdCsum);

         // calculate the checksum form the original file to see if it matches
         handle = open(origPath, flags);
         if(handle != -1)
         {
            pcl_calc_crc32_checksum(handle, origCsumBuf);

            if(strcmp(csumBuf, origCsumBuf)  != 0)
            {
                close(handle);
                handle = -1;  // checksum does NOT match ==> error: file corrupt
            }
            // else case: checksum matches ==> keep original file ==> nothing to do
         }
         else
         {
            close(handle);
            handle = -1;      // error: file corrupt
         }
      }
      else
      {
         close(fdCsum);
         handle = -1;         // error: file corrupt
      }
   }
   // *************************************************
   // there is ONLY a backup file
   // *************************************************
   else if(backupAvail == 0)
   {
      //printf("verifyConsistency => there is ONLY a backup file\n");
      // calculate checksum form backup file
      fdBackup = open(backupPath,  O_RDONLY);
      if(fdBackup != -1)
      {
         pcl_calc_crc32_checksum(fdBackup, backCsumBuf);
         close(fdBackup);

         // calculate the checksum form the original file to see if it matches
         handle = open(origPath, flags);
         if(handle != -1)
         {
            pcl_calc_crc32_checksum(handle, origCsumBuf);

            if(strcmp(backCsumBuf, origCsumBuf)  != 0)
            {
               close(handle);
               handle = -1;   // checksum does NOT match ==> error: file corrupt
            }
            // else case: checksum matches ==> keep original file ==> nothing to do

         }
         else
         {
            close(handle);
            handle = -1;      // error: file corrupt
         }
      }
      else
      {
         close(fdBackup);
         handle = -1;         // error: file corrupt
      }
   }
   // for else case: nothing to do


   // if we are in an inconsistent state: delete file, backup and checksum
   if(handle == -1)
   {
      remove(origPath);
      remove(backupPath);
      remove(csumPath);
   }

   return handle;
}


int pcl_recover_from_backup(int backupFd, const char* original)
{
   int handle = 0;
   int readSize = 0;
   char buffer[1024];

   handle = open(original, O_TRUNC | O_RDWR);
   if(handle != -1)
   {
      // copy data from one file to another
      while((readSize = read(backupFd, buffer, 1024)) > 0)
      {
         if(write(handle, buffer, readSize) != readSize)
         {
            printf("pcl_recover_from_backup => couldn't write whole buffer\n");
            break;
         }
      }

   }

   return handle;
}

int pcl_create_backup(const char* dstPath, int srcfd, const char* csumPath, const char* csumBuf)
{
   int dstFd = 0, csfd = 0;
   int readSize = -1;
   char buffer[1024];

   // create checksum file and and write checksum
   //printf("   pcl_create_backu => create checksum file: %s \n", csumPath);
   csfd = open(csumPath, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
   if(csfd != -1)
   {
      int csumSize = strlen(csumBuf);
      if(write(csfd, csumBuf, csumSize) != csumSize)
      {
         printf("pcl_create_backup: failed to write checksum to file\n");
      }
      close(csfd);
   }
   else
   {
      printf("pcl_create_backup => failed to create checksum file: %s | %s\n", csumPath, strerror(errno));
   }


   // create backup file, user and group has read/write permission, others have read permission
   //printf("   pclFileOpen => create a backup for file: %s\n", dstPath);
   dstFd = open(dstPath, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
   if(dstFd != -1)
   {
      off_t curPos = 0;
      // remember the current position
      curPos = lseek(srcfd, 0, SEEK_CUR);

      // copy data from one file to another
      while((readSize = read(srcfd, buffer, 1024)) > 0)
      {
         if(write(dstFd, buffer, readSize) != readSize)
         {
            printf("pcl_create_backup => couldn't write whole buffer\n");
            break;
         }
      }

      if(readSize == -1)
         printf("pcl_create_backup => error copying file\n");

      if((readSize = close(dstFd)) == -1)
         printf("pcl_create_backup => error closing fd\n");

      // set back to the position
      lseek(srcfd, curPos, SEEK_SET);
   }
   else
   {
      printf("pcl_create_backup => failed to open backup file: %s | %s \n", dstPath, strerror(errno));
   }

   return readSize;
}



int pcl_calc_crc32_checksum(int fd, char crc32sum[])
{
   int rval = 1;

   if(crc32sum != 0)
   {
      char* buf;
      struct stat statBuf;

      fstat(fd, &statBuf);
      buf = malloc((unsigned int)statBuf.st_size);

      if(buf != 0)
      {
         off_t curPos = 0;
         // remember the current position
         curPos = lseek(fd, 0, SEEK_CUR);

         if(curPos != 0)
         {
            // set to beginning of the file
            lseek(fd, 0, SEEK_SET);
         }

         while((rval = read(fd, buf, statBuf.st_size)) > 0)
         {
            unsigned int crc = 0;
            crc = crc32(crc, (unsigned char*)buf, statBuf.st_size);
            snprintf(crc32sum, ChecksumBufSize-1, "%x", crc);
         }

         // set back to the position
         lseek(fd, curPos, SEEK_SET);

         free(buf);
      }
   }
   return rval;
}




