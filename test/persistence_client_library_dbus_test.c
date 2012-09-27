/*
 * persistence_client_library_dbus_test.c
 *
 *  Created on: Aug 8, 2012
 *      Author: ihuerner
 */



#include "../include/persistence_client_library_key.h"
#include "../include/persistence_client_library_file.h"


#include <stdio.h>


int main(int argc, char *argv[])
{
   int ret = 0;
   char buffer[128];

   printf("Dbus interface test application\n");

   ret = key_read_data(0,    "/language/current_language", 3, 0, (unsigned char*)buffer, 128);

   getchar();


   printf("By\n");
   return ret;
}
