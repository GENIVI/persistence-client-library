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
   TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTIcacheON WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
******************************************************************************/
 /**
 * @file           persistence_client_library.c
 * @ingroup        Persistence client library
 * @author         Ingo Huerner
 * @brief          Implementation of the persistence client library.
 *                 Library provides an API to access persistent data
 * @see            
 */


#include "persistence_client_library.h"
#include "persistence_client_library_lc_interface.h"
#include "persistence_client_library_pas_interface.h"
#include "persistence_client_library_dbus_service.h"
#include "persistence_client_library_handle.h"
#include "persistence_client_library_data_access.h"
#include "persistence_client_library_custom_loader.h"

#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include <dlt/dlt.h>
#include <dlt/dlt_common.h>


/// debug log and trace (DLT) setup
DLT_DECLARE_CONTEXT(persClientLibCtx);


/// library constructor
void pers_library_init(void) __attribute__((constructor));

/// library deconstructor
void pers_library_destroy(void) __attribute__((destructor));



void pers_library_init(void)
{
   DLT_REGISTER_APP("Persistence Client Library","persClientLib");
   DLT_REGISTER_CONTEXT(persClientLibCtx,"persClientLib","Context for Logging");

   DLT_LOG(persClientLibCtx, DLT_LOG_ERROR, DLT_STRING("Initialize Persistence Client Library!!!!"));

   /// environment variable for on demand loading of custom libraries
   const char *pOnDemenaLoad = getenv("PERS_CUSTOM_LIB_LOAD_ON_DEMAND");

   /// environment variable for max key value data
   const char *pDataSize = getenv("PERS_MAX_KEY_VAL_DATA_SIZE");

   if(pDataSize != NULL)
   {
      gMaxKeyValDataSize = atoi(pDataSize);
   }

   // setup dbus main dispatching loop
   //setup_dbus_mainloop();

   // register for lifecycle and persistence admin service dbus messages
   //register_lifecycle();
   //register_pers_admin_service();

   // clear the open file descriptor array
   memset(gOpenFdArray, maxPersHandle, sizeof(int));

   /// get custom library names to load
   get_custom_libraries();

   if(pOnDemenaLoad == NULL)  // load all available libraries now
   {
      int i = 0;

      for(i=0; i < get_num_custom_client_libs(); i++ )
         load_custom_library(get_position_in_array(i), &gPersCustomFuncs[i] );

      /// just testing
      //gPersCustomFuncs[PersCustomLib_early].custom_plugin_open("Hallo", 88, 99);
      //gPersCustomFuncs[PersCustomLib_early].custom_plugin_close(17);
   }

   printf("A p p l i c a t i o n   n a m e : %s \n", program_invocation_short_name);   // TODO: only temp solution for application name
   strncpy(gAppId, program_invocation_short_name, maxAppNameLen);
}



void pers_library_destroy(void)
{
   int i = 0;
   GvdbTable* resourceTable = NULL;

   for(i=0; i< PersistenceRCT_LastEntry; i++)
   {
      resourceTable = get_resource_cfg_table_by_idx(i);

      // dereference opend database
      if(resourceTable != NULL)
      {
         gvdb_table_unref(resourceTable);
      }
   }

   // unregister for lifecycle and persistence admin service dbus messages
   //unregister_lifecycle();
   //unregister_pers_admin_service();


   DLT_UNREGISTER_CONTEXT(persClientLibCtx);
   DLT_UNREGISTER_APP();
   dlt_free();
}





