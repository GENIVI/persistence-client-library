#ifndef PERSISTENCE_CLIENT_LIBRARY_HANDLE_H
#define PERSISTENCE_CLIENT_LIBRARY_HANDLE_H

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
 * @file           persistence_client_library_handle.h
 * @ingroup        Persistence client library
 * @author         Ingo Huerner
 * @brief          Header of the persistence client library handle.
 *                 Library provides an API to access persistent data
 * @see            
 */

#include "persistence_client_library.h"

typedef struct _PersistenceHandle_s
{
   int shared_DB;             /// is a shared resource
   char dbPath[dbPathMaxLen]; /// path to the database
   char dbKey[dbKeyMaxLen];   /// database key
}
PersistenceHandle_s;

/// persistence handle array
static PersistenceHandle_s gHandleArray[maxPersHandle];


/// open file descriptor handle array
static int gOpenFdArray[maxPersHandle];


/// get persistence handle
int get_persistence_handle_idx();

/// close persistence handle
void set_persistence_handle_close_idx(int handle);


#endif /* PERSISTENCY_CLIENT_LIBRARY_HANDLE_H */

