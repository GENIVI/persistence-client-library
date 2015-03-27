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
 * @file           persistence_client_library_handle.c
 * @ingroup        Persistence client library handle
 * @author         Ingo Huerner
 * @brief          Implementation of the persistence client library handle
 *                 Library provides an API to access persistent data
 * @see
 */

#include "persistence_client_library_handle.h"
#include "persistence_client_library_tree_helper.h"

#include <pthread.h>


pthread_mutex_t gKeyHandleAccessMtx      = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t gFileHandleAccessMtx     = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t gOssFileHandleAccessMtx  = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t gMtx = PTHREAD_MUTEX_INITIALIZER;

/// tree to store key handle information
static jsw_rbtree_t *gKeyHandleTree = NULL;

/// tree to store file handle information
static jsw_rbtree_t *gFileHandleTree = NULL;


static jsw_rbtree_t *gOssFileHandleTree = NULL;




// open file descriptor handle array
int gOpenFdArray[MaxPersHandle] = { [0 ...MaxPersHandle-1] = 0 };
// handle array
int gOpenHandleArray[MaxPersHandle] = { [0 ...MaxPersHandle-1] = 0 };

// handle index
static int gHandleIdx = 1;
/// free handle array
static int gFreeHandleArray[MaxPersHandle] = { [0 ...MaxPersHandle-1] = 0 };
/// free handle array head index
static int gFreeHandleIdxHead = 0;


void deleteHandleTrees(void)
{
   if(gKeyHandleTree != NULL)
   {
      jsw_rbdelete(gKeyHandleTree);
      gKeyHandleTree = NULL;
   }

   if(gFileHandleTree != NULL)
   {
      jsw_rbdelete(gFileHandleTree);
      gFileHandleTree = NULL;
   }

   if(gOssFileHandleTree != NULL)
   {
      jsw_rbdelete(gOssFileHandleTree);
      gOssFileHandleTree = NULL;
   }
}


int get_persistence_handle_idx()
{
   int handle = 0;

   if(pthread_mutex_lock(&gMtx) == 0)
   {
      if(gFreeHandleIdxHead > 0)   // check if we have a free spot in the array before the current max
      {
         handle = gFreeHandleArray[--gFreeHandleIdxHead];
      }
      else
      {
         if(gHandleIdx < MaxPersHandle-1)
         {
            handle = gHandleIdx++;  // no free spot before current max, increment handle index
         }
         else
         {
            handle = EPERS_MAXHANDLE;
            DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("gPersHidx - max open handles: "), DLT_INT(MaxPersHandle));
         }
      }
      pthread_mutex_unlock(&gMtx);
   }
   return handle;
}


void set_persistence_handle_close_idx(int handle)
{
   if(pthread_mutex_lock(&gMtx) == 0)
   {
      if(gFreeHandleIdxHead < MaxPersHandle)
      {
         gFreeHandleArray[gFreeHandleIdxHead++] = handle;
      }
      pthread_mutex_unlock(&gMtx);
   }
}


void close_all_persistence_handle()
{
   if(pthread_mutex_lock(&gMtx) == 0)
   {
      // "free" all handles
      memset(gFreeHandleArray, 0, sizeof(gFreeHandleArray));
      memset(gOpenHandleArray, 0, sizeof(gOpenHandleArray));
      memset(gOpenFdArray,     0, sizeof(gOpenFdArray));

      // reset variables
      gHandleIdx = 1;
      gFreeHandleIdxHead = 0;

      pthread_mutex_unlock(&gMtx);
   }
}


int set_key_handle_data(int idx, const char* id, unsigned int ldbid,  unsigned int user_no, unsigned int seat_no)
{
	int handle = -1;

	if(pthread_mutex_lock(&gKeyHandleAccessMtx) == 0)
	{
	   KeyHandleTreeItem_s* item = NULL;

	   if(gKeyHandleTree == NULL)
	   {
	      gKeyHandleTree = jsw_rbnew(kh_key_val_cmp, kh_key_val_dup, kh_key_val_rel);
	   }

	   item = malloc(sizeof(KeyHandleTreeItem_s));    // assign key and value to the rbtree item
      if(item != NULL)
      {
         item->key = idx;

         item->value.keyHandle.ldbid   = ldbid;
         item->value.keyHandle.user_no = user_no;
         item->value.keyHandle.seat_no = seat_no;
         strncpy(item->value.keyHandle.resource_id, id, PERS_DB_MAX_LENGTH_KEY_NAME);
         item->value.keyHandle.resource_id[PERS_DB_MAX_LENGTH_KEY_NAME-1] = '\0'; // Ensures 0-Termination

         jsw_rbinsert(gKeyHandleTree, item);

         free(item);

         handle = idx;
      }

		pthread_mutex_unlock(&gKeyHandleAccessMtx);
   }

	return handle;
}


int get_key_handle_data(int idx, PersistenceKeyHandle_s* handleStruct)
{
	int rval = -1;

	if(pthread_mutex_lock(&gKeyHandleAccessMtx) == 0)
	{
      if(gKeyHandleTree != NULL)
      {
         KeyHandleTreeItem_s* item = malloc(sizeof(KeyHandleTreeItem_s));
         if(item != NULL)
         {
            KeyHandleTreeItem_s* foundItem = NULL;
            item->key = idx;
            foundItem = (KeyHandleTreeItem_s*)jsw_rbfind(gKeyHandleTree, item);
            if(foundItem != NULL)
            {
               handleStruct->ldbid   = foundItem->value.keyHandle.ldbid;
               handleStruct->user_no = foundItem->value.keyHandle.user_no;
               handleStruct->seat_no = foundItem->value.keyHandle.seat_no;
               strncpy(handleStruct->resource_id, foundItem->value.keyHandle.resource_id, PERS_DB_MAX_LENGTH_KEY_NAME);
               handleStruct->resource_id[PERS_DB_MAX_LENGTH_KEY_NAME-1] = '\0'; // Ensures 0-Termination
               rval = 0;
            }
            free(item);
         }
      }

		pthread_mutex_unlock(&gKeyHandleAccessMtx);
	}

   return rval;
}


void init_key_handle_array()
{
	if(pthread_mutex_lock(&gKeyHandleAccessMtx) == 0)
	{
      if(gKeyHandleTree != NULL)
      {
         jsw_rbdelete (gKeyHandleTree);
      }

      gKeyHandleTree = jsw_rbnew(kh_key_val_cmp, kh_key_val_dup, kh_key_val_rel);

		pthread_mutex_unlock(&gKeyHandleAccessMtx);
	}
}


void clear_key_handle_array(int idx)
{
   if(pthread_mutex_lock(&gKeyHandleAccessMtx) == 0)
   {
      if(gKeyHandleTree != NULL)
      {
         KeyHandleTreeItem_s* item = malloc(sizeof(KeyHandleTreeItem_s));
         if(item != NULL)
         {
            item->key = idx;

            if(jsw_rberase(gKeyHandleTree, item) == 0)
            {
               DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("clear_key_handle_array - failed remove idx: "), DLT_INT(idx));
            }
            free(item);
         }
      }

      pthread_mutex_unlock(&gKeyHandleAccessMtx);
   }
}


int remove_file_handle_data(int idx)
{
   int rval = -1;

   if(pthread_mutex_lock(&gFileHandleAccessMtx) == 0)
   {
      if(gFileHandleTree != NULL)
      {
         FileHandleTreeItem_s* item = malloc(sizeof(FileHandleTreeItem_s));    // assign key and value to the rbtree item
         if(item != NULL)
         {
            item->key = idx;
            rval = jsw_rberase(gFileHandleTree, item);
         }
      }

      pthread_mutex_unlock(&gFileHandleAccessMtx);
   }

   return rval;
}

int set_file_handle_data(int idx, PersistencePermission_e permission, const char* backup, const char* csumPath, char* filePath)
{
	int rval = -1;

	if(pthread_mutex_lock(&gFileHandleAccessMtx) == 0)
	{
		FileHandleTreeItem_s* item = NULL;

      if(gFileHandleTree == NULL)
      {
         gFileHandleTree = jsw_rbnew(fh_key_val_cmp, fh_key_val_dup, fh_key_val_rel);
      }

      item = malloc(sizeof(FileHandleTreeItem_s));    // assign key and value to the rbtree item
      if(item != NULL)
      {
         FileHandleTreeItem_s* foundItem = NULL;
         item->key = idx;
         foundItem = (FileHandleTreeItem_s*)jsw_rbfind(gFileHandleTree, item);
         if(foundItem == NULL)
         {
            item->key = idx;

            item->value.fileHandle.permission    = permission;
            item->value.fileHandle.backupCreated = 0;             // set to 0 by default
            item->value.fileHandle.cacheStatus   = -1;            // set to -1 by default
            item->value.fileHandle.userId        = 0;             // default value
            item->value.fileHandle.filePath      = filePath;

            strncpy(item->value.fileHandle.backupPath, backup, PERS_ORG_MAX_LENGTH_PATH_FILENAME);
            item->value.fileHandle.backupPath[PERS_ORG_MAX_LENGTH_PATH_FILENAME-1] = '\0'; // Ensures 0-Termination

            strncpy(item->value.fileHandle.csumPath, csumPath, PERS_ORG_MAX_LENGTH_PATH_FILENAME);
            item->value.fileHandle.csumPath[PERS_ORG_MAX_LENGTH_PATH_FILENAME-1] = '\0'; // Ensures 0-Termination

            //debugFileItem("set_file_handle_data => insert", item);
            jsw_rbinsert(gFileHandleTree, item);


         }
         else
         {
            FileHandleTreeItem_s* newItem = malloc(sizeof(FileHandleTreeItem_s));
            if(newItem != NULL)
            {
               memcpy(newItem->value.payload , foundItem->value.payload, sizeof(FileHandleData_u) ); // duplicate value

               newItem->key = idx;

               newItem->value.fileHandle.permission    = permission;
               newItem->value.fileHandle.filePath      = filePath;

               strncpy(newItem->value.fileHandle.backupPath, backup, PERS_ORG_MAX_LENGTH_PATH_FILENAME);
               newItem->value.fileHandle.backupPath[PERS_ORG_MAX_LENGTH_PATH_FILENAME-1] = '\0'; // Ensures 0-Termination

               strncpy(newItem->value.fileHandle.csumPath, csumPath, PERS_ORG_MAX_LENGTH_PATH_FILENAME);
               newItem->value.fileHandle.csumPath[PERS_ORG_MAX_LENGTH_PATH_FILENAME-1] = '\0'; // Ensures 0-Termination

               //debugFileItem("set_file_backup_status => erase => foundItem", foundItem);
               jsw_rberase(gFileHandleTree, foundItem);

               //debugFileItem("set_file_backup_status => insert => newItem", newItem);
               jsw_rbinsert(gFileHandleTree, newItem);

               free(newItem);
            }
         }
         free(item);
         rval = 0;
      }

		pthread_mutex_unlock(&gFileHandleAccessMtx);
	}

	return rval;
}


int get_file_permission(int idx)
{
	int permission = (int)PersistencePermission_LastEntry;

	if(pthread_mutex_lock(&gFileHandleAccessMtx) == 0)
	{
      if(gFileHandleTree != NULL)
      {
         FileHandleTreeItem_s* item = malloc(sizeof(FileHandleTreeItem_s));
         if(item != NULL)
         {
            FileHandleTreeItem_s* foundItem = NULL;
            item->key = idx;
            foundItem = (FileHandleTreeItem_s*)jsw_rbfind(gFileHandleTree, item);
            if(foundItem != NULL)
            {
               permission = foundItem->value.fileHandle.permission;
               //debugFileItem("get_file_permission => foundItem", foundItem);
            }
            else
            {
               permission = -1;
            }
            free(item);
         }
      }
		pthread_mutex_unlock(&gFileHandleAccessMtx);
	}
	return permission;
}


char* get_file_backup_path(int idx)
{
   char* charPtr = NULL;
   if(pthread_mutex_lock(&gFileHandleAccessMtx) == 0)
   {
      if(gFileHandleTree != NULL)
      {
         FileHandleTreeItem_s* item = malloc(sizeof(FileHandleTreeItem_s));
         if(item != NULL)
         {
            FileHandleTreeItem_s* foundItem = NULL;
            item->key = idx;
            foundItem = (FileHandleTreeItem_s*)jsw_rbfind(gFileHandleTree, item);
            if(foundItem != NULL)
            {
               charPtr = foundItem->value.fileHandle.backupPath;
               //debugFileItem("get_file_backup_path => foundItem", foundItem);
            }
            free(item);
         }
      }

      pthread_mutex_unlock(&gFileHandleAccessMtx);
   }
	return charPtr;
}

char* get_file_checksum_path(int idx)
{
   char* charPtr = NULL;
   if(pthread_mutex_lock(&gFileHandleAccessMtx) == 0)
   {
      if(gFileHandleTree != NULL)
      {
         FileHandleTreeItem_s* item = malloc(sizeof(FileHandleTreeItem_s));
         if(item != NULL)
         {
            FileHandleTreeItem_s* foundItem = NULL;
            item->key = idx;
            foundItem = (FileHandleTreeItem_s*)jsw_rbfind(gFileHandleTree, item);
            if(foundItem != NULL)
            {
               charPtr = foundItem->value.fileHandle.csumPath;
               //debugFileItem("get_file_checksum_path => foundItem", foundItem);
            }
            free(item);
         }
      }
      pthread_mutex_unlock(&gFileHandleAccessMtx);
   }
	return charPtr;
}


void set_file_backup_status(int idx, int status)
{
	if(pthread_mutex_lock(&gFileHandleAccessMtx) == 0)
	{
	   FileHandleTreeItem_s* item = malloc(sizeof(FileHandleTreeItem_s));

      if(gFileHandleTree == NULL)
      {
         gFileHandleTree = jsw_rbnew(fh_key_val_cmp, fh_key_val_dup, fh_key_val_rel);
      }

      if(item != NULL)
      {
         FileHandleTreeItem_s* foundItem = NULL;
         item->key = idx;
         foundItem = (FileHandleTreeItem_s*)jsw_rbfind(gFileHandleTree, item);
         if(foundItem == NULL)
         {
            item->value.fileHandle.backupCreated = status;

            item->value.fileHandle.permission    = PersistencePermission_LastEntry;
            item->value.fileHandle.cacheStatus   = -1;            // set to -1 by default
            item->value.fileHandle.userId        = 0;             // default value
            item->value.fileHandle.filePath      = NULL;

            //debugFileItem("set_file_backup_status => insert => item", item);
            jsw_rbinsert(gFileHandleTree, item);
         }
         else
         {
            FileHandleTreeItem_s* newItem = malloc(sizeof(FileHandleTreeItem_s));
            if(newItem != NULL)
            {
               memcpy(newItem->value.payload , foundItem->value.payload, sizeof(FileHandleData_u) ); // duplicate value

               newItem->key = idx;
               newItem->value.fileHandle.backupCreated = status;

               //debugFileItem("set_file_backup_status => erase => foundItem", foundItem);
               jsw_rberase(gFileHandleTree, foundItem);

               //debugFileItem("set_file_backup_status => insert => newItem", newItem);
               jsw_rbinsert(gFileHandleTree, newItem);

               free(newItem);
            }
         }
         free(item);
      }
		pthread_mutex_unlock(&gFileHandleAccessMtx);
	}
}

int get_file_backup_status(int idx)
{
   int backup = -1;
   if(pthread_mutex_lock(&gFileHandleAccessMtx) == 0)
   {
      if(gFileHandleTree != NULL)
      {
         FileHandleTreeItem_s* item = malloc(sizeof(FileHandleTreeItem_s));
         if(item != NULL)
         {
            FileHandleTreeItem_s* foundItem = NULL;
            item->key = idx;
            foundItem = (FileHandleTreeItem_s*)jsw_rbfind(gFileHandleTree, item);
            if(foundItem != NULL)
            {
               backup = foundItem->value.fileHandle.backupCreated;
               //debugFileItem("get_file_backup_status => foundItem", foundItem);
            }
            free(item);
         }
      }
      pthread_mutex_unlock(&gFileHandleAccessMtx);
   }
	return backup;
}

void set_file_cache_status(int idx, int status)
{
	if(pthread_mutex_lock(&gFileHandleAccessMtx) == 0)
	{
	   FileHandleTreeItem_s* item = malloc(sizeof(FileHandleTreeItem_s));

	   if(gFileHandleTree == NULL)
      {
	      gFileHandleTree = jsw_rbnew(fh_key_val_cmp, fh_key_val_dup, fh_key_val_rel);
      }

      if(item != NULL)
      {
         FileHandleTreeItem_s* foundItem = NULL;
         item->key = idx;
         foundItem = (FileHandleTreeItem_s*)jsw_rbfind(gFileHandleTree, item);
         if(foundItem == NULL)
         {
            item->value.fileHandle.cacheStatus   = status;

            item->value.fileHandle.backupCreated = 0;            // set to 0 by default
            item->value.fileHandle.permission    = PersistencePermission_LastEntry;
            item->value.fileHandle.userId        = 0;             // default value
            item->value.fileHandle.filePath      = NULL;

            memset(item->value.fileHandle.csumPath  , 0, PERS_ORG_MAX_LENGTH_PATH_FILENAME);
            item->value.fileHandle.csumPath[PERS_ORG_MAX_LENGTH_PATH_FILENAME-1] = '\0'; // Ensures 0-Termination
            memset(item->value.fileHandle.backupPath, 0, PERS_ORG_MAX_LENGTH_PATH_FILENAME);
            item->value.fileHandle.backupPath[PERS_ORG_MAX_LENGTH_PATH_FILENAME-1] = '\0'; // Ensures 0-Termination

            //debugFileItem("set_file_cache_status => insert => item", item);
            jsw_rbinsert(gFileHandleTree, item);
         }
         else
         {
            FileHandleTreeItem_s* newItem = malloc(sizeof(FileHandleTreeItem_s));
            if(newItem != NULL)
            {
               memcpy(newItem->value.payload , foundItem->value.payload, sizeof(FileHandleData_u) ); // duplicate value

               //debugFileItem("set_file_cache_status => erase => foundItem", foundItem);
               jsw_rberase(gFileHandleTree, foundItem);

               newItem->key = idx;
               newItem->value.fileHandle.cacheStatus = status;
               //debugFileItem("set_file_cache_status => insert => newItem", newItem);
               jsw_rbinsert(gFileHandleTree, newItem);
               free(newItem);
            }
         }
         free(item);
      }
		pthread_mutex_unlock(&gFileHandleAccessMtx);
	}
}

int get_file_cache_status(int idx)
{
	int status = -1;
	if(pthread_mutex_lock(&gFileHandleAccessMtx) == 0)
	{
      if(gFileHandleTree != NULL)
      {
         FileHandleTreeItem_s* item = malloc(sizeof(FileHandleTreeItem_s));
         if(item != NULL)
         {
            FileHandleTreeItem_s* foundItem = NULL;
            item->key = idx;
            foundItem = (FileHandleTreeItem_s*)jsw_rbfind(gFileHandleTree, item);
            if(foundItem != NULL)
            {
               status = foundItem->value.fileHandle.cacheStatus;
            }
            free(item);
         }
      }
		pthread_mutex_unlock(&gFileHandleAccessMtx);
	}
	return status;
}


void set_file_user_id(int idx, int userID)
{
   if(pthread_mutex_lock(&gFileHandleAccessMtx) == 0)
   {
      FileHandleTreeItem_s* item = malloc(sizeof(FileHandleTreeItem_s));
      if(gFileHandleTree == NULL)
      {
         gFileHandleTree = jsw_rbnew(fh_key_val_cmp, fh_key_val_dup, fh_key_val_rel);
      }

      if(item != NULL)
      {
         FileHandleTreeItem_s* foundItem = NULL;
         item->key = idx;
         foundItem = (FileHandleTreeItem_s*)jsw_rbfind(gFileHandleTree, item);
         if(foundItem == NULL)
         {
            item->value.fileHandle.userId        = userID;              // default value

            item->value.fileHandle.backupCreated = 0;                   // set to 0 by default
            item->value.fileHandle.permission    = -1;
            item->value.fileHandle.cacheStatus   = -1;                  // set to -1 by default
            item->value.fileHandle.filePath      = NULL;

            memset(item->value.fileHandle.csumPath  , 0, PERS_ORG_MAX_LENGTH_PATH_FILENAME);
            item->value.fileHandle.csumPath[PERS_ORG_MAX_LENGTH_PATH_FILENAME-1] = '\0'; // Ensures 0-Termination
            memset(item->value.fileHandle.backupPath, 0, PERS_ORG_MAX_LENGTH_PATH_FILENAME);
            item->value.fileHandle.backupPath[PERS_ORG_MAX_LENGTH_PATH_FILENAME-1] = '\0'; // Ensures 0-Termination

            //debugFileItem("set_file_user_id => insert", item);
            jsw_rbinsert(gFileHandleTree, item);
         }
         else
         {
            FileHandleTreeItem_s* newItem = malloc(sizeof(FileHandleTreeItem_s));
            if(newItem != NULL)
            {
               memcpy(newItem->value.payload , foundItem->value.payload, sizeof(FileHandleData_u) ); // duplicate value

               //debugFileItem("set_file_user_id => erase", foundItem);
               jsw_rberase(gFileHandleTree, foundItem);

               newItem->key = idx;
               newItem->value.fileHandle.userId = userID;
               //debugFileItem("set_file_user_id => insert", newItem);
               jsw_rbinsert(gFileHandleTree, newItem);

               free(newItem);
            }
         }
         free(item);
         free(foundItem);
      }

      pthread_mutex_unlock(&gFileHandleAccessMtx);
   }
}

int get_file_user_id(int idx)
{
   int id = -1;
   if(pthread_mutex_lock(&gFileHandleAccessMtx) == 0)
   {
      if(gFileHandleTree != NULL)
      {
         FileHandleTreeItem_s* item = malloc(sizeof(FileHandleTreeItem_s));
         if(item != NULL)
         {
            FileHandleTreeItem_s* foundItem = NULL;
            item->key = idx;
            foundItem = (FileHandleTreeItem_s*)jsw_rbfind(gFileHandleTree, item);
            if(foundItem != NULL)
            {
               id = foundItem->value.fileHandle.userId;
            }
            free(item);
         }
      }
      pthread_mutex_unlock(&gFileHandleAccessMtx);
   }
   return id;
}

//----------------------------------------------------------
//----------------------------------------------------------

int set_ossfile_handle_data(int idx, PersistencePermission_e permission, int backupCreated,
		                     const char* backup, const char* csumPath, char* filePath)
{
	int rval = 0;

	if(pthread_mutex_lock(&gOssFileHandleAccessMtx) == 0)
	{
	   FileHandleTreeItem_s* item = NULL;

      if(gOssFileHandleTree == NULL)
      {
         gOssFileHandleTree = jsw_rbnew(fh_key_val_cmp, fh_key_val_dup, fh_key_val_rel);
      }

      item = malloc(sizeof(FileHandleTreeItem_s));    // assign key and value to the rbtree item
      if(item != NULL)
      {
         FileHandleTreeItem_s* foundItem = NULL;
         item->key = idx;
         foundItem = (FileHandleTreeItem_s*)jsw_rbfind(gOssFileHandleTree, item);
         if(foundItem == NULL)
         {

            item->key = idx;

            item->value.fileHandle.permission    = permission;
            item->value.fileHandle.backupCreated = backupCreated;
            item->value.fileHandle.cacheStatus   = -1;            // set to -1 by default
            item->value.fileHandle.userId        = 0;             // default value
            item->value.fileHandle.filePath      = filePath;

            strncpy(item->value.fileHandle.backupPath, backup, PERS_ORG_MAX_LENGTH_PATH_FILENAME);
            item->value.fileHandle.backupPath[PERS_ORG_MAX_LENGTH_PATH_FILENAME-1] = '\0'; // Ensures 0-Termination
            strncpy(item->value.fileHandle.csumPath, csumPath, PERS_ORG_MAX_LENGTH_PATH_FILENAME);
            item->value.fileHandle.csumPath[PERS_ORG_MAX_LENGTH_PATH_FILENAME-1] = '\0'; // Ensures 0-Termination

            jsw_rbinsert(gOssFileHandleTree, item);

            free(item);
            rval = 0;
         }
         else
         {
            FileHandleTreeItem_s* newItem = malloc(sizeof(FileHandleTreeItem_s));
            if(newItem != NULL)
            {
               memcpy(newItem->value.payload , foundItem->value.payload, sizeof(FileHandleData_u) ); // duplicate value

               newItem->key = idx;

               newItem->value.fileHandle.permission    = permission;
               newItem->value.fileHandle.filePath      = filePath;

               strncpy(newItem->value.fileHandle.backupPath, backup, PERS_ORG_MAX_LENGTH_PATH_FILENAME);
               newItem->value.fileHandle.backupPath[PERS_ORG_MAX_LENGTH_PATH_FILENAME-1] = '\0'; // Ensures 0-Termination

               strncpy(newItem->value.fileHandle.csumPath, csumPath, PERS_ORG_MAX_LENGTH_PATH_FILENAME);
               newItem->value.fileHandle.csumPath[PERS_ORG_MAX_LENGTH_PATH_FILENAME-1] = '\0'; // Ensures 0-Termination

               //debugFileItem("set_file_backup_status => erase => foundItem", foundItem);
               jsw_rberase(gOssFileHandleTree, foundItem);

               //debugFileItem("set_file_backup_status => insert => newItem", newItem);
               jsw_rbinsert(gOssFileHandleTree, newItem);

               free(newItem);
            }

         }
      }
      else
      {

      }
		pthread_mutex_unlock(&gOssFileHandleAccessMtx);
	}

	return rval;
}


int get_ossfile_permission(int idx)
{
	int permission = (int)PersistencePermission_LastEntry;

	if(pthread_mutex_lock(&gOssFileHandleAccessMtx) == 0)
	{
	   if(gOssFileHandleTree != NULL)
      {
         FileHandleTreeItem_s* item = malloc(sizeof(FileHandleTreeItem_s));
         if(item != NULL)
         {
            FileHandleTreeItem_s* foundItem = NULL;
            item->key = idx;
            foundItem = (FileHandleTreeItem_s*)jsw_rbfind(gOssFileHandleTree, item);
            if(foundItem != NULL)
            {
               permission = foundItem->value.fileHandle.permission;
            }
            else
            {
               permission = -1;
            }
            free(item);
         }
      }
		pthread_mutex_unlock(&gOssFileHandleAccessMtx);
	}

	return permission;
}


char* get_ossfile_backup_path(int idx)
{
   char* charPtr = NULL;
   if(pthread_mutex_lock(&gOssFileHandleAccessMtx) == 0)
   {
      if(gOssFileHandleTree != NULL)
      {
         FileHandleTreeItem_s* item = malloc(sizeof(FileHandleTreeItem_s));
         if(item != NULL)
         {
            FileHandleTreeItem_s* foundItem = NULL;
            item->key = idx;
            foundItem = (FileHandleTreeItem_s*)jsw_rbfind(gOssFileHandleTree, item);
            if(foundItem != NULL)
            {
               charPtr = foundItem->value.fileHandle.backupPath;
            }
            free(item);
         }
      }
      pthread_mutex_unlock(&gOssFileHandleAccessMtx);
   }
	return charPtr;
}


char* get_ossfile_file_path(int idx)
{
   char* charPtr = NULL;
   if(pthread_mutex_lock(&gOssFileHandleAccessMtx) == 0)
   {
      if(gOssFileHandleTree != NULL)
      {
         FileHandleTreeItem_s* item = malloc(sizeof(FileHandleTreeItem_s));
         if(item != NULL)
         {
            FileHandleTreeItem_s* foundItem = NULL;
            item->key = idx;
            foundItem = (FileHandleTreeItem_s*)jsw_rbfind(gOssFileHandleTree, item);
            if(foundItem != NULL)
            {
               charPtr = foundItem->value.fileHandle.filePath;
            }
            free(item);
         }
      }
      pthread_mutex_unlock(&gOssFileHandleAccessMtx);
   }
	return charPtr;
}

void set_ossfile_file_path(int idx, char* file)
{
	if(pthread_mutex_lock(&gOssFileHandleAccessMtx) == 0)
	{
	   FileHandleTreeItem_s* item = malloc(sizeof(FileHandleTreeItem_s));

      if(gOssFileHandleTree == NULL)
      {
         gOssFileHandleTree = jsw_rbnew(fh_key_val_cmp, fh_key_val_dup, fh_key_val_rel);
      }

      if(item != NULL)
      {
         FileHandleTreeItem_s* foundItem = NULL;
         item->key = idx;
         foundItem = (FileHandleTreeItem_s*)jsw_rbfind(gOssFileHandleTree, item);
         if(foundItem == NULL)
         {
            item->value.fileHandle.filePath      = file;

            item->value.fileHandle.backupCreated = 0;             // set to 0 by default
            item->value.fileHandle.permission    = -1;
            item->value.fileHandle.cacheStatus   = -1;            // set to -1 by default
            item->value.fileHandle.userId        = 0;             // default value
            memset(item->value.fileHandle.csumPath  , 0, PERS_ORG_MAX_LENGTH_PATH_FILENAME);
            item->value.fileHandle.csumPath[PERS_ORG_MAX_LENGTH_PATH_FILENAME-1] = '\0'; // Ensures 0-Termination

            memset(item->value.fileHandle.backupPath, 0, PERS_ORG_MAX_LENGTH_PATH_FILENAME);
            item->value.fileHandle.backupPath[PERS_ORG_MAX_LENGTH_PATH_FILENAME-1] = '\0'; // Ensures 0-Termination

            jsw_rbinsert(gOssFileHandleTree, item);
         }
         else
         {
            FileHandleTreeItem_s* newItem = malloc(sizeof(FileHandleTreeItem_s));
            if(newItem != NULL)
            {
               memcpy(newItem->value.payload , foundItem->value.payload, sizeof(FileHandleData_u) ); // duplicate value

               jsw_rberase(gOssFileHandleTree, foundItem);

               newItem->key = idx;
               newItem->value.fileHandle.filePath = file;
               jsw_rbinsert(gOssFileHandleTree, newItem);

               free(newItem);
            }
         }
         free(item);
      }
		pthread_mutex_unlock(&gOssFileHandleAccessMtx);
	}
}


char* get_ossfile_checksum_path(int idx)
{
   char* charPtr = NULL;
   if(pthread_mutex_lock(&gOssFileHandleAccessMtx) == 0)
   {
      if(gOssFileHandleTree != NULL)
      {
         FileHandleTreeItem_s* item = malloc(sizeof(FileHandleTreeItem_s));
         if(item != NULL)
         {
            FileHandleTreeItem_s* foundItem = NULL;
            item->key = idx;
            foundItem = (FileHandleTreeItem_s*)jsw_rbfind(gOssFileHandleTree, item);
            if(foundItem != NULL)
            {
               charPtr = foundItem->value.fileHandle.csumPath;
            }
            free(item);
         }
      }
      pthread_mutex_unlock(&gOssFileHandleAccessMtx);
   }
	return charPtr;
}

#if 0
void set_ossfile_backup_status(int idx, int status)
{
	if(pthread_mutex_lock(&gOssFileHandleAccessMtx) == 0)
	{
	   FileHandleTreeItem_s* item = malloc(sizeof(FileHandleTreeItem_s));

      if(gOssFileHandleTree == NULL)
      {
         gOssFileHandleTree = jsw_rbnew(fh_key_val_cmp, fh_key_val_dup, fh_key_val_rel);
      }

      if(item != NULL)
      {
         FileHandleTreeItem_s* foundItem = NULL;
         item->key = idx;
         foundItem = (FileHandleTreeItem_s*)jsw_rbfind(gOssFileHandleTree, item);
         if(foundItem == NULL)
         {
            item->value.fileHandle.backupCreated = status;

            item->value.fileHandle.permission    = PersistencePermission_LastEntry;
            item->value.fileHandle.cacheStatus   = -1;            // set to -1 by default
            item->value.fileHandle.userId        = 0;             // default value
            item->value.fileHandle.filePath      = NULL;

            jsw_rbinsert(gOssFileHandleTree, item);
         }
         else
         {
            FileHandleTreeItem_s* newItem = malloc(sizeof(FileHandleTreeItem_s));
            if(newItem != NULL)
            {
               memcpy(newItem->value.payload , foundItem->value.payload, sizeof(FileHandleData_u) ); // duplicate value

               newItem->key = idx;
               newItem->value.fileHandle.backupCreated = status;

               jsw_rberase(gOssFileHandleTree, foundItem);

               jsw_rbinsert(gOssFileHandleTree, newItem);
               free(newItem);
            }
         }
         free(item);
      }
		pthread_mutex_unlock(&gOssFileHandleAccessMtx);
	}
}

int get_ossfile_backup_status(int idx)
{
   int rval = -1;

   if(pthread_mutex_lock(&gOssFileHandleAccessMtx) == 0)
   {
      if(gOssFileHandleTree != NULL)
      {
         FileHandleTreeItem_s* item = malloc(sizeof(FileHandleTreeItem_s));
         if(item != NULL)
         {
            FileHandleTreeItem_s* foundItem = NULL;
            item->key = idx;
            foundItem = (FileHandleTreeItem_s*)jsw_rbfind(gOssFileHandleTree, item);
            if(foundItem != NULL)
            {
               rval = foundItem->value.fileHandle.backupCreated;
            }
            free(item);
         }
      }
      pthread_mutex_unlock(&gOssFileHandleAccessMtx);
   }
	return rval;
}
#endif

int remove_ossfile_handle_data(int idx)
{
   int rval = -1;

   if(pthread_mutex_lock(&gOssFileHandleAccessMtx) == 0)
   {
      if(gOssFileHandleTree != NULL)
      {
         FileHandleTreeItem_s* item = malloc(sizeof(FileHandleTreeItem_s));    // assign key and value to the rbtree item
         if(item != NULL)
         {
            item->key = idx;
            rval = jsw_rberase(gOssFileHandleTree, item);
         }
      }

      pthread_mutex_unlock(&gOssFileHandleAccessMtx);
   }

   return rval;
}
