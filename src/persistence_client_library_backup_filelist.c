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
#include "../include_protected/crc32.h"
#include "../include_protected/persistence_client_library_data_organization.h"


#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/sendfile.h>


/// structure definition for a key value item
typedef struct _key_value_s
{
   unsigned int key;
   char*        value;
}key_value_s;


void  key_val_rel(void *p);

void* key_val_dup(void *p);

int key_val_cmp(const void *p1, const void *p2 );



/// the size of the token array
enum configConstants
{
   TOKENARRAYSIZE = 255
};


const char gCharLookup[] =
{
   0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,  // from 0x0 (NULL)  to 0x1F (unit seperator)
   0,1,1,1,1,1,1,1,  1,1,1,1,1,1,1,1,  1,1,1,1,1,1,1,1,  1,1,1,1,1,1,1,1,  // from 020 (space) to 0x2F (?)
   1,1,1,1,1,1,1,1,  1,1,1,1,1,1,1,1,  1,1,1,1,1,1,1,1,  1,1,1,1,1,1,1,1,  // from 040 (@)     to 0x5F (_)
   1,1,1,1,1,1,1,1,  1,1,1,1,1,1,1,1,  1,1,1,1,1,1,1,1,  1,1,1,1,1,1,1     // from 060 (')     to 0x7E (~)
};


char* gpConfigFileMap = 0;
char* gpTokenArray[TOKENARRAYSIZE] = {0};
int gTokenCounter = 0;
unsigned int gConfigFileSize = 0;


/// the rb tree
static jsw_rbtree_t *gRb_tree_bl = NULL;

void fillCharTokenArray()
{
   unsigned int i=0;
   int blankCount=0;
   char* tmpPointer = gpConfigFileMap;

   // set the first pointer to the start of the file
   gpTokenArray[blankCount] = tmpPointer;
   blankCount++;

   while(i < gConfigFileSize)
   {
      if(   ((unsigned int)*tmpPointer < 127)
         && ((unsigned int)*tmpPointer >= 0))
	   {
		   if(1 != gCharLookup[(unsigned int)*tmpPointer])
		   {
			   *tmpPointer = 0;

			   // check if we are at the end of the token array
			   if(blankCount >= TOKENARRAYSIZE)
			   {
				   break;
			   }
			   gpTokenArray[blankCount] = tmpPointer+1;
			   blankCount++;
			   gTokenCounter++;
		   }
	   }
      tmpPointer++;
	   i++;
   }
}


void createAndStoreFileNames()
{
   int i= 0, j =0;
   char path[128];
   const char* gFilePostFix                = ".pers";
   const char* gKeyPathFormat              = "/%s/%s/%s/%s/%s%s";
   key_value_s* item;

   // creat new tree
   gRb_tree_bl = jsw_rbnew(key_val_cmp, key_val_dup, key_val_rel);

   if(gRb_tree_bl != NULL)
   {

      for(i=0; i<128; i++)
      {
         // assemble path
         snprintf(path, 128, gKeyPathFormat, gpTokenArray[j+2],      // storage type
                                             gpTokenArray[j+3],      // policy id
                                             gpTokenArray[j+4],      // profileID
                                             gpTokenArray[j],        // application id
                                             gpTokenArray[j+1],      // filename
                                             gFilePostFix);          // file postfix

         // asign key and value to the rbtree item
         item = malloc(sizeof(key_value_s));
         if(item != NULL)
         {
            item->key = pclCrc32(0, (unsigned char*)path, strlen(path));
            // we don't need the path name here, we just need to know that this key is available in the tree
            item->value = "";
            jsw_rbinsert(gRb_tree_bl, item);
            free(item);
         }
         j+=5;
         if(gpTokenArray[j] == NULL)
         {
            break;
         }
      }
   }

}


int readBlacklistConfigFile(const char* filename)
{
   int fd = 0,
       status = 0,
       rval = 0;

   struct stat buffer;

   if(filename != NULL)
   {

	   memset(&buffer, 0, sizeof(buffer));
	   status = stat(filename, &buffer);
	   if(status != -1)
	   {
		  gConfigFileSize = buffer.st_size;
	   }

	   fd = open(filename, O_RDONLY);
	   if (fd == -1)
	   {
		  DLT_LOG(gDLTContext, DLT_LOG_WARN, DLT_STRING("configReader::readConfigFile ==> Error file open"), DLT_STRING(filename), DLT_STRING(strerror(errno)) );
		  return -1;
	   }

	   // check for empty file
	   if(gConfigFileSize == 0)
	   {
		  DLT_LOG(gDLTContext, DLT_LOG_WARN, DLT_STRING("configReader::readConfigFile ==> Error file size is 0:"), DLT_STRING(filename));
		  close(fd);
		  return -1;
	   }

	   // map the config file into memory
	   gpConfigFileMap = (char*)mmap(0, gConfigFileSize, PROT_WRITE, MAP_PRIVATE, fd, 0);

	   if (gpConfigFileMap == MAP_FAILED)
	   {
		  gpConfigFileMap = 0;
		  close(fd);
		  DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("configReader::readConfigFile ==> Error mapping the file:"), DLT_STRING(filename), DLT_STRING(strerror(errno)) );

		  return -1;
	   }

	   // reset the token counter
	   gTokenCounter = 0;

	   fillCharTokenArray();

	   // create filenames and store them in the tree
	   createAndStoreFileNames();

	   munmap(gpConfigFileMap, gConfigFileSize);

	   close(fd);
   }
   else
   {
	   rval = -1;
   }

   return rval;
}



int need_backup_key(unsigned int key)
{
   int rval = 1;
   key_value_s* item = NULL;
   key_value_s* foundItem = NULL;

   item = malloc(sizeof(key_value_s));
   if(item != NULL && gRb_tree_bl != NULL)
   {
      item->key = key;
      foundItem = (key_value_s*)jsw_rbfind(gRb_tree_bl, item);
      if(foundItem != NULL)
      {
         rval = 0;
      }
      free(item);
   }
   else
   {
      if(item!=NULL)
	     free(item);

      rval = -1;
      DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("need_backup_key ==> item or gRb_tree_bl is NULL"));
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
      // duplicate hash key
     dst->key = src->key;

     // duplicate value
     dst->value = malloc(value_size);
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


int pclBackupDoFileCopy(int srcFd, int dstFd)
{
   struct stat buf;
   memset(&buf, 0, sizeof(buf));

   fstat(srcFd, &buf);
   return sendfile(dstFd, srcFd, 0, buf.st_size);
}


int pclCreateFile(const char* path)
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
      handle = open(createPath, O_CREAT|O_RDWR |O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
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

   return handle;
}



int pclRecoverFromBackup(int backupFd, const char* original)
{
   int handle = 0;

   handle = open(original, O_TRUNC | O_RDWR);
   if(handle != -1)
   {
      // copy data from one file to another
      if((handle = pclBackupDoFileCopy(backupFd, handle)) == -1)
      {
         DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("pclRecoverFromBackup => couldn't write whole buffer"));
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

      handle = pclCreateFile(pathToCreate);
      close(handle); // don't need the open file
   }

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
      if((readSize = pclBackupDoFileCopy(srcfd, dstFd)) == -1)
      {
         DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("pcl_create_backup => error copying file"));
      }

      if(close(dstFd) == -1)
      {
         DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("pcl_create_backup => error closing fd"));
      }

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
            crc = pclCrc32(crc, (unsigned char*)buf, statBuf.st_size);
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
   return need_backup_key(pclCrc32(0, (const unsigned char*)path, strlen(path)));
}



int pclGetPosixPermission(PersistencePermission_e permission)
{
   int posixPerm = 0;

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



