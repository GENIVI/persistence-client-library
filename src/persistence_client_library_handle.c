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

#include <pthread.h>
#include <stdlib.h>
#include <string.h>


pthread_mutex_t gKeyHandleAccessMtx      = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t gFileHandleAccessMtx     = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t gOssFileHandleAccessMtx  = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t gMtx = PTHREAD_MUTEX_INITIALIZER;


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

// persistence key handle array
static PersistenceKeyHandle_s gKeyHandleArray[MaxPersHandle];
// persistence file handle array
static PersistenceFileHandle_s gFileHandleArray[MaxPersHandle];
// persistence handle array for OSS and third party handles
static PersistenceFileHandle_s gOssHandleArray[MaxPersHandle];



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
		if((idx < MaxPersHandle) && (0 < idx))
		{
			strncpy(gKeyHandleArray[idx].resource_id, id, DbResIDMaxLen);
			gKeyHandleArray[idx].resource_id[DbResIDMaxLen-1] = '\0'; // Ensures 0-Termination
			gKeyHandleArray[idx].ldbid   = ldbid;
			gKeyHandleArray[idx].user_no = user_no;
			gKeyHandleArray[idx].seat_no = seat_no;

			handle = idx;
		}
		else
		{
			DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("sKeyHdata - idx out of bounds:"), DLT_INT(idx));
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
		if((idx < MaxPersHandle) && (idx > 0))
		{
			strncpy(handleStruct->resource_id, gKeyHandleArray[idx].resource_id, DbResIDMaxLen);

			handleStruct->ldbid   = gKeyHandleArray[idx].ldbid;
			handleStruct->user_no = gKeyHandleArray[idx].user_no;
			handleStruct->seat_no = gKeyHandleArray[idx].seat_no;

			rval = 0;
		}

		pthread_mutex_unlock(&gKeyHandleAccessMtx);
	}

   return rval;
}


void init_key_handle_array()
{
	if(pthread_mutex_lock(&gKeyHandleAccessMtx) == 0)
	{
		memset(gKeyHandleArray, 0, MaxPersHandle * sizeof(PersistenceKeyHandle_s));

		pthread_mutex_unlock(&gKeyHandleAccessMtx);
	}
}


void clear_key_handle_array(int idx)
{
	if(pthread_mutex_lock(&gKeyHandleAccessMtx) == 0)
	{
      if(idx < MaxPersHandle && idx > 0 )
      {
         memset(&gKeyHandleArray[idx], 0, sizeof(gKeyHandleArray[idx]));
      }
		pthread_mutex_unlock(&gKeyHandleAccessMtx);
	}
}


int set_file_handle_data(int idx, PersistencePermission_e permission, const char* backup, const char* csumPath, char* filePath)
{
	int rval = 0;

	if(pthread_mutex_lock(&gFileHandleAccessMtx) == 0)
	{
		if(idx < MaxPersHandle && idx > 0 )
		{
			strcpy(gFileHandleArray[idx].backupPath, backup);
			strcpy(gFileHandleArray[idx].csumPath,   csumPath);
			gFileHandleArray[idx].backupCreated = 0;			// set to 0 by default
			gFileHandleArray[idx].permission = permission;
			gFileHandleArray[idx].filePath = filePath; 		// check to do if this works
			gFileHandleArray[idx].cacheStatus = -1; 			// set to -1 by default
			gFileHandleArray[idx].userId = 0;               // default value
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
		if(idx < MaxPersHandle && idx > 0 )
		{
			permission =  gFileHandleArray[idx].permission;
		}
		else
		{
			permission = -1;
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
      if(idx < MaxPersHandle && idx > 0 )
      {
         charPtr = gFileHandleArray[idx].backupPath;
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
      if(idx < MaxPersHandle && idx > 0 )
      {
         charPtr = gFileHandleArray[idx].csumPath;
      }
      pthread_mutex_unlock(&gFileHandleAccessMtx);
   }
	return charPtr;
}


void set_file_backup_status(int idx, int status)
{
	if(pthread_mutex_lock(&gFileHandleAccessMtx) == 0)
	{
      if(MaxPersHandle >= idx && idx > 0 )
      {
         gFileHandleArray[idx].backupCreated = status;
      }
		pthread_mutex_unlock(&gFileHandleAccessMtx);
	}
}

int get_file_backup_status(int idx)
{
   int backup = -1;
   if(pthread_mutex_lock(&gFileHandleAccessMtx) == 0)
   {
      if(MaxPersHandle >= idx && idx > 0 )
      {
         backup= gFileHandleArray[idx].backupCreated;
      }
      pthread_mutex_unlock(&gFileHandleAccessMtx);
   }
	return backup;
}

void set_file_cache_status(int idx, int status)
{
	if(pthread_mutex_lock(&gFileHandleAccessMtx) == 0)
	{
		gFileHandleArray[idx].cacheStatus = status;

		pthread_mutex_unlock(&gFileHandleAccessMtx);
	}
}

int get_file_cache_status(int idx)
{
	int status = -1;
	if(pthread_mutex_lock(&gFileHandleAccessMtx) == 0)
	{
		if(MaxPersHandle >= idx)
		{
			status = gFileHandleArray[idx].cacheStatus;
		}
		pthread_mutex_unlock(&gFileHandleAccessMtx);
	}
	return status;
}


void set_file_user_id(int idx, int userID)
{
   if(pthread_mutex_lock(&gFileHandleAccessMtx) == 0)
   {
      gFileHandleArray[idx].userId = userID;

      pthread_mutex_unlock(&gFileHandleAccessMtx);
   }
}

int get_file_user_id(int idx)
{
   int id = -1;
   if(pthread_mutex_lock(&gFileHandleAccessMtx) == 0)
   {
      if(MaxPersHandle >= idx)
      {
         id = gFileHandleArray[idx].userId;
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
		if(idx < MaxPersHandle && idx > 0 )
		{
			strcpy(gOssHandleArray[idx].backupPath, backup);
			strcpy(gOssHandleArray[idx].csumPath,   csumPath);
			gOssHandleArray[idx].backupCreated = backupCreated;
			gOssHandleArray[idx].permission = permission;
			gOssHandleArray[idx].filePath = filePath; // check to do if this works
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
		if(idx < MaxPersHandle && idx > 0 )
		{
			permission =  gOssHandleArray[idx].permission;
		}
		else
		{
			permission = -1;
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

      if(idx < MaxPersHandle && idx > 0 )
      {
         charPtr = gOssHandleArray[idx].backupPath;
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

      if(idx < MaxPersHandle && idx > 0 )
      {
         charPtr = gOssHandleArray[idx].filePath;
      }
      pthread_mutex_unlock(&gOssFileHandleAccessMtx);
   }
	return charPtr;
}

void set_ossfile_file_path(int idx, char* file)
{
	if(pthread_mutex_lock(&gOssFileHandleAccessMtx) == 0)
	{
      if(idx < MaxPersHandle && idx > 0 )
      {
         gOssHandleArray[idx].filePath = file;
      }
		pthread_mutex_unlock(&gOssFileHandleAccessMtx);
	}
}


char* get_ossfile_checksum_path(int idx)
{
   char* charPtr = NULL;
   if(pthread_mutex_lock(&gOssFileHandleAccessMtx) == 0)
   {
      if(idx < MaxPersHandle && idx > 0 )
      {
         charPtr = gOssHandleArray[idx].csumPath;
      }
      pthread_mutex_unlock(&gOssFileHandleAccessMtx);
   }
	return charPtr;
}


void set_ossfile_backup_status(int idx, int status)
{
	if(pthread_mutex_lock(&gOssFileHandleAccessMtx) == 0)
	{
      if(idx < MaxPersHandle && idx > 0 )
      {
         gOssHandleArray[idx].backupCreated = status;
      }
		pthread_mutex_unlock(&gOssFileHandleAccessMtx);
	}
}

int get_ossfile_backup_status(int idx)
{
   int rval = -1;

   if(pthread_mutex_lock(&gOssFileHandleAccessMtx) == 0)
   {
      if(idx < MaxPersHandle && idx > 0 )
      {
         rval = gOssHandleArray[idx].backupCreated;
      }
      pthread_mutex_unlock(&gOssFileHandleAccessMtx);
   }
	return rval;
}
