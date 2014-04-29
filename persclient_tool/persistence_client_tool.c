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
 * @file           persistence_client_tool.c
 * @ingroup        Persistence client library test tool
 * @author         Ingo Huerner
 * @brief          Persistence Client Library Test Tool
 * @see            
 */


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>


#define PCLT_VERSION "0.1"


void printHelp();



int main(int argc, char *argv[])
{
	int opt;

	while ((opt = getopt(argc, argv, "hv")) != -1)
	{
		switch (opt) {
	   	case 'h':
	   		printHelp();
	         break;
	   	case 'v':
	   		printf("Version: %s\n", PCLT_VERSION);
	         break;
	   	default: /* '?' */
	   		fprintf(stderr, "Usage: %s [-t nsecs] [-n] name\n", argv[0]);
	      	printHelp();
	         exit(EXIT_FAILURE);
	        }
	    }

   return 0;
}



void printHelp()
{
	printf("Usage: persclient_tool [-o <action to do>] [-a <application name>] [-r <resource id>] [-l <logical db id>]\n");
	printf("                       [-u <user no>] [-s <seat no>] [-f <file>] [-p <payload>] [-H] [-h] [-v]\n");

	printf("\n");
	printf("-o, --option=<action to do>   The possible actions are:\n");
	printf("                              readkey    - read out the given key\n");
	printf("                              writekey   - write something to a key\n");
	printf("                              deletekey  - delete a key\n");
	printf("                              getkeysize - get the size of a key\n");
	printf("-a, --appname=<application name>   The Application Name used for the initialization of the PCL\n");
	printf("-f, --file=<filename>              The file for data import or export\n");
	printf("-p, --payload=<payload>            The data to be written to a key if no file is specified as source.\n");
	printf("-r, --resource_id=<resource id>    The resource ID is the name of the key to process\n");
	printf("-l, --ldbid=<logical db id>        Logical Database ID in hex notation! e.g. 0xFF. If not specified the default value '0xFF' is used\n");
	printf("-u, --user_no=<user no>            The user number. If not specified the default value '0' is used\n");
	printf("-s, --seat_no=<seat no>            The seat number. If not specified the default value '0' is used\n");
	printf("-H, --forcehexdump                 Force print out a HexDump of the written/read data\n");
	printf("-h, --help                         Print help message\n");
	printf("-V, --version                      Print program version\n");

	printf("\n");
	printf("Examples:\n");
   printf("1.) Read a Key and show value as HexDump:\n");
	printf("    persclient_tool -o readkey -a MyApplication -r MyKey                   optional parameters: [-l 0xFF -u 0 -s 0]\n");
	printf("2.) Read a Key into a file:\n");
	printf("    persclient_tool -o readkey -a MyApplication -r MyKey -f outfile.bin    optional parameters: [-l 0xFF -u 0 -s 0 -H]\n");
	printf("3.) Write a Key and use the <payload> as the data source.:\n");
	printf("    persclient_tool -o writekey -a MyApplication -r MyKey -p 'Hello World' optional parameters: [-l 0xFF -u 0 -s 0 -H]\n");
	printf("4.) Write a Key and use a file as the data source.:\n");
	printf("    persclient_tool -o writekey -a MyApplication -r MyKey -f infile.bin    optional parameters: [-l 0xFF -u 0 -s 0 -H]\n");
	printf("5.) Get the size of a key [bytes]:\n");
	printf("    persclient_tool -o getkeysize -a MyApplication -r MyKey                optional parameters: [-l 0xFF -u 0 -s 0]\n");
	printf("6.) Delete a key:\n");
	printf("    persclient_tool -o deletekey -a MyApplication -r MyKey                 optional parameters: [-l 0xFF -u 0 -s 0]\n");
}

