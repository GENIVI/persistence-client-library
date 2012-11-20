#ifndef PERSISTENCE_CLIENT_LIBRARY_ERROR_DEF_H
#define PERSISTENCE_CLIENT_LIBRARY_ERROR_DEF_H

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
   TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
******************************************************************************/
 /**
 * @file           persistence_client_library_error_def.h
 * @ingroup        Persistence client library
 * @author         Ingo Huerner
 * @brief          Error definition header
 * @see
 */

// common error, for this error errno will be set
#define EPERS_COMMON             (-1)
/// file system is locked
#define EPERS_LOCKFS             (-2)
/// filesystem is currently locked
#define EPERS_MAP_LOCKFS         ((void *) -2)
/// bad storage policy
#define EPERS_BADPOL             (-3)
/// open handle limit reached
#define EPERS_MAXHANDLE          (-4)
/// max buffer limit for persistence data
#define EPERS_BUFLIMIT           (-5)
/// persistence resource configuration table not found
#define EPERS_NOPRCTABLE         (-6)
/// key not found
#define EPERS_NOKEY              (-7)
/// no data for key
#define EPERS_NOKEYDATA          (-8)
/// write of data failed
#define EPERS_SETDTAFAILED       (-9)
/// failed to open file
#define EPERS_OPENFILE           (-10)
/// invalid buffer or key
#define EPERS_DESER_BUFORKEY     (-11)
/// can't allocat memory for deserialization of keyvalue
#define EPERS_DESER_ALLOCMEM     (-12)
/// no ploicy avaliable in data to serialize
#define EPERS_DESER_POLICY       (-13)
/// no store type avaliable in data to serialize
#define EPERS_DESER_STORE        (-14)
/// no permission avaliable in data to serialize
#define EPERS_DESER_PERM         (-15)
/// no max size avaliable in data to serialize
#define EPERS_DESER_MAXSIZE      (-16)
/// no responsibility avaliable in data to serialize
#define EPERS_DESER_RESP         (-17)
/// out of array bounds
#define EPERS_OUTOFBOUNDS        (-18)
/// failed to map config file
#define EPERS_CONFIGMAPFAILED    (-19)
/// config file if not available
#define EPERS_CONFIGNOTAVAILABLE (-20)
/// can't stat config file
#define EPERS_CONFIGNOSTAT       (-21)
/// plugin functin not found
#define EPERS_NOPLUGINFCNT       (-22)
/// dlopen error
#define EPERS_DLOPENERROR        (-23)
/// plugin function not loaded
#define EPERS_NOPLUGINFUNCT      (-24)
/// file remove error
#define EPERS_FILEREMOVE         (-25)
/// err code to signalize last entry in DB
#define EPERS_LAST_ENTRY_IN_DB   (-26)


/**
 * @brief Main dispatching loop
 *
 * @return 0
 */
void* dbus_main_dispatching_loop(void* dataPtr);


#endif /* PERSISTENCE_CLIENT_LIBRARY_ERROR_DEF_H */
