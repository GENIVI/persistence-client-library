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
 * @file           persistence_client_library_dbus_test.c
 * @ingroup        Persistence client library test
 * @author         Ingo Huerner
 * @brief          Test of persistence client library
 * @see            
 */

#include "../include/persistence_client_library_key.h"
#include "../include/persistence_client_library_file.h"
#include "../include/persistence_client_library_error_def.h"

#include <stdio.h>


int main(int argc, char *argv[])
{
   int ret = 0;
   unsigned char buffer[1024];
   printf("Dbus interface test application\n");

   getchar();
   ret = pclKeyReadData(0xFF, "pos/last_position",         0, 0, buffer, 1024);
   getchar();


   printf("By\n");
   return ret;
}
