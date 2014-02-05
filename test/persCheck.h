/******************************************************************************
 * Project         Persistency
 * (c) copyright   2014
 * Company         XS Embedded GmbH
 *****************************************************************************/
/******************************************************************************
 * This Source Code Form is subject to the terms of the
 * Mozilla Public License, v. 2.0. If a  copy of the MPL was not distributed
 * with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
******************************************************************************/
 /**
 * @file           persCheck.h
 * @ingroup        Persistence client library test
 * @author         awehrle
 * @brief          Test of persistence client library
 * @see
 */

#ifndef PERSCHECK_H_
#define PERSCHECK_H_

#include "pers_test_base.h"
#include <check.h>

#ifdef __cplusplus
extern "C" {
#endif

enum X_TEST_REPORTS{
   X_TEST_REPORTED_RESULT
};

int _optTestsReported;
char _optTestID[256];

#define X_TEST_INIT() do { \
   _optTestID[0] = '\0'; \
   } while(0)

#define ___FILE___ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#define REPORT_WARNINGS(){ \
   if(0==(_optTestsReported & (1 << X_TEST_REPORTED_RESULT))) \
      X_TEST_REPORT_RESULT(PASSED); \
}

/**
 * @brief: Report name of test. This has to be reported first.
 * MANDATORY
 */
#define X_TEST_REPORT_TEST_NAME(...) do { \
   char buf[sizeof(_optTestID)]; \
   snprintf(buf,sizeof(buf), __VA_ARGS__); \
   snprintf (_optTestID, sizeof(_optTestID),"%s::%s", ___FILE___, buf); \
   X_TEST_REPORT_TEST_ID(_optTestID); \
   X_TEST_REPORT_TEST_NAME_ID(_optTestID, __VA_ARGS__); \
   } while(0)

/**
 * @brief: Path to root of source code directory under test
 * MANDATORY
 */
#define X_TEST_REPORT_PATH(...) do { \
   X_TEST_REPORT_PATH_ID( _optTestID, __VA_ARGS__ ); \
   } while(0)

/**
 * @brief: Name of subcomponent under test, leave empty or set value NONE if not suitable for a COMPONENT test
 * MANDATORY
 */
#define X_TEST_REPORT_COMP_NAME(...) do { \
   X_TEST_REPORT_COMP_NAME_ID( _optTestID, __VA_ARGS__ ); \
   } while(0)

/**
 * @brief: Name of class or file under test, leave empty or set value NONE for a COMPONENT test
 * MANDATORY
 */
#define X_TEST_REPORT_FILE_NAME(...) do { \
   X_TEST_REPORT_FILE_NAME_ID( _optTestID, __VA_ARGS__ ); \
   } while(0)

/**
 * @brief: If information exists: Reference to a requirement, feature or bug ID. Else leave empty or set value NONE
 * MANDATORY
 */
#define X_TEST_REPORT_REFERENCE(...) do { \
   X_TEST_REPORT_REFERENCE_ID( _optTestID, __VA_ARGS__ ); \
   } while(0)

/**
 * @brief: A short description of test case.
 * Do not leave empty, can also be a internal department Test ID like CORE-OS-BOOT-0001
 * MANDATORY
 */
#define X_TEST_REPORT_DESCRIPTION(...) do { \
   X_TEST_REPORT_DESCRIPTION_ID( _optTestID, __VA_ARGS__ ); \
   } while(0)

/**
 * @brief: Reports weather this is a UNIT or a COMPONENT test
 * MANDATORY
 */
#define X_TEST_REPORT_KIND(kind) do { \
   X_TEST_REPORT_KIND_ID( _optTestID, kind ); \
   } while(0)

/**
 * @brief: valid values: PASSED, FAILED or NONE. PASSED if test result is ok, FAILED if test result is not as expected, NONE if no test exists for whole file or class
 * MANDATORY
 */
#define X_TEST_REPORT_RESULT(result) do { \
   X_TEST_REPORT_RESULT_ID( _optTestID, result); \
   _optTestsReported |= 1 << X_TEST_REPORTED_RESULT; \
   } while(0)

/**
 * @brief: Additional information, if test "just" checks common information flow inside structure (GOOD test case) or if structure is tested with invalid or border values(BORDER)
 * OPTIONAL
 */
#define X_TEST_REPORT_TYPE(type) do { \
   X_TEST_REPORT_TYPE_ID( _optTestID, type ); \
   } while(0)

#undef START_TEST
/* Start a unit test with START_TEST(unit_name), end with END_TEST
   One must use braces within a START_/END_ pair to declare new variables
*/
#define START_TEST(__testname)\
static void __testname (int _i CK_ATTRIBUTE_UNUSED)\
{\
   X_TEST_INIT(); \
   X_TEST_REPORT_TEST_NAME(""# __testname); \
   tcase_fn_start (""# __testname, __FILE__, __LINE__);

#define x_fail_unless(exp, ...){\
   int result = exp; \
   if(!result){ \
      X_TEST_REPORT_RESULT(FAILED); \
   } \
   fail_unless(exp, ##__VA_ARGS__); \
} while(0)

#define x_fail_if(exp, ...) {\
   int result = exp; \
   if(result){ \
      X_TEST_REPORT_RESULT(FAILED); \
   } \
   fail_if(exp, ##__VA_ARGS__); \
} while(0)

/* Always fail */
#define x_fail(...) {\
   X_TEST_REPORT_RESULT(FAILED); \
   fail(__VA_ARGS__); \
} while(0)

#undef END_TEST
/* End a unit test */
#define END_TEST {\
   REPORT_WARNINGS(); \
   _optTestsReported = 0; }\
}

#ifdef __cplusplus
}
#endif

#endif /* PERSCHECK_H_ */
