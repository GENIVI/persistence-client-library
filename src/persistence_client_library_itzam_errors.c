/*
    Itzam/C (version 6.0) is an embedded database engine written in Standard C.

    Copyright 2011 Scott Robert Ladd. All rights reserved.

    Older versions of Itzam/C are:
        Copyright 2002, 2004, 2006, 2008 Scott Robert Ladd. All rights reserved.

    Ancestral code, from Java and C++ books by the author, is:
        Copyright 1992, 1994, 1996, 2001 Scott Robert Ladd.  All rights reserved.

    Itzam/C is user-supported open source software. It's continued development is dependent on
    financial support from the community. You can provide funding by visiting the Itzam/C
    website at:

        http://www.coyotegulch.com

    You may license Itzam/C in one of two fashions:

    1) Simplified BSD License (FreeBSD License)

    Redistribution and use in source and binary forms, with or without modification, are
    permitted provided that the following conditions are met:

    1.  Redistributions of source code must retain the above copyright notice, this list of
        conditions and the following disclaimer.

    2.  Redistributions in binary form must reproduce the above copyright notice, this list
        of conditions and the following disclaimer in the documentation and/or other materials
        provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY SCOTT ROBERT LADD ``AS IS'' AND ANY EXPRESS OR IMPLIED
    WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
    FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SCOTT ROBERT LADD OR
    CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
    CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
    ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
    NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
    ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

    The views and conclusions contained in the software and documentation are those of the
    authors and should not be interpreted as representing official policies, either expressed
    or implied, of Scott Robert Ladd.

    2) Closed-Source Proprietary License

    If your project is a closed-source or proprietary project, the Simplified BSD License may
    not be appropriate or desirable. In such cases, contact the Itzam copyright holder to
    arrange your purchase of an appropriate license.

    The author can be contacted at:

          scott.ladd@coyotegulch.com
          scott.ladd@gmail.com
          http:www.coyotegulch.com
*/

/******************************************************************************
 * Project         Persistency
 * (c) copyright   2012
 * Company         XS Embedded GmbH
 *****************************************************************************/
/******************************************************************************
Small changes to use in persistence
******************************************************************************/
 /**
 * @file           persistence_client_itzam_errors.c
 * @ingroup        Persistence client library
 * @author         Ingo Huerner
 * @brief          Itzam database error definnitions
 * @see
 */

#include "persistence_client_library_itzam_errors.h"
#include "../include_protected/persistence_client_library_data_organization.h"


const char * ERROR_STRINGS [] =
{
    "invalid datafile signature",
    "invalid version",
    "can not open 64-bit datafile on 32-bit operating system",
    "write failed",
    "open failed",
    "read failed",
    "close failed",
    "seek failed",
    "tell failed",
    "duplicate remove",
    "flush failed",
    "rewrite record too small",
    "page not found",
    "lost key",
    "key not written",
    "key seek failed",
    "unable to remove key record",
    "record seek failed",
    "unable to remove data record",
    "list of deleted records could not be read",
    "list of deleted records could not be written",
    "iterator record count differs from database internal count",
    "rewrite over deleted record",
    "invalid column index",
    "invalid row index",
    "invalid hash value",
    "memory allocation failed",
    "attempt reading deleted record",
    "invalid record signature found",
    "invalid file locking mode",
    "unable to lock datafile",
    "unable to unlock datafile",
    "size mismatch when reading record",
    "attempt to start new transaction while one is already active",
    "no transaction active",
    "attempt to free a B-tree cursor when cursors were active",
    "invalid datafile object",
    "size_t is incompatible with Itzam",
    "could not create datafile",
    "global shared memory requires Administrator or user with SeCreateGlobalPrivilege",
    "cannot create global shared memory",
    "another process or thread has already created shared objects for this datafile",
    "invalid operation for read only file"
};

const char * STATE_MESSAGES [] =
{
    "okay",
    "operation failed",
    "version mismatch in files",
    "iterator at end",
    "iterator at beginning",
    "key not found",
    "duplicate key",
    "exceeded maximum file size on 32-bit system",
    "unable to write data record for index",
    "sizeof(size_t) smaller than required for file references; possibly 64-bit DB on 32-bit platform",
    "invalid operation for read only file"
};


void error_handler(const char * function_name, itzam_error error)
{
   DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("Itzam error in: "), DLT_STRING(function_name), DLT_STRING(ERROR_STRINGS[error]));
}
