# Persistence Client Library

## Copyright and License
Copyright (C) 2016 Mentor Graphics. 
This software is licensed under the Mozilla Public License 2.0, see COPYING file for more details.

## Project Scope 
The persistence client library has been developed as a proof of concept (PoC) for GENIVI.
It now serves as a reference implementation.

## Dependencies
The client library has the following dependencies
* build-time library dependencies;
  * automotive-dlt (http://projects.genivi.org/diagnostic-log-trace/)
  * Persistence Common Object (https://github.com/GENIVI/persistence-common-object.git)
    Add "--with_database=key-value-store" to the configure step
    The previous default backend Itzam/C and will not supported anymore.
  * dbus-1
  *  check unit test framework for C used when configured with "--enable-tests"
     Ubuntu: use Synaptic package manager
     Or download http://check.sourceforge.net/
* autotools
* libtool

## Building the PCL
The Persistence Client Lib component uses automake to build the library.
Execute the following steps in order to build the component
* autoreconf –vi
* configure, with the following options
  * --enable-tests to enable the build of the tests
  * --enable-pasinterface enable the PAS interface (disabled by default)
  * --enable-appcheck, performs an application check if the using application is a valid/trusted one.
* make 

## Testing and Debugging

Introduction:
--------------
The test framework “check” has been used to write unit tests which will be run automatically
when the test binary is started. At the end a test report will be printed to the console
showing first a summary about number of tests that have been executed and how many
tests have been passed or failed. After the summary a test report will be generated showing
the status of each test. When a bug will be fixed a test will be written to verify the problem
has been solved.
DTL (Diagnostic, Log and Trace) will be used by the PCL to send status and error. For
details about DLT, please refer to the GENIVI DLT p roject page
(http://projects.genivi.org/diagnostic-log-trace/). 

Persistence Common Object
-------------------------------
The Persistence Common Object (libpers_common.so) is the default plugin to read/write persistent data managed by the key-value API.

Attention:
The Persistence Common Object must be added to the plugin configuration file with the 
appropriate path to the library

Example: “default /usr/local/lib/libpers_common.so init sync”

Running tests:
--------------
There are unit tests available for the persistency client library component available.
The unit tests are used to verify that the component is working correctly and exclude any
regressions. Please refer always to the source codeto see the available tests

Preconditions:
--------------
* GENIVI Lifecycle components are available on thesystem
  * The GENIVI lifecycle components are available here: 
     http://projects.genivi.org/node-state-manager/
     http://projects.genivi.org/node-startup-controller/
  * Make sure Node State Manager is running (Start "node-state-manager")
* Persistence Administration Service is available in the system and running
  * The GENIVI Persistence Administration Service is available here:
   http://git.projects.genivi.org/?p=persistence/persistence-administrator.git
* Persistence Common Object is available in the system
  * The GENIVI Persistence Common Object is available here:
   http://git.projects.genivi.org/?p=persistence/persistence-common-object.git
   use the key-value-store backend (run configure step with "--with_database=key-value-store")
* Persistence partitions has been created and mounted and test data is
* If partition and data is not available, use the PAS to setup the test data.
  * make sure node-state-manager and pers_admin_svc are running
  * Now install the test data:
   "persadmin_tool install /path_to_test_data/PAS_data.tar.gz"
   The test data is provided by the PCL (../test/data/PAS_data.tar.gz)
* Make sure dlt-daemon is running
* Make sure D-Bus system bus is available 

Application verification
--------------
To check if an application is valid/trusted and allowed to access persistent data an application check 
functionality can be enabled (configure step --enable-appcheck, see section 6 “How to build”).
If an application is not valid/trusted every API call to the key-value or file API returns the 
error EPERS_SHUTDOWN_NO_TRUSTED will be returned.
A trusted/valid application has a corresponding resource configuration table installed by the 
Persistence Administration Service under /Data/mnt-c or /Data/mnt-wt.

Data location and partitions
--------------
Persistence expects its data under the two folders /Data/mnt-c and /Data/mnt-wt.
It is requested to have one persistence partition which will be mounted to the two folders stated above (one partition will be mounted to two different mointpoints).

Example:
sudo mount -t fsType /dev/sdx /Data/mnt-wt
sudo mount -t fsType /dev/sdx /Data/mnt-c

Run tests:
--------------
run persistency unit test "./persistence_client_library_test" 

Expected test result:
--------------
The expected result is to have 0 failures and 0 errors, see example output below:

./persistence_client_library_test
Running suite(s): Persistency client library
100%: Checks: 13, Failures: 0, Errors: 0
persistence_client_library_test.c:141:P:GetData:test_GetData:0: Passed
persistence_client_library_test.c:357:P:SetData:test_SetData:0: Passed
persistence_client_library_test.c:400:P:SetDataNoPRCT:test_SetDataNoPRCT:0: Passed
persistence_client_library_test.c:434:P:GetDataSize:test_GetDataSize:0: Passed
persistence_client_library_test.c:478:P:DeleteData:test_DeleteData:0: Passed
persistence_client_library_test.c:236:P:GetDataHandle:test_GetDataHandle:0: Passed
persistence_client_library_test.c:652:P:DataHandle:test_DataHandle:0: Passed
persistence_client_library_test.c:727:P:DataHandleOpen:test_DataHandleOpen:0: Passed
persistence_client_library_test.c:578:P:DataFile:test_DataFile:0: Passed
persistence_client_library_test.c:604:P:DataFileRecovery:test_DataFileRecovery:0: Passed
persistence_client_library_test.c:794:P:Cursor:test_Cursor:0: Passed
persistence_client_library_test.c:864:P:ReadDefault:test_ReadDefault:0: Passed
persistence_client_library_test.c:886:P:ReadConfDefault:test_ReadConfDefault:0: Passed

The output above may vary as the test cases will be adopted or extended. 

## Project page
- https://at.projects.genivi.org/wiki/display/PROJ/Persistence+Client+Library

## Mailing List
Subscribe to the mailing list here: http://lists.genivi.org/mailman/listinfo/genivi-persistence

## Bug reporting
View or Report bugs here: http://bugs.genivi.org/buglist.cgi?product=persistence-client-library

## Documentation
For details about the Persistence Client Library, please refer to to following documents:

- Persistence Client Library Architecture Documentation:
  see docs sufolder: Persistence_ArchitectureManual.pdf

- Persistence Client Library User Manual: 
  see docs subfolder: Persistence_ClientLibrary_UserGuide.pdf

## How to create doxygen documentation
The doxygen package must be installed.
Run "doxygen doc/pcl_doxyfile" and the html documentation will be generated in the doc folder.
