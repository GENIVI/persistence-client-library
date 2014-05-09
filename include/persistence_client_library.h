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
 * 04/11/13 Ingo Hürner     1.3.0 - Added define for shutdown type none
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

#define  PERSIST_API_INTERFACE_VERSION   (0x01030000U)

/** \} */


/** \defgroup SHUTDOWN_TYPE notifications type definitions
 * according to Node State Manager Component
 * \{
 */

#define PCL_SHUTDOWN_TYPE_FAST   2     /// Client registered for fast lifecycle shutdown
#define PCL_SHUTDOWN_TYPE_NORMAL 1     /// Client registered for normal lifecycle shutdown
#define PCL_SHUTDOWN_TYPE_NONE   0     /// Client does not register to lifecycle shutdown
/** \} */


/** \defgroup PCL_OVERALL functions for Library initialization
 *  The following functions have to be called for library initialization
 * \{
 */


#define PCL_SHUTDOWN             1		/// trigger shutdown
#define PCL_SHUTDOWN_CANEL       0		/// cancel shutdown


/**
 * @brief initialize client library.
 *        This function will be called by the process using the PCL during startup phase.
 *
 * @attention This function is currently  N O T  part of the GENIVI compliance specification
 *
 * @param appname application name, the name must be a unique name in the system
 * @param shutdownMode shutdown mode ::PCL_SHUTDOWN_TYPE_FAST or ::PCL_SHUTDOWN_TYPE_NORMAL
 *
 * @return positive value: success;
 *   On error a negative value will be returned with the following error codes:
 *   ::EPERS_NOT_INITIALIZED, ::EPERS_INIT_DBUS_MAINLOOP,
 *   ::EPERS_REGISTER_LIFECYCLE, ::EPERS_REGISTER_ADMIN
 */
int pclInitLibrary(const char* appname, int shutdownMode);


/**
 * @brief deinitialize client library
 *        This function will be called during the shutdown phase of the process which uses the PCL.
 *
 * @attention This function is currently  N O T  part of the GENIVI compliance specification
 *
 * @return positive value: success;
 *   On error a negative value will be returned.
 */
int pclDeinitLibrary(void);




/**
 * @brief pclLifecycleSet client library
 *        This function can be called if to flush and write back the data form cache to memory device.
 *        The function is only available if PCL_SHUTDOWN_TYPE_NONE has been used in pclInitLibrary.
 *
 * @attention This function is currently  N O T  part of the GENIVI compliance specification
 * @attention In order to prevent misuse of this function the cancel shutdown request
 *            can only be called 3 times per lifecycle.
 *            If this function has been called by an application more then 3 times the application
 *            will not be able to store it's data anymore during the current lifecycle.
 *            The application isn't fully operable in this lifecycle anymore.
 *            In the next lifecycle the application can store data again until the limit above
 *            has been reached.
 *
 * @parm PCL_SHUTDOWN for write back data when shutdown is requested,
 *       and PCL_SHUTDOWN_CANEL when shutdown cancel request has been received.
 *
 * @return positive value: success;
 *   On error a negative value will be returned with the following error codes:
 *   ::EPERS_COMMON, :.EPERS_MAX_CANCEL_SHUTDOWN, ::EPERS_SHTDWN_NO_PERMIT
 */
int pclLifecycleSet(int shutdown);


/** \} */

#ifdef __cplusplus
}
#endif

/** \} */ /* End of API */
/** \} */ /* End of MODULE */


#endif /* PERSISTENCY_CLIENT_LIBRARY_H */
