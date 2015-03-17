#ifndef PERSISTENCE_CLIENT_LIBRARY_TREE_HELPER_H
#define PERSISTENCE_CLIENT_LIBRARY_TREE_HELPER_H

/******************************************************************************
 * Project         Persistency
 * (c) copyright   2015
 * Company         XS Embedded GmbH
 *****************************************************************************/
/******************************************************************************
 * This Source Code Form is subject to the terms of the
 * Mozilla Public License, v. 2.0. If a  copy of the MPL was not distributed
 * with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
******************************************************************************/
 /**
 * @file           persistence_client_library_tree_helper.h
 * @ingroup        Persistence client library
 * @author         Ingo Huerner
 * @brief          Header of persistence client library tree helper functions
 * @see
 */


#include<stdio.h>
#include<stdlib.h>

#include "persistence_client_library_handle.h"
#include "rbtree.h"



/// key handle data union definition
typedef union KeyHandleData_u_
{
   /// key handle data definition
   PersistenceKeyHandle_s keyHandle;

   /// key handle data payload
   char payload[sizeof(struct _PersistenceKeyHandle_s)];

} KeyHandleData_u;

/// type definition of a key tree item
typedef struct _KeyHandleTreeItem_s
{
   /// key handle key
   int key;

   /// key handle data
   KeyHandleData_u value;

} KeyHandleTreeItem_s;



/// file handle data union definition
typedef union FileHandleData_u_
{
   /// file handle data definition
   PersistenceFileHandle_s fileHandle;

   /// file handle data payload
   char payload[sizeof(struct _PersistenceFileHandle_s)];

} FileHandleData_u;

/// type definition of a file tree item
typedef struct _FileHandleTreeItem_s
{
   /// file handle key
   int key;

   /// file handle data
   FileHandleData_u value;

} FileHandleTreeItem_s;


/// structure definition for a key value item
typedef struct _key_value_s
{
   unsigned int key;
   char*        value;
}key_value_s;



/**
 * @brief Compare function for file handle tree item
 *
 * @param p1 pointer to the first item to compare
 * @param p2 pointer to the second item to compare
 *
 * @return 0 if key is equal; -1 if p1 < p2; 1 if p1 > p2
 */
int fh_key_val_cmp(const void *p1, const void *p2 );


/**
 * @brief Duplicate function for file handle tree item
 *
 * @param p the pointer of the item to duplicate
 *
 * @return pointer to the duplicated item, on failure NULL
 */
void* fh_key_val_dup(void *p);

/**
 * @brief Release function for file handle tree item
 *
 * @param p pointer tp the item to release
 */
void  fh_key_val_rel(void *p );



/**
 * @brief Compare function for key handle tree item
 *
 * @param p1 pointer to the first item to compare
 * @param p2 pointer to the second item to compare
 *
 * @return 0 if key is equal; -1 if p1 < p2; 1 if p1 > p2
 */
int kh_key_val_cmp(const void *p1, const void *p2 );

/**
 * @brief Duplicate function for key handle tree item
 *
 * @param p the pointer of the item to duplicate
 *
 * @return pointer to the duplicated item, on failure NULL
 */
void* kh_key_val_dup(void *p);

/**
 * @brief Release function for key handle tree item
 *
 * @param p pointer tp the item to release
 */
void  kh_key_val_rel(void *p );


/**
 * @brief Compare function for key tree item
 *
 * @param p1 pointer to the first item to compare
 * @param p2 pointer to the second item to compare
 *
 * @return 0 if key is equal; -1 if p1 < p2; 1 if p1 > p2
 */
int key_val_cmp(const void *p1, const void *p2 );


/**
 * @brief Duplicate function for key tree item
 *
 * @param p the pointer of the item to duplicate
 *
 * @return pointer to the duplicated item, on failure NULL
 */
void* key_val_dup(void *p);


/**
 * @brief Release function for key tree item
 *
 * @param p pointer tp the item to release
 */
void  key_val_rel(void *p);


#if 0
/**
 * @brief Release function for key tree item
 *
 * @param prefix a string prefix to identify the item
 * @param item the item to debug
 */
void debugFileItem(const char* prefix, FileHandleTreeItem_s* item);
#endif

#endif /* PERSISTENCE_CLIENT_LIBRARY_TREE_HELPER_H */
