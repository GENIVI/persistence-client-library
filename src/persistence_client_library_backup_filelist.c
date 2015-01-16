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
 * @file           persistence_client_library_backup_filelist.c
 * @ingroup        Persistence client library
 * @author         Ingo Huerner
 * @brief          Implementation of persistence client library backup filelist
 * @see
 */

#include "persistence_client_library_backup_filelist.h"
#include "persistence_client_library_handle.h"
#include "rbtree.h"

#include "crc32.h"
#include "persistence_client_library_data_organization.h"


#if USE_FILECACHE
   #include <persistence_file_cache.h>
#endif


#include <sys/mman.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/sendfile.h>


/// structure definition for a key value item
typedef struct _key_value_s
{
   unsigned int key;
   char*        value;
}key_value_s;


static void  key_val_rel(void *p);

static void* key_val_dup(void *p);

static char* gpTokenArray[TOKENARRAYSIZE] = {0};

/// the rb tree
static jsw_rbtree_t *gRb_tree_bl = NULL;


// local function prototypes
static int need_backup_key(unsigned int key);
static int key_val_cmp(const void *p1, const void *p2 );

static void fillFileBackupCharTokenArray(unsigned int customConfigFileSize, char* fileMap)
{
   unsigned int i=0;
   int tokenCounter = 0;
   int blankCount=0;
   char* tmpPointer = fileMap;

   gpTokenArray[blankCount] = tmpPointer;    // set the first pointer to the start of the file
   blankCount++;

   while(i < customConfigFileSize)
   {
      if(   ((unsigned int)*tmpPointer < 127)
         && ((unsigned int)*tmpPointer >= 0))
	   {
		   if(1 != gCharLookup[(unsigned int)*tmpPointer])
		   {
			   *tmpPointer = 0;

			   if(blankCount >= TOKENARRAYSIZE)    // check if we are at the end of the token array
			   {
				   break;
			   }
			   gpTokenArray[blankCount] = tmpPointer+1;
			   blankCount++;
			   tokenCounter++;
		   }
	   }
      tmpPointer++;
	   i++;
   }
}


static void createAndStoreFileNames()
{
   int i= 0;
   char path[128] = {0};
   key_value_s* item;

   // create new tree
   gRb_tree_bl = jsw_rbnew(key_val_cmp, key_val_dup, key_val_rel);

   if(gRb_tree_bl != NULL)
   {
		while( i < TOKENARRAYSIZE )
		{
			if(gpTokenArray[i+1]  != 0 )
			{
				memset(path, 0, sizeof(path));
				snprintf(path, 128, "%s", gpTokenArray[i]);    // storage type

				item = malloc(sizeof(key_value_s));    // asign key and value to the rbtree item
				if(item != NULL)
				{
					//printf("createAndStoreFileNames => path: %s\n", path);
					item->key = pclCrc32(0, (unsigned char*)path, strlen(path));
					// we don't need the path name here, we just need to know that this key is available in the tree
					item->value = "";
					jsw_rbinsert(gRb_tree_bl, item);
					free(item);
				}
				i+=1;
			}
			else
			{
				break;
			}
      }
   }

}


int readBlacklistConfigFile(const char* filename)
{
   int rval = 0;

   if(filename != NULL)
   {
		struct stat buffer;

	   memset(&buffer, 0, sizeof(buffer));
	   if(stat(filename, &buffer) != -1)
	   {
			if(buffer.st_size > 0)	   // check for empty file
			{
				char* configFileMap = 0;
				int fd = open(filename, O_RDONLY);

				if(fd == -1)
				{
				  DLT_LOG(gPclDLTContext, DLT_LOG_WARN, DLT_STRING("blacklist - Err file open"), DLT_STRING(filename), DLT_STRING(strerror(errno)) );
				  return EPERS_COMMON;
				}

				configFileMap = (char*)mmap(0, buffer.st_size, PROT_WRITE, MAP_PRIVATE, fd, 0);  // map the configuration file into memory

				if(configFileMap == MAP_FAILED)
				{
				  close(fd);
				  DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("blacklist - Err mapping file:"), DLT_STRING(filename), DLT_STRING(strerror(errno)) );

				  return EPERS_COMMON;
				}

				fillFileBackupCharTokenArray(buffer.st_size, configFileMap);

				createAndStoreFileNames();    // create filenames and store them in the tree

				munmap(configFileMap, buffer.st_size);

				close(fd);
			}
			else
			{
				DLT_LOG(gPclDLTContext, DLT_LOG_WARN, DLT_STRING("blacklist - Err config file size is 0:"), DLT_STRING(filename));
				return EPERS_COMMON;
			}
	   }
	   else
	   {
	   	DLT_LOG(gPclDLTContext, DLT_LOG_DEBUG, DLT_STRING("blacklist - failed to stat() conf file:"), DLT_STRING(filename));
	   	return EPERS_COMMON;
	   }
	}
   else
   {
   	DLT_LOG(gPclDLTContext, DLT_LOG_WARN, DLT_STRING("blacklist - config file name is NULL:"));
	   rval = EPERS_COMMON;
   }

   return rval;
}



int need_backup_key(unsigned int key)
{
   int rval = CREATE_BACKUP;
   key_value_s* item = NULL;

   item = malloc(sizeof(key_value_s));
   if(item != NULL && gRb_tree_bl != NULL)
   {
   	key_value_s* foundItem = NULL;
      item->key = key;
      foundItem = (key_value_s*)jsw_rbfind(gRb_tree_bl, item);
      if(foundItem != NULL)
      {
         rval = DONT_CREATE_BACKUP;
      }
      free(item);
   }
   else
   {
      if(item!=NULL)
	     free(item);

      rval = CREATE_BACKUP;
   }

   return rval;
}


/// compare function for tree key_value_s item
int key_val_cmp(const void *p1, const void *p2 )
{
   int rval = -1;
   key_value_s* first;
   key_value_s* second;

   first  = (key_value_s*)p1;
   second = (key_value_s*)p2;

   if(second->key == first->key)
   {
      rval = 0;
   }
   else if(second->key < first->key)
   {
      rval = -1;
   }
   else
   {
      rval = 1;
   }

   return rval;
 }

/// duplicate function for key_value_s item
void* key_val_dup(void *p)
{
   int value_size = 0;
   key_value_s* src = NULL;
   key_value_s* dst = NULL;

   src = (key_value_s*)p;
   value_size = strlen(src->value)+1;

   // allocate memory for node
   dst = malloc(sizeof(key_value_s));
   if(dst != NULL)
   {
     dst->key = src->key;               // duplicate hash key

     dst->value = malloc(value_size);  // duplicate value
     if(dst->value != NULL)
        strncpy(dst->value, src->value, value_size);
   }


   return dst;
}

/// release function for key_value_s item
void  key_val_rel(void *p )
{
   key_value_s* rel = NULL;
   rel = (key_value_s*)p;

   if(rel->value != NULL)
      free(rel->value);

   if(rel != NULL)
      free(rel);
}


static int pclBackupDoFileCopy(int srcFd, int dstFd)
{
   struct stat buf;
   int rval = 0;
   memset(&buf, 0, sizeof(buf));

   fstat(srcFd, &buf);
   rval = (int)sendfile(dstFd, srcFd, 0, buf.st_size);

   // Reset file position pointer of destination file 'dstFd'
   lseek(dstFd, 0, SEEK_SET);

   return rval;
}


int pclCreateFile(const char* path, int chached)
{
   const char* delimiters = "/\n";   // search for blank and end of line
   char* tokenArray[24];
   char thePath[DbPathMaxLen] = {0};
   int numTokens = 0, i = 0, validPath = 1;
   int handle = -1;

   strncpy(thePath, path, DbPathMaxLen);

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

#if USE_FILECACHE
      if(chached == 0)
		{
      	handle = open(createPath, O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
		}
      else
      {
      	handle = pfcOpenFile(createPath, CreateFile);
      }
#else
      handle = open(createPath, O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
#endif
   }
   else
   {
      DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("createFile - no valid path to create: "), DLT_STRING(path) );
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
   if((backupAvail == 0) && (csumAvail == 0) )
   {
      DLT_LOG(gPclDLTContext, DLT_LOG_INFO, DLT_STRING("verifyConsist- there is a backup file AND csum"));

      fdBackup = open(backupPath,  O_RDONLY);      // calculate checksum form backup file
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
                  handle = pclRecoverFromBackup(fdBackup, origPath);    // checksum matches ==> replace with original file
               }
               else
               {
                  handle = open(origPath, openFlags);    // checksum does not match, check checksum with original file
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
      DLT_LOG(gPclDLTContext, DLT_LOG_INFO, DLT_STRING("verifyConsist - there is ONLY a csum file"));

      fdCsum = open(csumPath,  O_RDONLY);
      if(fdCsum != -1)
      {
         readSize = read(fdCsum, csumBuf, ChecksumBufSize);
         if(readSize <= 0)
         {
            DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("verifyConsist - read csum: invalid readSize"));
         }
         close(fdCsum);

         handle = open(origPath, openFlags);    // calculate the checksum form the original file to see if it matches
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
      DLT_LOG(gPclDLTContext, DLT_LOG_INFO, DLT_STRING("verifyConsist - there is ONLY a backup file"));

      fdBackup = open(backupPath,  O_RDONLY);      // calculate checksum form backup file
      if(fdBackup != -1)
      {
         pclCalcCrc32Csum(fdBackup, backCsumBuf);
         close(fdBackup);

         handle = open(origPath, openFlags);       // calculate the checksum form the original file to see if it matches
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

   if(handle == -1)     // if we are in an inconsistent state: delete file, backup and checksum
   {
      remove(origPath);
      remove(backupPath);
      remove(csumPath);
   }

   return handle;
}



int pclRecoverFromBackup(int backupFd, const char* original)
{
   int handle = 0;

   handle = open(original, O_TRUNC | O_RDWR);
   if(handle != -1)
   {
      if(pclBackupDoFileCopy(backupFd, handle) == -1)    // copy data from one file to another
      {
         DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("rBackup - couldn't write whole buffer"));
      }
   }

   return handle;
}



int pclCreateBackup(const char* dstPath, int srcfd, const char* csumPath, const char* csumBuf)
{
   int dstFd = 0, csfd = 0;
   int readSize = -1;

   if(access(dstPath, F_OK) != 0)
   {
      int handle = -1;
      char pathToCreate[DbPathMaxLen] = {0};
      strncpy(pathToCreate, dstPath, DbPathMaxLen);

      handle = pclCreateFile(pathToCreate, 0);
      close(handle);       // don't need the open file
   }

   // create checksum file and and write checksum
   csfd = open(csumPath, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
   if(csfd != -1)
   {
      int csumSize = strlen(csumBuf);
      if(write(csfd, csumBuf, csumSize) != csumSize)
      {
         DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("cBackup - failed write csum to file"));
      }
      close(csfd);
   }
   else
   {
      DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("cBackup - failed create csum file:"), DLT_STRING(strerror(errno)) );
   }

   // create backup file, user and group has read/write permission, others have read permission
   dstFd = open(dstPath, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
   if(dstFd != -1)
   {
      off_t curPos = 0;

      curPos = lseek(srcfd, 0, SEEK_CUR);		// remember the current position
      lseek(srcfd, 0, SEEK_SET);					// set to beginning of file

      // copy data from one file to another
      if((readSize = pclBackupDoFileCopy(srcfd, dstFd)) == -1)
      {
         DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("cBackup - err copying file"));
      }

      if(close(dstFd) == -1)
      {
         DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("cBackup - err closing fd"));
      }

      lseek(srcfd, curPos, SEEK_SET);     // set back to the position
   }
   else
   {
      DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("cBackup - failed open backup file"),
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
         curPos = lseek(fd, 0, SEEK_CUR);    // remember the current position

         if(curPos != 0)
         {
            lseek(fd, 0, SEEK_SET);          // set to beginning of the file
         }

         while((rval = read(fd, buf, statBuf.st_size)) > 0)
         {
            unsigned int crc = 0;
            crc = pclCrc32(crc, (unsigned char*)buf, statBuf.st_size);
            snprintf(crc32sum, ChecksumBufSize-1, "%x", crc);
         }

         lseek(fd, curPos, SEEK_SET);        // set back to the position

         free(buf);
      }
      else
      {
      	rval = -1;
      }
   }
   return rval;
}



int pclBackupNeeded(const char* path)
{
   return need_backup_key(pclCrc32(0, (const unsigned char*)path, strlen(path)));
}



int pclGetPosixPermission(PersistencePermission_e permission)
{
   int posixPerm = -1;

   switch( (int)permission)
   {
   case PersistencePermission_ReadWrite:
      posixPerm = O_RDWR;
      break;
   case PersistencePermission_ReadOnly:
      posixPerm = O_RDONLY;
      break;
   case PersistencePermission_WriteOnly:
      posixPerm = O_WRONLY;
      break;
   default:
      posixPerm = O_RDONLY;
      break;
   }

   return posixPerm;
}



