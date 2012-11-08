/******************************************************************************
 * Project         Persistency
 * (c) copyright   2012
 * Company         XS Embedded GmbH
 *****************************************************************************/
/******************************************************************************
   Permission is hereby granted, free of charge, to any person obtaining
   a copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included
   in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
   DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
   TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
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

#include <stdlib.h>

/// handle index
static int gHandleIdx = 1;

static int gInitialized = 0;

/// open file descriptor handle array
int gOpenFdArray[maxPersHandle];

/// persistence handle array
PersistenceHandle_s gHandleArray[maxPersHandle];
/// free handle array
int gFreeHandleArray[maxPersHandle];

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
         if(gHandleIdx < maxPersHandle-1)
         {
            handle = gHandleIdx++;  // no free spot before current max, increment handle index
         }
         else
         {
            handle = -1;
            printf("get_persistence_handle_idx => Reached maximum of open handles: %d \n", maxPersHandle);
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
      if(gFreeHandleIdxHead < maxPersHandle)
      {
         gFreeHandleArray[gFreeHandleIdxHead++] = handle;
      }
      pthread_mutex_unlock(&gMtx);
   }
}

