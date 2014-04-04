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
#include "persistence_client_library_pas_interface.h"
#include "persistence_client_library_handle.h"
#include "persistence_client_library_prct_access.h"
#include "persistence_client_library_data_organization.h"
#include "persistence_client_library_db_access.h"
#include "crc32.h"

#if USE_FILECACHE
   #include <persistence_file_cache.h>
#endif

#include <fcntl.h>   // for open flags
#include <errno.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>



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
#if USE_FILECACHE
         rval = pfcCloseFile(fd);
#else
         rval = close(fd);
#endif

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
   int size = EPERS_NOT_INITIALIZED;

   if(gPclInitialized >= PCLinitialized)
   {

#if USE_FILECACHE
      size = pfcFileGetSize(fd);
#else
      struct stat buf;
      size = fstat(fd, &buf);

      //DLT_LOG(gDLTContext, DLT_LOG_INFO, DLT_STRING("pclFileGetSize fd: "), DLT_INT(fd));

      if(size != -1)
      {
         size = buf.st_size;
      }
#endif
   }
   return size;
}



void* pclFileMapData(void* addr, long size, long offset, int fd)
{
   void* ptr = 0;

#if USE_FILECACHE
   DLT_LOG(gPclDLTContext, DLT_LOG_WARN, DLT_STRING("pclFileMapData not supported when using file cache"));
#else
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
#endif

   return ptr;
}



int pclFileOpen(unsigned int ldbid, const char* resource_id, unsigned int user_no, unsigned int seat_no)
{
   int handle = EPERS_NOT_INITIALIZED;

   if(gPclInitialized >= PCLinitialized)
   {
      int shared_DB = 0;
      PersistenceInfo_s dbContext;

      char dbKey[DbKeyMaxLen]      = {0};    // database key
      char dbPath[DbPathMaxLen]    = {0};    // database location
      char backupPath[DbPathMaxLen] = {0};    // backup file
      char csumPath[DbPathMaxLen]  = {0};    // checksum file

      //DLT_LOG(gDLTContext, DLT_LOG_INFO, DLT_STRING("pclFileOpen: "), DLT_INT(ldbid), DLT_STRING(resource_id) );

      dbContext.context.ldbid   = ldbid;
      dbContext.context.seat_no = seat_no;
      dbContext.context.user_no = user_no;

      // get database context: database path and database key
      shared_DB = get_db_context(&dbContext, resource_id, ResIsFile, dbKey, dbPath);

      if(dbContext.configKey.type == PersistenceResourceType_file)   // check if the resource is really a file
      {
         // create backup path
         int length = 0;
         char fileSubPath[DbPathMaxLen] = {0};

         if(dbContext.configKey.policy ==  PersistencePolicy_wc)
         {
            length = gCPathPrefixSize;
         }
         else
         {
            length = gWTPathPrefixSize;
         }

         strncpy(fileSubPath, dbPath+length, DbPathMaxLen);
         snprintf(backupPath, DbPathMaxLen-1, "%s%s", gBackupPrefix, fileSubPath);
         snprintf(csumPath,   DbPathMaxLen-1, "%s%s%s", gBackupPrefix, fileSubPath, ".crc");

         if(shared_DB >= 0)                                          // check valid database context
         {
            int flags = pclGetPosixPermission(dbContext.configKey.permission);

            // file will be opened writable, so check about data consistency
            if( (dbContext.configKey.permission != PersistencePermission_ReadOnly)
               && pclBackupNeeded(dbPath) )
            {
               if((handle = pclVerifyConsistency(dbPath, backupPath, csumPath, flags)) == -1)
               {
                  DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("pclFileOpen: error => file inconsistent, recovery  N O T  possible!"));
                  return -1;
               }
            }
            else
            {
               DLT_LOG(gPclDLTContext, DLT_LOG_INFO, DLT_STRING("pclFileOpen: No Backup => file is read only OR is in blacklist!"));
            }

#if USE_FILECACHE

            if(handle > 0)   // when the file is open, close it and do a new open unde PFC control
            {
               close(handle);
            }

            handle = pfcOpenFile(dbPath);
#else
            if(handle <= 0)   // check if open is needed or already done in verifyConsistency
            {
               handle = open(dbPath, flags);
            }
#endif

            if(handle == -1 && errno == ENOENT) // file does not exist, create file and folder
            {
               if( (handle = pclCreateFile(dbPath)) == -1)
               {
                  DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("pclFileOpen: error => failed to create file: "), DLT_STRING(dbPath));
               }
            }

            if(handle < MaxPersHandle && handle > 0 )
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
         else  // requested resource is not in the RCT, so create resource as local/cached.
         {
            // assemble file string for local cached location
            snprintf(dbPath, DbPathMaxLen, gLocalCacheFilePath, gAppId, user_no, seat_no, resource_id);
            handle = pclCreateFile(dbPath);

            if(handle < MaxPersHandle && handle > 0)
            {
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
      else
      {
         handle = EPERS_RESOURCE_NO_FILE;
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
#if USE_FILECACHE
      readSize = pfcReadFile(fd, buffer, buffer_size);
#else
      readSize = read(fd, buffer, buffer_size);
#endif
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
               DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("pclFileRemove => remove ERROR"), DLT_STRING(strerror(errno)) );
            }
         }
         else
         {
            rval = shared_DB;
            DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("pclFileRemove ==> no valid database context or resource not a file"));
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
#if USE_FILECACHE
         rval = pfcFileSeek(fd, offset, whence);
#else
         rval = lseek(fd, offset, whence);
#endif
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
            if(gFileHandleArray[fd].permission != PersistencePermission_ReadOnly)
            {
               // check if a backup file has to be created
               if(gFileHandleArray[fd].backupCreated == 0)
               {
                  char csumBuf[ChecksumBufSize] = {0};

                  // calculate checksum
                  pclCalcCrc32Csum(fd, csumBuf);
                  // create checksum and backup file
                  pclCreateBackup(gFileHandleArray[fd].backupPath, fd, gFileHandleArray[fd].csumPath, csumBuf);

                  gFileHandleArray[fd].backupCreated = 1;
               }

#if USE_FILECACHE
               size = pfcWriteFile(fd, buffer, buffer_size);
#else
               size = write(fd, buffer, buffer_size);
#endif
            }
            else
            {
               size = EPERS_RESOURCE_READ_ONLY;
            }
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
      char backupPath[DbPathMaxLen] = {0};    // backup file
      char csumPath[DbPathMaxLen]  = {0};    // checksum file

      //DLT_LOG(gDLTContext, DLT_LOG_INFO, DLT_STRING("pclFileOpen: "), DLT_INT(ldbid), DLT_STRING(resource_id) );

      dbContext.context.ldbid   = ldbid;
      dbContext.context.seat_no = seat_no;
      dbContext.context.user_no = user_no;

      // get database context: database path and database key
      shared_DB = get_db_context(&dbContext, resource_id, ResIsFile, dbKey, dbPath);

      if( dbContext.configKey.type == PersistenceResourceType_file)     // check if type matches
      {
         if(shared_DB >= 0)                                             // check valid database context
         {
            int flags = pclGetPosixPermission(dbContext.configKey.permission);

            // file will be opened writable, so check about data consistency
            if(   dbContext.configKey.permission != PersistencePermission_ReadOnly
               && pclBackupNeeded(dbPath) )
            {
               snprintf(backupPath, DbPathMaxLen-1, "%s%s", dbPath, "~");
               snprintf(csumPath,   DbPathMaxLen-1, "%s%s", dbPath, "~.crc");

               if((handle = pclVerifyConsistency(dbPath, backupPath, csumPath, flags)) == -1)
               {
                  DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("pclFileOpen: error => file inconsistent, recovery  N O T  possible!"));
                  return -1;
               }
               // we don't need the file handle here
               // the application calling this function must use the POSIX open() function to get an file descriptor
               close(handle);
            }

            handle = get_persistence_handle_idx();

            if(handle != -1)
            {
               if(handle < MaxPersHandle)
               {
                  __sync_fetch_and_add(&gOpenHandleArray[handle], FileOpen); // set open flag

                  if(dbContext.configKey.permission != PersistencePermission_ReadOnly)
                  {
                     strncpy(gOssHandleArray[handle].backupPath, backupPath, DbPathMaxLen);
                     strncpy(gOssHandleArray[handle].csumPath,   csumPath, DbPathMaxLen);

                     gOssHandleArray[handle].backupCreated = 0;
                     gOssHandleArray[handle].permission = dbContext.configKey.permission;
                  }

                  *size = strlen(dbPath);
                  *path = malloc((*size)+1);       // allocate 1 byte for the string termination
                  memcpy(*path, dbPath, (*size));
                  (*path)[(*size)] = '\0';         // terminate string
                  gOssHandleArray[handle].filePath = *path;

                  if(access(*path, F_OK) == -1)
                  {
                     // file does not exist, create it.
                     int handle = pclCreateFile(*path);
                     close(handle);    // don't need the open file
                  }
               }
               else
               {
                  set_persistence_handle_close_idx(handle);
                  handle = EPERS_MAXHANDLE;
               }
            }
         }
         else  // requested resource is not in the RCT, so create resource as local/cached.
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
                  strncpy(gOssHandleArray[handle].backupPath, backupPath, DbPathMaxLen);
                  strncpy(gOssHandleArray[handle].csumPath,   csumPath, DbPathMaxLen);
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
      else
      {
         handle = EPERS_RESOURCE_NO_FILE;
      }
   }

   return handle;
}



int pclFileReleasePath(int pathHandle)
{
   int rval = EPERS_NOT_INITIALIZED;

   //DLT_LOG(gDLTContext, DLT_LOG_INFO, DLT_STRING("pclFileClose fd: "), DLT_INT(fd));

   if(gPclInitialized >= PCLinitialized)
   {
      if(pathHandle < MaxPersHandle)
      {
         // check if a backup and checksum file needs to bel deleted
         if( gFileHandleArray[pathHandle].permission != PersistencePermission_ReadOnly)
         {
            // remove backup file
            remove(gOssHandleArray[pathHandle].backupPath);  // we don't care about return value

            // remove checksum file
            remove(gOssHandleArray[pathHandle].csumPath);    // we don't care about return value

         }
         free(gOssHandleArray[pathHandle].filePath);
         __sync_fetch_and_sub(&gOpenHandleArray[pathHandle], FileClosed);   // set closed flag
         set_persistence_handle_close_idx(pathHandle);
         gOssHandleArray[pathHandle].filePath = NULL;
         rval = 1;
      }
      else
      {
        rval = EPERS_MAXHANDLE;
      }
   }

   return rval;
}





