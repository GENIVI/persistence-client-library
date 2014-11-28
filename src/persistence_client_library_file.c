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
#include <unistd.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/sendfile.h>

// local function prototype
int pclFileGetDefaultData(int handle, const char* resource_id, int policy);

extern int doAppcheck(void);

char* get_raw_string(char* dbKey)
{
	char* keyPtr = NULL;
	int cnt = 0, i = 0;

	for(i=0; i<DbKeyMaxLen; i++)
	{
		if(dbKey[i] == '/')
		{
			cnt++;
		}

		if(cnt >= 5)	// stop after the 5th '/' has been found
		{
			break;
		}
		keyPtr = dbKey+i;
	}
	return ++keyPtr;
}


int pclFileClose(int fd)
{
   int rval = EPERS_NOT_INITIALIZED;

   //DLT_LOG(gDLTContext, DLT_LOG_INFO, DLT_STRING("pclFileClose fd: "), DLT_INT(fd));

   if(__sync_add_and_fetch(&gPclInitCounter, 0) > 0)
   {
      if(doAppcheck() == 1)
      {
         int  permission = get_file_permission(fd);

         if(permission != -1)	// permission is here also used for range check
         {
            // check if a backup and checksum file needs to be deleted
            if(permission != PersistencePermission_ReadOnly || permission != PersistencePermission_LastEntry)
            {
               // remove backup file
               remove(get_file_backup_path(fd));  // we don't care about return value

               // remove checksum file
               remove(get_file_checksum_path(fd));    // we don't care about return value

            }
            __sync_fetch_and_sub(&gOpenFdArray[fd], FileClosed);   // set closed flag
   #if USE_FILECACHE
            if(get_file_cache_status(fd) == 1)
            {
               rval = pfcCloseFile(fd);
            }
            else
            {
               fsync(fd);
               rval = close(fd);
            }
   #else
            fsync(fd);
            rval = close(fd);
   #endif

         }
         else
         {
           rval = EPERS_MAXHANDLE;
         }
      }
      else
      {
         rval = EPERS_SHUTDOWN_NO_TRUSTED;
      }
   }
   return rval;
}



int pclFileGetSize(int fd)
{
   int size = EPERS_NOT_INITIALIZED;

   if(__sync_add_and_fetch(&gPclInitCounter, 0) > 0)
   {
      struct stat buf;

#if USE_FILECACHE
      if(get_file_cache_status(fd) == 1)
      {
      	size = pfcFileGetSize(fd);
      }
      else
      {
			size = fstat(fd, &buf);

			if(size != -1)
			{
				size = buf.st_size;
			}
      }
#else

      size = fstat(fd, &buf);

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

   if(__sync_add_and_fetch(&gPclInitCounter, 0) > 0)
   {
      if(AccessNoLock != isAccessLocked() ) // check if access to persistent data is locked
      {
         ptr = mmap(addr,size, PROT_WRITE | PROT_READ, MAP_SHARED, fd, offset);
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

   if(__sync_add_and_fetch(&gPclInitCounter, 0) > 0)
   {
      int shared_DB = 0;
      int wantBackup = 1;
      int cacheStatus = -1;
      PersistenceInfo_s dbContext;

      char dbKey[DbKeyMaxLen]       = {0};    // database key
      char dbPath[DbPathMaxLen]     = {0};    // database location
      char backupPath[DbPathMaxLen] = {0};    // backup file
      char csumPath[DbPathMaxLen]   = {0};    // checksum file

      //DLT_LOG(gDLTContext, DLT_LOG_INFO, DLT_STRING("pclFileOpen: "), DLT_INT(ldbid), DLT_STRING(resource_id) );

      dbContext.context.ldbid   = ldbid;
      dbContext.context.seat_no = seat_no;
      dbContext.context.user_no = user_no;

      // get database context: database path and database key
      shared_DB = get_db_context(&dbContext, resource_id, ResIsFile, dbKey, dbPath);

      //
      // check if the resource is marked as a file resource
      //
      if(dbContext.configKey.type == PersistenceResourceType_file)
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
         snprintf(backupPath, DbPathMaxLen-1, "%s%s%s", gBackupPrefix, fileSubPath, gBackupPostfix);
         snprintf(csumPath,   DbPathMaxLen-1, "%s%s%s", gBackupPrefix, fileSubPath, gBackupCsPostfix);

         //
         // check valid database context
         //
         if(shared_DB >= 0)
         {
            int flags = pclGetPosixPermission(dbContext.configKey.permission);

            // file will be opened writable, so check about data consistency
            if( (dbContext.configKey.permission != PersistencePermission_ReadOnly)
               && (pclBackupNeeded(get_raw_string(dbKey)) == CREATE_BACKUP))
            {
            	wantBackup = 0;
               if((handle = pclVerifyConsistency(dbPath, backupPath, csumPath, flags)) == -1)
               {
                  DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("pclFileOpen - file inconsistent, recovery  N O T  possible!"));
                  return -1;
               }
            }
            else
            {
            	if(dbContext.configKey.permission == PersistencePermission_ReadOnly)
            		DLT_LOG(gPclDLTContext, DLT_LOG_INFO, DLT_STRING("pclFileOpen: No Backup - file is READ ONLY!"), DLT_STRING(dbKey));
            	else
            		DLT_LOG(gPclDLTContext, DLT_LOG_INFO, DLT_STRING("pclFileOpen: No Backup - file is in backup blacklist!"), DLT_STRING(dbKey));

            }

#if USE_FILECACHE
            if(handle > 0)   // when the file is open, close it and do a new open unde PFC control
            {
               close(handle);
            }

            if(strstr(dbPath, WTPREFIX) != NULL)
				{
					// if it's a write through resource, add the O_SYNC and O_DIRECT flag to prevent caching
					handle = open(dbPath, flags);
					cacheStatus = 0;
				}
            else
            {
            	handle = pfcOpenFile(dbPath, DontCreateFile);
            	cacheStatus = 1;
            }

#else
            if(handle <= 0)   // check if open is needed or already done in verifyConsistency
            {
               handle = open(dbPath, flags);

               if(strstr(dbPath, WTPREFIX) != NULL)
               {
               	cacheStatus = 0;
               }
               else
               {
               	cacheStatus = 1;
               }
            }
#endif
            //
            // file does not exist, create it and get default data
            //
            if(handle == -1 && errno == ENOENT)
            {
               if((handle = pclCreateFile(dbPath, cacheStatus)) == -1)
               {
                  DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("pclFileOpen - failed to create file: "), DLT_STRING(dbPath));
               }
               else
               {
               	if(pclFileGetDefaultData(handle, resource_id, dbContext.configKey.policy) == -1)	// try to get default data
               	{
               		DLT_LOG(gPclDLTContext, DLT_LOG_WARN, DLT_STRING("pclFileOpen - no default data available: "), DLT_STRING(resource_id));
               	}
               }

               set_file_cache_status(handle, cacheStatus);
            }

				if(dbContext.configKey.permission != PersistencePermission_ReadOnly)
				{
					if(set_file_handle_data(handle, dbContext.configKey.permission, backupPath, csumPath, NULL) != -1)
					{
						set_file_backup_status(handle, wantBackup);
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
				   if(set_file_handle_data(handle, dbContext.configKey.permission, backupPath, csumPath, NULL) == -1)
				   {
                  close(handle);
                  handle = EPERS_MAXHANDLE;
				   }
				}
         }
         //
         // requested resource is not in the RCT, so create resource as local/cached.
         //
         else
         {
            // assemble file string for local cached location
            snprintf(dbPath, DbPathMaxLen, gLocalCacheFilePath, gAppId, user_no, seat_no, resource_id);
            handle = pclCreateFile(dbPath, 1);
            set_file_cache_status(handle, 1);

            if(handle != -1)
            {
            	if(set_file_handle_data(handle, PersistencePermission_ReadWrite, backupPath, csumPath, NULL) != -1)
					{
            		set_file_backup_status(handle, 1);
						__sync_fetch_and_add(&gOpenFdArray[handle], FileOpen); // set open flag
					}
					else
					{
#if USE_FILECACHE
						pfcCloseFile(handle);
#else
						close(handle);
#endif
						handle = EPERS_MAXHANDLE;
					}
            }
         }
      }
      else
      {
         handle = EPERS_RESOURCE_NO_FILE;	// resource is not marked as file in RCT
      }
   } // initialized

   return handle;
}



int pclFileReadData(int fd, void * buffer, int buffer_size)
{
   int readSize = EPERS_NOT_INITIALIZED;

   //DLT_LOG(gDLTContext, DLT_LOG_INFO, DLT_STRING("pclFileReadData fd: "), DLT_INT(fd));
   if(__sync_add_and_fetch(&gPclInitCounter, 0) > 0)
   {
#if USE_FILECACHE
   	if(get_file_cache_status(fd) == 1)
   	{
   		readSize = pfcReadFile(fd, buffer, buffer_size);
   	}
   	else
   	{
   		readSize = read(fd, buffer, buffer_size);
   	}
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

   if(__sync_add_and_fetch(&gPclInitCounter, 0) > 0)
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
               DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("pclFileRemove - remove()"), DLT_STRING(strerror(errno)) );
            }
         }
         else
         {
            rval = shared_DB;
            DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("pclFileRemove - no valid database context or resource not a file"));
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

   if(__sync_add_and_fetch(&gPclInitCounter, 0) > 0)
   {
      if(AccessNoLock != isAccessLocked() ) // check if access to persistent data is locked
      {
#if USE_FILECACHE
      	if(get_file_cache_status(fd) == 1)
      	{
      		rval = pfcFileSeek(fd, offset, whence);
      	}
      	else
      	{
      		 rval = lseek(fd, offset, whence);
      	}
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

   if(__sync_add_and_fetch(&gPclInitCounter, 0) > 0)
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

   if(__sync_add_and_fetch(&gPclInitCounter, 0) > 0)
   {
      if(AccessNoLock != isAccessLocked() ) // check if access to persistent data is locked
      {
      	int permission = get_file_permission(fd);
         if(permission != -1)
         {
            if(permission != PersistencePermission_ReadOnly)
            {
               // check if a backup file has to be created
               if(get_file_backup_status(fd) == 0)
               {
                  char csumBuf[ChecksumBufSize] = {0};

                  // calculate checksum
                  pclCalcCrc32Csum(fd, csumBuf);

                  // create checksum and backup file
                  pclCreateBackup(get_file_backup_path(fd), fd, get_file_checksum_path(fd), csumBuf);

                  set_file_backup_status(fd, 1);
               }

#if USE_FILECACHE
               if(get_file_cache_status(fd) == 1)
               {
               	size = pfcWriteFile(fd, buffer, buffer_size);
               }
               else
               {
                  size = write(fd, buffer, buffer_size);

						if(fsync(fd) == -1)
							DLT_LOG(gPclDLTContext, DLT_LOG_WARN, DLT_STRING("pclFileWriteData: Failed to fsync ==>!"), DLT_STRING(strerror(errno)));
               }
#else
               size = write(fd, buffer, buffer_size);
               if(get_file_cache_status(fd) == 1)
               {
               	if(fsync(fd) == -1)
               		DLT_LOG(gPclDLTContext, DLT_LOG_WARN, DLT_STRING("pclFileWriteData - Failed to fsync ==>!"), DLT_STRING(strerror(errno)));
               }
#endif
            }
            else
            {
            	DLT_LOG(gPclDLTContext, DLT_LOG_INFO, DLT_STRING("pclFileWriteData - Failed to write ==> read only file!"), DLT_STRING(get_file_backup_path(fd)));
               size = EPERS_RESOURCE_READ_ONLY;
            }
         }
			else
			{
			   size = EPERS_MAXHANDLE;
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

   if(__sync_add_and_fetch(&gPclInitCounter, 0) > 0)
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
               && pclBackupNeeded(get_raw_string(dbPath)) == CREATE_BACKUP)
            {
               snprintf(backupPath, DbPathMaxLen-1, "%s%s", dbPath, gBackupPostfix);
               snprintf(csumPath,   DbPathMaxLen-1, "%s%s", dbPath, gBackupCsPostfix);

               if((handle = pclVerifyConsistency(dbPath, backupPath, csumPath, flags)) == -1)
               {
                  DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("pclFileCreatePath - file inconsistent, recovery  N O T  possible!"));
                  return -1;
               }
               // we don't need the file handle here
               // the application calling this function must use the POSIX open() function to get an file descriptor
               if(handle > 0)
               	close(handle);
            }
            else
				{
					DLT_LOG(gPclDLTContext, DLT_LOG_INFO, DLT_STRING("pclFileCreatePath - No Backup, read only OR in blacklist!"), DLT_STRING(dbKey));
				}

            handle = get_persistence_handle_idx();

            if(handle != -1)
            {
               if(handle < MaxPersHandle)
               {
                  *size = strlen(dbPath);
                  *path = malloc((*size)+1);       // allocate 1 byte for the string termination

                  /* Check if malloc was successful */
                  if(NULL != (*path))
                  {
							memcpy(*path, dbPath, (*size));
							(*path)[(*size)] = '\0';         // terminate string

                     if(access(*path, F_OK) == -1)
                     {
								int handle = 0, cacheStatus = -1;
				            if(strstr(dbPath, WTPREFIX) != NULL)
								{
				            	cacheStatus = 0;
								}
				            else
				            {
				            	cacheStatus = 1;
				            }

								handle = pclCreateFile(*path, cacheStatus);	// file does not exist, create it.
								set_file_cache_status(handle, cacheStatus);

								if(handle == -1)
								{
									DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("pclFileCreatePath - failed to create file: "), DLT_STRING(*path));
								}
								else
								{
									if(pclFileGetDefaultData(handle, resource_id, dbContext.configKey.policy) == -1)	// try to get default data
									{
										DLT_LOG(gPclDLTContext, DLT_LOG_WARN, DLT_STRING("pclFileCreatePath - no default data available: "), DLT_STRING(resource_id));
									}
									close(handle);    // don't need the open file
								}
                     }
                     __sync_fetch_and_add(&gOpenHandleArray[handle], FileOpen); // set open flag

                     set_ossfile_handle_data(handle, dbContext.configKey.permission, 0/*backupCreated*/, backupPath, csumPath, *path);
                  }
						else
                  {
               	     handle = EPERS_DESER_ALLOCMEM;
               	     DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("pclFileCreatePath: malloc() failed for path:"),
               	                                            DLT_STRING(dbPath), DLT_STRING("With the size:"), DLT_UINT(*size));
                  }
               }
               else
					{
						set_persistence_handle_close_idx(handle);
						handle = EPERS_MAXHANDLE;
					}
            }
         }
         //
         // requested resource is not in the RCT, so create resource as local/cached.
         //
         else
         {
            // assemble file string for local cached location
            snprintf(dbPath, DbPathMaxLen, gLocalCacheFilePath, gAppId, user_no, seat_no, resource_id);
            handle = get_persistence_handle_idx();

            if(handle != -1)
            {
               if(handle < MaxPersHandle)
               {
                  snprintf(backupPath, DbPathMaxLen, "%s%s", dbPath, gBackupPostfix);
                  snprintf(csumPath,   DbPathMaxLen, "%s%s", dbPath, gBackupCsPostfix);

                  __sync_fetch_and_add(&gOpenHandleArray[handle], FileOpen); // set open flag

                  set_ossfile_handle_data(handle, PersistencePermission_ReadWrite, 0/*backupCreated*/, backupPath, csumPath, NULL);
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

   if(__sync_add_and_fetch(&gPclInitCounter, 0) > 0)
   {
   	int  permission = get_ossfile_permission(pathHandle);
      if(permission != -1)		// permission is here also used for range check
      {
         // check if a backup and checksum file needs to bel deleted
         if(permission != PersistencePermission_ReadOnly)
         {
            // remove backup file
            remove(get_ossfile_backup_path(pathHandle));  // we don't care about return value

            // remove checksum file
            remove(get_ossfile_checksum_path(pathHandle));    // we don't care about return value

         }
         free(get_ossfile_file_path(pathHandle));

         __sync_fetch_and_sub(&gOpenHandleArray[pathHandle], FileClosed);   // set closed flag

         set_persistence_handle_close_idx(pathHandle);			// TODO

         set_ossfile_file_path(pathHandle, NULL);
         rval = 1;
      }
      else
      {
        rval = EPERS_MAXHANDLE;
      }
   }

   return rval;
}




int pclFileGetDefaultData(int handle, const char* resource_id, int policy)
{
	// check if there is default data available
	char pathPrefix[DbPathMaxLen]  = { [0 ... DbPathMaxLen-1] = 0};
	char defaultPath[DbPathMaxLen] = { [0 ... DbPathMaxLen-1] = 0};
	int defaultHandle = -1;
	int rval = 0;

	// create path to default data
	if(policy == PersistencePolicy_wc)
	{
		snprintf(pathPrefix, DbPathMaxLen, gLocalCachePath, gAppId);
	}
	else if(policy == PersistencePolicy_wt)
	{
		snprintf(pathPrefix, DbPathMaxLen, gLocalWtPath, gAppId);
	}

	// first check for  c o n f i g u r a b l e  default data
	snprintf(defaultPath, DbPathMaxLen, "%s%s/%s", pathPrefix, PERS_ORG_CONFIG_DEFAULT_DATA_FOLDER_NAME_, resource_id);
	if(access(defaultPath, F_OK) )
	{
      // if no  c o n f i g u r  a b l e  default data available, check for  d e f a u l t  data
      snprintf(defaultPath, DbPathMaxLen, "%s%s/%s", pathPrefix, PERS_ORG_DEFAULT_DATA_FOLDER_NAME_, resource_id);
	}

	defaultHandle = open(defaultPath, O_RDONLY);
	if(defaultHandle != -1)	// check if default data is available
	{
		// copy default data
		struct stat buf;
		memset(&buf, 0, sizeof(buf));

		fstat(defaultHandle, &buf);
		rval = sendfile(handle, defaultHandle, 0, buf.st_size);
		if(rval != -1)
		{
			rval = lseek(handle, 0, SEEK_SET); // set fd back to beginning of the file
		}
		else
		{
			DLT_LOG(gPclDLTContext, DLT_LOG_WARN, DLT_STRING("pclFileGetDefaultData - failed to copy file "), DLT_STRING(strerror(errno)));
		}

		close(defaultHandle);
	}
	else
	{
		rval = -1; // no default data available
	}

	return rval;
}

