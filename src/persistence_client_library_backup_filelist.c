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
            item->key = crc32(0, (unsigned char*)path, strlen(path));
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



int need_backup_path(const char* path)
{
   return need_backup_key(crc32(0, (const unsigned char*)path, strlen(path)));
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


