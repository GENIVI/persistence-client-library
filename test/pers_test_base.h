/******************************************************************************
 * Project         Persistency
 * (c) copyright   2013
 * Company         XS Embedded GmbH
 *****************************************************************************/
/******************************************************************************
 * This Source Code Form is subject to the terms of the
 * Mozilla Public License, v. 2.0. If a  copy of the MPL was not distributed
 * with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
******************************************************************************/
 /**
 * @file           pers_test_base.h
 * @ingroup        Persistence client library test
 * @author         awehrle
 * @brief          Test of persistence client library
 * @see
 */


#ifndef PERSBASETEST_H_
#define PERSBASETEST_H_

#include <stdbool.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAGIC_STRING "$$XS_TEST$$"

#define FAILED 0
#define PASSED 1
#define NONE 2

#define BORDER 0
#define GOOD 1

#define UNIT 0
#define COMPONENT 1

/**
* @brief: Report name of test. This has to be reported first.
* MANDATORY
*/
#define X_TEST_REPORT_TEST_ID(ID) do { \
printf (MAGIC_STRING"%s", ID); \
printf ("\n"); \
} while(0)

/**
* @brief: Report name of test. This has to be reported first.
* MANDATORY
*/
#define X_TEST_REPORT_TEST_NAME_ID(ID, ...) do { \
printf (MAGIC_STRING"%s$$", ID); \
printf ("testName:%s", __VA_ARGS__); \
printf ("\n"); \
} while(0)

/**
* @brief: Path to root of source code directory under test
* MANDATORY
*/
#define X_TEST_REPORT_PATH_ID(ID, ...) do { \
printf (MAGIC_STRING"%s$$", ID); \
printf ("path:"__VA_ARGS__); \
printf ("\n"); \
} while(0)

/**
* @brief: Name of subcomponent under test, leave empty or set value NONE if not suitable for a COMPONENT test
* MANDATORY
*/
#define X_TEST_REPORT_COMP_NAME_ID(ID, ...) do { \
printf (MAGIC_STRING"%s$$", ID); \
printf ("compName:"__VA_ARGS__); \
printf ("\n"); \
} while(0)

/**
* @brief: Name of class or file under test, leave empty or set value NONE for a COMPONENT test
* MANDATORY
*/
#define X_TEST_REPORT_FILE_NAME_ID(ID, ...) do { \
printf (MAGIC_STRING"%s$$", ID); \
printf ("fileName:"__VA_ARGS__); \
printf ("\n"); \
} while(0)

/**
* @brief: If information exists: Reference to a requirement, feature or bug ID. Else leave empty or set value NONE
* MANDATORY
*/
#define X_TEST_REPORT_REFERENCE_ID(ID, ...) do { \
printf (MAGIC_STRING"%s$$", ID); \
printf ("ref:"__VA_ARGS__); \
printf ("\n"); \
} while(0)

/**
* @brief: A short description of test case.
* Do not leave empty, can also be a internal department Test ID like CORE-OS-BOOT-0001
* MANDATORY
*/
#define X_TEST_REPORT_DESCRIPTION_ID(ID, ...) do { \
printf (MAGIC_STRING"%s$$", ID); \
printf ("desc:"__VA_ARGS__); \
printf ("\n"); \
} while(0)

/**
* @brief: Reports weather this is a UNIT or a COMPONENT test
* MANDATORY
*/
#define X_TEST_REPORT_KIND_ID(ID, kind) do { \
printf (MAGIC_STRING"%s$$kind:%s\n", ID, kind==UNIT?"UNIT":kind==COMPONENT?"COMPONENT":"NONE"); \
} while(0)

/**
* @brief: valid values: PASSED, FAILED or NONE. PASSED if test result is ok, FAILED if test result is not as expected, NONE if no test exists for whole file or class
* MANDATORY
*/
#define X_TEST_REPORT_RESULT_ID(ID, result) do { \
printf (MAGIC_STRING"%s$$result:%s\n", ID, result==PASSED?"PASSED":result==FAILED?"FAILED":"NONE"); \
} while(0)

/**
* @brief: Additional information, if test "just" checks common information flow inside structure (GOOD test case) or if structure is tested with invalid or border values(BORDER)
* OPTIONAL
*/
#define X_TEST_REPORT_TYPE_ID(ID, type) do { \
printf (MAGIC_STRING"%s$$type:%s\n", ID, type==BORDER?"BORDER":"GOOD"); \
} while(0)

#ifdef __cplusplus
}
#endif

#endif /* PERSBASETEST_H_ */
