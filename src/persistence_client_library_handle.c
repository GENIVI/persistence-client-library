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

static int gInitialized = 0;

/// open file descriptor handle array
int gOpenFdArray[MaxPersHandle] = {0};

/// persistence key handle array
PersistenceKeyHandle_s gKeyHandleArray[MaxPersHandle];


/// persistence key handle array
PersistenceFileHandle_s gFileHandleArray[MaxPersHandle];


/// free handle array
int gFreeHandleArray[MaxPersHandle] = {0};

int gFreeHandleIdxHead = 0;

pthread_mutex_t gMtx;


/// get persistence handle
int get_persistence_handle_idx()
{
   int handle = 0;

   if(gInitialized == 0)
   {
      gInitialized = 1;
      pthread_mutex_init(&gMtx, 0);
   }

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

