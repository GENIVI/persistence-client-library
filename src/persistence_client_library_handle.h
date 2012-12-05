#ifndef PERSISTENCE_CLIENT_LIBRARY_HANDLE_H
#define PERSISTENCE_CLIENT_LIBRARY_HANDLE_H

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
 * @file           persistence_client_library_handle.h
 * @ingroup        Persistence client library
 * @author         Ingo Huerner
 * @brief          Header of the persistence client library handle.
 *                 Library provides an API to access persistent data
 * @see            
 */

#include "../include_protected/persistence_client_library.h"

/// handle structure definition
typedef struct _PersistenceHandle_s
{
   PersistenceInfo_s info;    /// persistence info
   char dbPath[DbPathMaxLen]; /// path to the database
   char dbKey[DbKeyMaxLen];   /// database key
}
PersistenceHandle_s;


/// persistence handle array
extern PersistenceHandle_s gHandleArray[MaxPersHandle];


/// open file descriptor handle array
extern int gOpenFdArray[MaxPersHandle];


/**
 * @brief get persistence handle
 *
 * @return a new handle or 0 if an error occured
 */
int get_persistence_handle_idx();


/**
 * @brief close persistence handle
 *
 * @param the handle to close
 */
void set_persistence_handle_close_idx(int handle);




#endif /* PERSISTENCY_CLIENT_LIBRARY_HANDLE_H */

