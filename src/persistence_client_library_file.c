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
#include "persistence_client_library_backup_filelist.h"
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
int pclCreateFile(const char* path);

int pclCreateBackup(const char* srcPath, int srcfd, const char* csumPath, const char* csumBuf);

int pclRecoverFromBackup(int backupFd, const char* original);

int pclCalcCrc32Csum(int fd, char crc32sum[]);

int pclVerifyConsistency(const char* origPath, const char* backupPath, const char* csumPath, int openFlags);

int pclBackupNeeded(const char* path);
//-------------------------------------------------------------



int pclFileClose(int fd)
{
   int rval = EPERS_NOT_INITIALIZED;

   //DLT_LOG(gDLTContext, DLT_LOG_INFO, DLT_STRING("pclFileClose fd: "), DLT_INT(fd));

   if(gPclInitialized >= PCLinitialized)
   {
      if(fd < MaxPersHandle)
      {
         // check if a backup and checksum file needs to bel deleted
         if( gFileHandleArray[fd].permission != PersistencePermission_ReadOnly)
         {
            // remove backup file
            remove(gFileHandleArray[fd].backupPath);  // we don't care about return value

            // remove checksum file
            remove(gFileHandleArray[fd].csumPath);    // we don't care about return value

         }
         __sync_fetch_and_sub(&gOpenFdArray[fd], FileClosed);   // set closed flag
         rval = close(fd);
      }
      else
      {
    	  rval = EPERS_MAXHANDLE;
      }
   }

   return rval;
}



int pclFileGetSize(int fd)
{
   int rval = EPERS_NOT_INITIALIZED;

   if(gPclInitialized >= PCLinitialized)
   {
      struct stat buf;
      int ret = 0;
      ret = fstat(fd, &buf);

      //DLT_LOG(gDLTContext, DLT_LOG_INFO, DLT_STRING("pclFileGetSize fd: "), DLT_INT(fd));

      if(ret != -1)
      {
         rval = buf.st_size;
      }
   }

   return rval;
}



void* pclFileMapData(void* addr, long size, long offset, int fd)
{
   void* ptr = 0;

   //DLT_LOG(gDLTContext, DLT_LOG_INFO, DLT_STRING("pclFileMapData fd: "), DLT_INT(fd));

   if(gPclInitialized >= PCLinitialized)
   {
      if(AccessNoLock != isAccessLocked() ) // check if access to persistent data is locked
      {
         int mapFlag = PROT_WRITE | PROT_READ;
         ptr = mmap(addr,size, mapFlag, MAP_SHARED, fd, offset);
      }
      else
      {
         ptr = EPERS_MAP_LOCKFS;
      }
   }

   return ptr;
}



int pclFileOpen(unsigned int ldbid, const char* resource_id, unsigned int user_no, unsigned int seat_no)
{
   int handle = EPERS_NOT_INITIALIZED;

   if(gPclInitialized >= PCLinitialized)
   {
      int shared_DB = 0;
      PersistenceInfo_s dbContext;

      char dbKey[DbKeyMaxLen]      = {0};         // database key
      char dbPath[DbPathMaxLen]    = {0};       // database location
      char backupPath[DbKeyMaxLen] = {0};    // backup file
      char csumPath[DbPathMaxLen]  = {0};     // checksum file

      //DLT_LOG(gDLTContext, DLT_LOG_INFO, DLT_STRING("pclFileOpen: "), DLT_INT(ldbid), DLT_STRING(resource_id) );

      dbContext.context.ldbid   = ldbid;
      dbContext.context.seat_no = seat_no;
      dbContext.context.user_no = user_no;

      // get database context: database path and database key
      shared_DB = get_db_context(&dbContext, resource_id, ResIsFile, dbKey, dbPath);

      if(   (shared_DB >= 0)                                               // check valid database context
         && (dbContext.configKey.type == PersistenceResourceType_file) )   // check if type matches
      {
         int flags = dbContext.configKey.permission;

         // file will be opened writable, so check about data consistency
         if(   dbContext.configKey.permission != PersistencePermission_ReadOnly
            && pclBackupNeeded(dbPath) )
         {
            snprintf(backupPath, DbPathMaxLen, "%s%s", dbPath, "~");
            snprintf(csumPath,   DbPathMaxLen, "%s%s", dbPath, "~.crc");

            if(pclVerifyConsistency(dbPath, backupPath, csumPath, flags) == -1)
            {
               DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("pclFileOpen: error => file inconsistent, recovery  N O T  possible!"));
               return -1;
            }
         }

         // open file
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
            handle = pclCreateFile(dbPath);
         }
      }
      else
      {
         // assemble file string for local cached location
         snprintf(dbPath, DbPathMaxLen, gLocalCacheFilePath, gAppId, user_no, seat_no, resource_id);
         handle = pclCreateFile(dbPath);

         if(handle != -1)
         {
            if(handle < MaxPersHandle)
            {
               snprintf(backupPath, DbPathMaxLen, "%s%s", dbPath, "~");
               snprintf(csumPath,   DbPathMaxLen, "%s%s", dbPath, "~.crc");

               __sync_fetch_and_add(&gOpenFdArray[handle], FileOpen); // set open flag
               strcpy(gFileHandleArray[handle].backupPath, backupPath);
               strcpy(gFileHandleArray[handle].csumPath,   csumPath);
               gFileHandleArray[handle].backupCreated = 0;
               gFileHandleArray[handle].permission = PersistencePermission_ReadWrite;  // make it writable
            }
            else
            {
               close(handle);
               handle = EPERS_MAXHANDLE;
            }
         }
      }
   }

   return handle;
}



int pclFileReadData(int fd, void * buffer, int buffer_size)
{
   int readSize = EPERS_NOT_INITIALIZED;

   //DLT_LOG(gDLTContext, DLT_LOG_INFO, DLT_STRING("pclFileReadData fd: "), DLT_INT(fd));
   if(gPclInitialized >= PCLinitialized)
   {
      readSize = read(fd, buffer, buffer_size);
   }
   return readSize;
}



int pclFileRemove(unsigned int ldbid, const char* resource_id, unsigned int user_no, unsigned int seat_no)
{
   int rval = EPERS_NOT_INITIALIZED;

   //DLT_LOG(gDLTContext, DLT_LOG_INFO, DLT_STRING("pclFileReadData "), DLT_INT(ldbid), DLT_STRING(resource_id));

   if(gPclInitialized >= PCLinitialized)
   {
      if(AccessNoLock != isAccessLocked() ) // check if access to persistent data is locked
      {
         int shared_DB = 0;
         PersistenceInfo_s dbContext;

         char dbKey[DbKeyMaxLen]   = {0};      // database key
         char dbPath[DbPathMaxLen] = {0};    // database location

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
               DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("pclFileRemove => remove ERROR"), DLT_STRING(strerror(errno)) );
            }
         }
         else
         {
            rval = shared_DB;
            DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("pclFileRemove ==> no valid database context or resource not a file"));
         }
      }
      else
      {
         rval = EPERS_LOCKFS;
      }
   }

   return rval;
}



int pclFileSeek(int fd, long int offset, int whence)
{
   int rval = EPERS_NOT_INITIALIZED;

   //DLT_LOG(gDLTContext, DLT_LOG_INFO, DLT_STRING("pclFileSeek fd:"), DLT_INT(fd));

   if(gPclInitialized >= PCLinitialized)
   {
      if(AccessNoLock != isAccessLocked() ) // check if access to persistent data is locked
      {
         rval = lseek(fd, offset, whence);
      }
      else
      {
         rval = EPERS_LOCKFS;
      }
   }

   return rval;
}



int pclFileUnmapData(void* address, long size)
{
   int rval = EPERS_NOT_INITIALIZED;

   //DLT_LOG(gDLTContext, DLT_LOG_INFO, DLT_STRING("pclFileUnmapData"));

   if(gPclInitialized >= PCLinitialized)
   {
      if(AccessNoLock != isAccessLocked() ) // check if access to persistent data is locked
      {
         rval =  munmap(address, size);
      }
      else
      {
         rval = EPERS_LOCKFS;
      }
   }

   return rval;
}



int pclFileWriteData(int fd, const void * buffer, int buffer_size)
{
   int size = EPERS_NOT_INITIALIZED;

   //DLT_LOG(gDLTContext, DLT_LOG_INFO, DLT_STRING("pclFileWriteData fd:"), DLT_INT(fd));

   if(gPclInitialized >= PCLinitialized)
   {
      if(AccessNoLock != isAccessLocked() ) // check if access to persistent data is locked
      {
         if(fd < MaxPersHandle)
         {
            // check if a backup file has to be created
            if(   gFileHandleArray[fd].permission != PersistencePermission_ReadOnly
               && gFileHandleArray[fd].backupCreated == 0)
            {
               char csumBuf[ChecksumBufSize] = {0};

               // calculate checksum
               pclCalcCrc32Csum(fd, csumBuf);

               // create checksum and backup file
               pclCreateBackup(gFileHandleArray[fd].backupPath, fd, gFileHandleArray[fd].csumPath, csumBuf);

               gFileHandleArray[fd].backupCreated = 1;
            }

            size = write(fd, buffer, buffer_size);
         }
      }
      else
      {
         size = EPERS_LOCKFS;
      }
   }

   return size;
}


int pclFileCreatePath(unsigned int ldbid, const char* resource_id, unsigned int user_no, unsigned int seat_no, char** path, unsigned int* size)
{
   int handle = EPERS_NOT_INITIALIZED;

   if(gPclInitialized >= PCLinitialized)
   {
      int shared_DB = 0;
      PersistenceInfo_s dbContext;

      char dbKey[DbKeyMaxLen]      = {0};    // database key
      char dbPath[DbPathMaxLen]    = {0};    // database location
      char backupPath[DbKeyMaxLen] = {0};    // backup file
      char csumPath[DbPathMaxLen]  = {0};    // checksum file

      //DLT_LOG(gDLTContext, DLT_LOG_INFO, DLT_STRING("pclFileOpen: "), DLT_INT(ldbid), DLT_STRING(resource_id) );

      dbContext.context.ldbid   = ldbid;
      dbContext.context.seat_no = seat_no;
      dbContext.context.user_no = user_no;

      // get database context: database path and database key
      shared_DB = get_db_context(&dbContext, resource_id, ResIsFile, dbKey, dbPath);

      if(   (shared_DB >= 0)                                               // check valid database context
         && (dbContext.configKey.type == PersistenceResourceType_file) )   // check if type matches
      {
         int flags = dbContext.configKey.permission;

         // file will be opened writable, so check about data consistency
         if(   dbContext.configKey.permission != PersistencePermission_ReadOnly
            && pclBackupNeeded(dbPath) )
         {
            snprintf(backupPath, DbPathMaxLen, "%s%s", dbPath, "~");
            snprintf(csumPath,   DbPathMaxLen, "%s%s", dbPath, "~.crc");

            if((pclVerifyConsistency(dbPath, backupPath, csumPath, flags)) == -1)
            {
               DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("pclFileOpen: error => file inconsistent, recovery  N O T  possible!"));
               return -1;
            }
         }

         handle = get_persistence_handle_idx();

         if(handle != -1)
         {
            if(handle < MaxPersHandle)
            {
               __sync_fetch_and_add(&gOpenHandleArray[handle], FileOpen); // set open flag

               if(dbContext.configKey.permission != PersistencePermission_ReadOnly)
               {
                  strcpy(gOssHandleArray[handle].backupPath, backupPath);
                  strcpy(gOssHandleArray[handle].csumPath,   csumPath);

                  gOssHandleArray[handle].backupCreated = 0;
                  gOssHandleArray[handle].permission = dbContext.configKey.permission;
               }

               *size = strlen(dbPath);
               *path = malloc(*size);
               memcpy(*path, dbPath, *size);
            }
            else
            {
               set_persistence_handle_close_idx(handle);
               handle = EPERS_MAXHANDLE;
            }
         }
      }
      else
      {
         // assemble file string for local cached location
         snprintf(dbPath, DbPathMaxLen, gLocalCacheFilePath, gAppId, user_no, seat_no, resource_id);

         handle = get_persistence_handle_idx();

         if(handle != -1)
         {
            if(handle < MaxPersHandle)
            {
               snprintf(backupPath, DbPathMaxLen, "%s%s", dbPath, "~");
               snprintf(csumPath,   DbPathMaxLen, "%s%s", dbPath, "~.crc");

               __sync_fetch_and_add(&gOpenHandleArray[handle], FileOpen); // set open flag
               strcpy(gOssHandleArray[handle].backupPath, backupPath);
               strcpy(gOssHandleArray[handle].csumPath,   csumPath);
               gOssHandleArray[handle].backupCreated = 0;
               gOssHandleArray[handle].permission = PersistencePermission_ReadWrite;  // make it writable
            }
            else
            {
               set_persistence_handle_close_idx(handle);
               handle = EPERS_MAXHANDLE;
            }
         }
      }
   }

   return handle;
}



int pclFileReleasePath(int pathPandle, char* path)
{
   int rval = EPERS_NOT_INITIALIZED;

   //DLT_LOG(gDLTContext, DLT_LOG_INFO, DLT_STRING("pclFileClose fd: "), DLT_INT(fd));

   if(gPclInitialized >= PCLinitialized)
   {
      if(pathPandle < MaxPersHandle)
      {
         // check if a backup and checksum file needs to bel deleted
         if( gFileHandleArray[pathPandle].permission != PersistencePermission_ReadOnly)
         {
            // remove backup file
            remove(gOssHandleArray[pathPandle].backupPath);  // we don't care about return value

            // remove checksum file
            remove(gOssHandleArray[pathPandle].csumPath);    // we don't care about return value

         }
         __sync_fetch_and_sub(&gOpenHandleArray[pathPandle], FileClosed);   // set closed flag
         set_persistence_handle_close_idx(pathPandle);
         free(path);
         path = NULL;
         rval = 1;
      }
      else
      {
        rval = EPERS_MAXHANDLE;
      }
   }

   return rval;
}





/****************************************************************************************
 * Functions to create backup files
 ****************************************************************************************/

int pclCreateFile(const char* path)
{
   const char* delimiters = "/\n";   // search for blank and end of line
   char* tokenArray[24];
   char* thePath = (char*)path;
   int numTokens = 0, i = 0, validPath = 1;
   int handle = 0;

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
      char createPath[DbPathMaxLen] = {0};
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
      DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("pclCreateFile ==> no valid path to create: "), DLT_STRING(path) );
   }

   return handle;
}


int pclVerifyConsistency(const char* origPath, const char* backupPath, const char* csumPath, int openFlags)
{
   int handle = 0, readSize = 0;
   int backupAvail = 0, csumAvail = 0;
   int fdCsum = 0, fdBackup = 0;

   char origCsumBuf[ChecksumBufSize] = {0};
   char backCsumBuf[ChecksumBufSize] = {0};
   char csumBuf[ChecksumBufSize]     = {0};

   // check if we have a backup and checksum file
   backupAvail = access(backupPath, F_OK);
   csumAvail   = access(csumPath, F_OK);

   // *************************************************
   // there is a backup file and a checksum
   // *************************************************
   if( (backupAvail == 0) && (csumAvail == 0) )
   {
      DLT_LOG(gDLTContext, DLT_LOG_INFO, DLT_STRING("pclVerifyConsistency => there is a backup file AND a checksum"));
      // calculate checksum form backup file
      fdBackup = open(backupPath,  O_RDONLY);
      if(fdBackup != -1)
      {
         pclCalcCrc32Csum(fdBackup, backCsumBuf);

         fdCsum = open(csumPath,  O_RDONLY);
         if(fdCsum != -1)
         {
            readSize = read(fdCsum, csumBuf, ChecksumBufSize);
            if(readSize > 0)
            {
               if(strcmp(csumBuf, backCsumBuf)  == 0)
               {
                  // checksum matches ==> replace with original file
                  handle = pclRecoverFromBackup(fdBackup, origPath);
               }
               else
               {
                  // checksum does not match, check checksum with original file
                  handle = open(origPath, openFlags);
                  if(handle != -1)
                  {
                     pclCalcCrc32Csum(handle, origCsumBuf);
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
      DLT_LOG(gDLTContext, DLT_LOG_INFO, DLT_STRING("pclVerifyConsistency => there is ONLY a checksum file"));

      fdCsum = open(csumPath,  O_RDONLY);
      if(fdCsum != -1)
      {
         readSize = read(fdCsum, csumBuf, ChecksumBufSize);
         if(readSize <= 0)
         {
            DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("pclVerifyConsistency => read checksum: invalid readSize"));
         }
         close(fdCsum);

         // calculate the checksum form the original file to see if it matches
         handle = open(origPath, openFlags);
         if(handle != -1)
         {
            pclCalcCrc32Csum(handle, origCsumBuf);

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
      DLT_LOG(gDLTContext, DLT_LOG_INFO, DLT_STRING("pclVerifyConsistency => there is ONLY a backup file"));

      // calculate checksum form backup file
      fdBackup = open(backupPath,  O_RDONLY);
      if(fdBackup != -1)
      {
         pclCalcCrc32Csum(fdBackup, backCsumBuf);
         close(fdBackup);

         // calculate the checksum form the original file to see if it matches
         handle = open(origPath, openFlags);
         if(handle != -1)
         {
            pclCalcCrc32Csum(handle, origCsumBuf);

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
   else
   {
      close(handle);
   }

   return handle;
}


int pclRecoverFromBackup(int backupFd, const char* original)
{
   int handle = 0;
   int readSize = 0;
   char buffer[RDRWBufferSize];

   handle = open(original, O_TRUNC | O_RDWR);
   if(handle != -1)
   {
      // copy data from one file to another
      while((readSize = read(backupFd, buffer, RDRWBufferSize)) > 0)
      {
         if(write(handle, buffer, readSize) != readSize)
         {
            DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("pclRecoverFromBackup => couldn't write whole buffer"));
            break;
         }
      }

   }

   return handle;
}

int pclCreateBackup(const char* dstPath, int srcfd, const char* csumPath, const char* csumBuf)
{
   int dstFd = 0, csfd = 0;
   int readSize = -1;
   char buffer[RDRWBufferSize];

   // create checksum file and and write checksum
   csfd = open(csumPath, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
   if(csfd != -1)
   {
      int csumSize = strlen(csumBuf);
      if(write(csfd, csumBuf, csumSize) != csumSize)
      {
         DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("pclCreateBackup => failed to write checksum to file"));
      }
      close(csfd);
   }
   else
   {
      DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("pclCreateBackup => failed to create checksum file:"), DLT_STRING(strerror(errno)) );
   }


   // create backup file, user and group has read/write permission, others have read permission
   dstFd = open(dstPath, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
   if(dstFd != -1)
   {
      off_t curPos = 0;
      // remember the current position
      curPos = lseek(srcfd, 0, SEEK_CUR);

      // copy data from one file to another
      while((readSize = read(srcfd, buffer, RDRWBufferSize)) > 0)
      {
         if(write(dstFd, buffer, readSize) != readSize)
         {
            DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("pclCreateBackup => couldn't write whole buffer"));
            break;
         }
      }

      if(readSize == -1)
         DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("pcl_create_backup => error copying file"));

      if((readSize = close(dstFd)) == -1)
         DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("pcl_create_backup => error closing fd"));

      // set back to the position
      lseek(srcfd, curPos, SEEK_SET);
   }
   else
   {
      DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("pclCreateBackup => failed to open backup file"),
                                          DLT_STRING(dstPath), DLT_STRING(strerror(errno)));
   }

   return readSize;
}



int pclCalcCrc32Csum(int fd, char crc32sum[])
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



int pclBackupNeeded(const char* path)
{
   return need_backup_path(path);
}




