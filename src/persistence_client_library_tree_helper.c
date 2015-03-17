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
 * @file           persistence_client_library_tree_helper.c
 * @ingroup        Persistence client library
 * @author         Ingo Huerner
 * @brief          Implementation of persistence client library tree helper functions
 * @see
 */

#include "persistence_client_library_tree_helper.h"


#if 0
void debugFileItem(const char* prefix, FileHandleTreeItem_s* item)
{
   printf("-----------------------------------------\n");
   printf("   prefix        : %s\n", prefix);
   printf("   key           : %d\n", item->key );
   printf("   backupCreated : %d\n", item->value.fileHandle.backupCreated );
   printf("   backupPath    : %s\n", item->value.fileHandle.backupPath );
   printf("   csumPath      : %s\n", item->value.fileHandle.csumPath );
   printf("   filePath      : %s\n", item->value.fileHandle.filePath );
   printf("   cacheStatus   : %d\n", item->value.fileHandle.cacheStatus );
   printf("   permission    : %d\n", item->value.fileHandle.permission );
   printf("   userId        : %d\n", item->value.fileHandle.userId );
   printf("-----------------------------------------\n");
}
#endif
/**
 * File handle helper functions
 */

// compare function for tree item
int fh_key_val_cmp(const void *p1, const void *p2)
{
   int rval = -1;

   FileHandleTreeItem_s* first;
   FileHandleTreeItem_s* second;

   first  = (FileHandleTreeItem_s*)p1;
   second = (FileHandleTreeItem_s*)p2;

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



/// duplicate function for tree item
void* fh_key_val_dup(void *p)
{
   FileHandleTreeItem_s* dst = NULL;
   FileHandleTreeItem_s* src = (FileHandleTreeItem_s*)p;

   // allocate memory for node
   dst = malloc(sizeof(FileHandleTreeItem_s));

   if(dst != NULL)
   {
     dst->key = src->key;               // duplicate hash key

     memcpy(dst->value.payload , src->value.payload, sizeof(FileHandleData_u) ); // duplicate value
   }

   return dst;
}

/// release function for tree item
void  fh_key_val_rel(void *p)
{
   FileHandleTreeItem_s* rel = (FileHandleTreeItem_s*)p;

   if(rel != NULL)
      free(rel);
}




/**
 * Key handle helper functions
 */

/// compare function for tree item
int kh_key_val_cmp(const void *p1, const void *p2)
{
   int rval = -1;

   KeyHandleTreeItem_s* first;
   KeyHandleTreeItem_s* second;

   first  = (KeyHandleTreeItem_s*)p1;
   second = (KeyHandleTreeItem_s*)p2;

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



/// duplicate function for tree item
void* kh_key_val_dup(void *p)
{
   KeyHandleTreeItem_s* dst = NULL;
   KeyHandleTreeItem_s* src = (KeyHandleTreeItem_s*)p;

   // allocate memory for node
   dst = malloc(sizeof(KeyHandleTreeItem_s));

   if(dst != NULL)
   {
     dst->key = src->key;               // duplicate hash key

     memcpy(dst->value.payload , src->value.payload, sizeof(KeyHandleData_u) ); // duplicate value
   }
   return dst;
}



/// release function for tree item
void  kh_key_val_rel(void *p)
{
   KeyHandleTreeItem_s* rel = (KeyHandleTreeItem_s*)p;

   if(rel != NULL)
      free(rel);
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
     dst->key = src->key;               // duplicate hash key

     dst->value = malloc(value_size);  // duplicate value
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

   if(rel != NULL)
   {
      if(rel->value != NULL)
         free(rel->value);

      free(rel);
   }
}



