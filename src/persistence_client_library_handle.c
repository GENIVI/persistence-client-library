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

/// handle index
static int gHandleIdx = 1;

/// open file descriptor handle array
int gOpenFdArray[MaxPersHandle] = {0};

/// handle array
int gOpenHandleArray[MaxPersHandle] = {0};

/// persistence key handle array
PersistenceKeyHandle_s gKeyHandleArray[MaxPersHandle];

/// persistence file handle array
PersistenceFileHandle_s gFileHandleArray[MaxPersHandle];

/// persistence handle array for OSS and third party handles
PersistenceFileHandle_s gOssHandleArray[MaxPersHandle];


/// free handle array
int gFreeHandleArray[MaxPersHandle] = {0};
int gFreeHandleIdxHead = 0;
pthread_mutex_t gMtx = PTHREAD_MUTEX_INITIALIZER;



/// get persistence handle
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
            handle = -1;
            DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("get_persistence_handle_idx => Reached maximum of open handles: "), DLT_INT(MaxPersHandle));
         }
      }
      pthread_mutex_unlock(&gMtx);
   }

   return handle;
}


/// close persistence handle
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
      memset(gFreeHandleArray, 0, MaxPersHandle);
      memset(gOpenFdArray, 0, MaxPersHandle);
      memset(gOpenFdArray, 0, MaxPersHandle);

      // reset variables
      gHandleIdx = 1;
      gFreeHandleIdxHead = 0;

      pthread_mutex_unlock(&gMtx);
   }
}
