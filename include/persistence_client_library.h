#ifndef PERSISTENCY_CLIENT_LIBRARY_H
#define PERSISTENCY_CLIENT_LIBRARY_H

/******************************************************************************
 * Project         Persistency
 * (c) copyright   2011
 * Company         XS Embedded GmbH
 *****************************************************************************/
/******************************************************************************
 * This Source Code Form is subject to the terms of the
 * Mozilla Public License, v. 2.0. If a  copy of the MPL was not distributed
 * with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
******************************************************************************/
 /**
 * \file           persistence_client_library.h
 * \ingroup        Persistence client library
 * \author         Ingo Huerner (XSe) / Guy Sagnes (Continental)
 * \brief          Header of the persistence client library.
 *                 Library provides an API to access persistent data
 * \par change history
 * Date     Author          Version
 * 25/06/13 Ingo Hürner     1.0.0 - Rework of Init functions
 *
 */
/** \ingroup GEN_PERS */
/** \defgroup PERS_CLIENT Client: initialisation access
 *  \{
 */
/** \defgroup PERS_CLIENT_INTERFACE API document
 *  \{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** \defgroup PCL_DEFINES_API Defines, Struct, Enum
 * \{
 */

#define  PERSIST_API_INTERFACE_VERSION   (0x01020000U)

/** \} */


/** \defgroup PCL_OVERALL functions for Library Initialisation
 * The following functions have to be called to allow intialisation of the internal interfaces.
 * \{
 */


#define PCL_SHUTDOWN_TYPE_FAST   2      /// Client registered for fast lifecycle shutdown
#define PCL_SHUTDOWN_TYPE_NORMAL 1      /// Client registered for normal lifecycle shutdown


/**
 * @brief initalize client library
 *
 * @attention This function is currently  N O T  part of the GENIVI compliance specification
 *
 * @param appname application name, the name must be a unique name in the system
 * @param shutdownMode shutdown mode ::PCL_SHUTDOWN_TYPE_FAST or ::PCL_SHUTDOWN_TYPE_NORMAL
 *
 * @return positive value: success;
 *   On error a negative value will be returned with th follwoing error codes:
 *   ::EPERS_LOCKFS, ::EPERS_NOT_INITIALIZED, ::EPERS_INIT_DBUS_MAINLOOP,
 */
int pclInitLibrary(const char* appname, int shutdownMode);


/**
 * @brief deinitialize client library
 *
 * @attention This function is currently  N O T  part of the GENIVI compliance specification
 *
 * @return positive value: success;
 *   On error a negative value will be returned with th follwoing error codes: ::EPERS_LOCKFS
 */
int pclDeinitLibrary(void);

/** \} */

#ifdef __cplusplus
}
#endif

/** \} */ /* End of API */
/** \} */ /* End of MODULE */


#endif /* PERSISTENCY_CLIENT_LIBRARY_H */
