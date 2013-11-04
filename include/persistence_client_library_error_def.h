#ifndef PERSISTENCE_CLIENT_LIBRARY_ERROR_DEF_H
#define PERSISTENCE_CLIENT_LIBRARY_ERROR_DEF_H

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
 * @file           persistence_client_library_error_def.h
 * @ingroup        Persistence client library
 * @author         Ingo Huerner
 * @brief          Error definition header
 * @see
 */
/** \ingroup GEN_PERS */
/** \defgroup PERS_GEN_ERROR Client Library: Generic errors
 *  \{
 */

#ifdef __cplusplus
extern "C" {
#endif

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
/// internal database error
#define EPERS_DB_ERROR_INTERNAL  (-27)
/// db key size is to long
#define EPERS_DB_KEY_SIZE        (-28)
/// db value size is to long
#define EPERS_DB_VALUE_SIZE      (-29)
/// resource is not a key
#define EPERS_RES_NO_KEY         (-30)
/// change notification signal could ne be sent
#define EPERS_NOTIFY_SIG         (-31)
/// client library has not been initialized
#define EPERS_NOT_INITIALIZED 	(-32)
// max buffer size
#define EPERS_MAX_BUFF_SIZE      (-33)
// failed to setup dbus mainloop
#define EPERS_DBUS_MAINLOOP     (-34)
// failed register lifecycle dbus
#define EPERS_REGISTER_LIFECYCLE (-35)
// failed register admin service dbus
#define EPERS_REGISTER_ADMIN     (-36)
// registration on this key is not allowed
#define EPERS_NOTIFY_NOT_ALLOWED (-37)


#ifdef __cplusplus
}
#endif
/** \} */ /* End of PERS_GEN_ERROR */
#endif /* PERSISTENCE_CLIENT_LIBRARY_ERROR_DEF_H */
