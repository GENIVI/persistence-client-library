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
 * 25/06/13 Ingo Huerner    1.0.0 - Rework of Init functions
 * 04/11/13 Ingo Huerner    1.3.0 - Added define for shutdown type none
 * 12/11/14 Guy Sagnes      1.4.0 - Add defines for accessing:
 *                                   - node data, configurable default
 *                                   - specific logical database (local, shared public)
 * 12/12/14 Petrica Manoila 1.4.1 - Additional error code ::EPERS_INVALID_ARGUMENT returned by pclInitLibrary()
 * 10/04/14 Guy Sagnes      1.4.2 - Add range definition for the user id definition
 *
 */
/** \ingroup GEN_PERS */
/** \defgroup PERS_CLIENT Client: initialization access
 *  \{
 */
/** \defgroup PERS_CLIENT_INTERFACE API document
 *  \{
 */

/**
 * @mainpage The Persistence Client Library documentation
 *           GENIVI's Persistence Client Library, known as PCL, is responsible for handling persistent data,
 *           including all data read and modified during a lifetime of an infotainment system.<br>
 *           "Persistent data" is data stored in a non-volatile storage such as a hard disk drive or FLASH memory.
 *           <br><br>
 *           The Persistence Client Library (PCL) provides an API to applications to read and write persistent data
 *           via a key-value and a file interface.<br>
 *           It also provide a plugin API to allow users to extend the client library with custom storage solutions.
 *
 * @section doc Further documentation
 * @subsection docUser User Manual
 *          There is a a user manual for the client library available,
 *          see: http://docs.projects.genivi.org/persistence-client-library/1.0/GENIVI_Persistence_Client_Library_UserManual.pdf
 *
 * @subsection docArch Architecture Manual
 *          For more information about the persistence architecture,
 *          see: http://docs.projects.genivi.org/persistence-client-library/1.0/GENIVI_Persistence_ArchitectureDocumentation.pdf
 *
 *
 * @section plugin Custom plugin configuration file
 *          @attention
 *          The plugin configuration file has been changed!
 *
 *          The configuration file has now the following format<br>
 *          &lt;predefinedPluginName&gt; &lt;pathAndLibraryName&gt; &lt;loadingType&gt; &lt;initType&gt;
 *
 *          <b>Predefined plugin name</b><br>
 *          Use one of the following names:
 *          - early     => predefined custom library for early persistence
 *          - secure    => predefined custom library for secure persistence
 *          - emergency => predefined custom library for emengency persistence
 *          - hwinfo    => predefined custom library for hw information
 *          - custom1   => custom library 1
 *          - custom2   => custom library 2
 *          - custom3   => custom library 3
 *
 *          <b>Path and library name:</b><br>
 *          The name and path of the library
 *
 *          <b>Valid loading type:</b>
 *          - "init" ==> if the plugin must be laoding during the pclInitLibrary function
 *          - "on"	==> if on demand laoding of the plugin is requested. The plugin will be loaded
 *          when a plugin function will be loaded the first time
 *
 *          <b>Init Type:</b>
 *          - sync ==> use the "plugin_init" function for plugin initialization
 *          - async ==> use the "plugin_init_async" function for plugin initialization
 *
 *          <b>Example:</b><br>
 *          hwinfo /usr/local/lib/libhwinfoperscustom.so init async
 *
 *          @note
 *          Make sure that there is only ONE blank between each entry and no blank at the end of the file.
 *          The parser has been optimized for speed, so there is less error checking.
 */



#ifdef __cplusplus
extern "C" {
#endif

/** \defgroup PCL_VERSION_OVERALL library initialization IF Version
 *  \{
 */
/** Module version :  &lt;genivi_major_version&gt;.&lt;genivi_minor_version&gt;.&lt;patch_version&gt;
 *  The most significant 2 bytes indicate the original Genivi library initialization IF version (major and minor version).
 *  The rest of the bytes indicate the patch version created over the original Genivi version.
*/
#define  PERSIST_API_INTERFACE_VERSION   (0x01040200U) /* 1.3.3 over 1.4.0 */

/** \} */


/** \defgroup SHUTDOWN_TYPE notifications type definitions
 * according to Node State Manager Component
 * \{
 */


/**
 * @brief PCL registered for fast lifecycle shutdown
 *        A lifecycle register message will be sent to the Node State Manager (NSM).
 *        When the system performs a fast shutdown the PCL receives this shutdown notification
 *        from the NSM, and will write back changed data to non volatile memory device.
 */
#define PCL_SHUTDOWN_TYPE_FAST   2

/**
 * @brief PCL registered for normal lifecycle shutdown
 *        A lifecycle register message will be sent to the Node State Manager (NSM).
 *        When the system performs a normal shutdown the PCL receives this shutdown notification
 *        from the NSM, and will write back changed data to non volatile memory device.
 */
#define PCL_SHUTDOWN_TYPE_NORMAL 1

/**
 * @brief PCL does NOT register to lifecycle shutdown
 *        The PCL does NOT receive any shutdown notification form the Node State Manager (NSM).
 *        The application itself is responsible to get shutdown information.
 *        To write back changed data to non volatile memory devicem, the application can use the
 *        function ::pclLifecycleSet with the parameter ::PCL_SHUTDOWN.
 *        PCL writes back the data, and blocks any further access to persistent data.
 *        In order to access data again (e.g. using ::pclKeyWriteData) the application
 *        needs to call ::pclLifecycleSet again with the parameter ::PCL_SHUTDOWN_CANCEL.
 *        There is a limitation for calling ::pclLifecycleSet with the parameter ::PCL_SHUTDOWN_CANCEL.
 *        It can be called only for a limited number, which is defined in ::Shutdown_MaxCount.
 *
 * @attention PCL does not receive any shutdown notifications from NSM.
 */
#define PCL_SHUTDOWN_TYPE_NONE   0
/** \} */


/** \defgroup PCL_USERDEF specific defines for user parameters 
 * The valid user value range:
 *  - 0: NODE data access
 *  - 1..max value: valid range for dedicated user
 *  - -1: user to access the configurable default value
 *
 * The following allows specific access to the data
 * \{
 */

#define PCL_USER_NODEDATA        0               /*!< user value to access the node value (user independent) */
#define PCL_USER_DEFAULTDATA     0xFFFFFFFF      /*!< user value to access directly the configurable default value */
#define PCL_USER_MAXUSERID       0xEFFFFFFF      /*!< range definition of the user, this is the last supported user */

/** \} */

/** \defgroup PCL_DBDEF specific defines for logical database parameters 
 * The valid user value range:
 *  - 0x00: shared public data
 *  - 0xFF: local data
 *  - 0x01..0x7F: shared group
 *
 * The following allows specific access to the data
 * \{
 */

#define PCL_LDBID_LOCAL        0xFF     /*!< value to access application local managed data */
#define PCL_LDBID_PUBLIC       0x00     /*!< value to access public shared data, data is under control by the resource configuration in system */

/** \} */

/** \defgroup SHUTDOWN_EVENTS shutdown events definitions
 * \{
 */

/**
 *
 * @brief   Shutdown event.
 *          Used with the pclLifecycleSet() function to inform PCL about the shutdown lifecycle state.
 */
#define PCL_SHUTDOWN              1


/**
 *
 * @brief   Cancel shutdown event.
 *          Used with the pclLifecycleSet() function to inform PCL about the cancel shutdown lifecycle state.
 */
#define PCL_SHUTDOWN_CANCEL       0

/** \} */

/** \defgroup PCL_OVERALL functions for Library initialization
 * The following functions have to be called for library initialization.
 * \{
 */

/**
 * @brief initialize client library.
 *        This function will be called by the process using the PCL during startup phase.
 *        The function will be called within process using always the same appname!
 *        It is not allowed call this function within a process using different appnames!
 *
 * @param appname application name, the name must be a unique name in the system
 * @param shutdownMode shutdown mode ::PCL_SHUTDOWN_TYPE_FAST or ::PCL_SHUTDOWN_TYPE_NORMAL ::PCL_SHUTDOWN_TYPE_NONE
 *
 * @return positive value: success;
 *   On error a negative value will be returned with the following error codes:
 *   ::EPERS_DBUS_MAINLOOP, ::EPERS_REGISTER_LIFECYCLE, ::EPERS_REGISTER_ADMIN
 *   - since V1.3.1: ::EPERS_NOPRCTABLE: the application (folder) is not or incorrectly installed
 *   - since V1.4.1: ::EPERS_INVALID_ARGUMENT: invalid argument provided
 */
int pclInitLibrary(const char* appname, int shutdownMode);


/**
 * @brief deinitialize client library
 *        This function will be called during the shutdown phase of the process which uses the PCL.
 *
 * @attention If this function will be called the PCL is NOT operational anymore.
 *            The function ::pclInitLibrary needs to be called to make PCL operational again.
 *            This function should only be called at the end of the lifecycle.
 *            It is not recommended to be called during a lifecycle,
 *            if it's needed your should know what you do and why exactly you need to do this in
 *            this way.
 *
 * @return positive value: success;
 *   On error a negative value will be returned.
 *   ::EPERS_NOT_INITIALIZED
 */
int pclDeinitLibrary(void);




/**
 * @brief pclLifecycleSet client library
 *        This function can be called if to flush and write back the data form cache to memory device.
 *        The function is only available if PCL_SHUTDOWN_TYPE_NONE has been used in pclInitLibrary.
 *
 * @attention In order to prevent misuse of this function the cancel shutdown request
 *            can only be called 3 times per lifecycle.
 *            The function called by an application with the parameter ::PCL_SHUTDOWN_CANCEL
 *            is limited to a value defined in ::Shutdown_MaxCount.
 *            When an application has reached this limit, it will not be able to store
 *            it's data anymore during the current lifecycle.
 *            The application isn't fully operable in this lifecycle anymore.
 *            In the next lifecycle the application can store data again until the limit above
 *            has been reached.
 *
 * @param shutdown PCL_SHUTDOWN for write back data when shutdown is requested,
 *        and PCL_SHUTDOWN_CANEL when shutdown cancel request has been received.
 *
 * @return positive value: success;
 *   On error a negative value will be returned with the following error codes:
 *   ::EPERS_COMMON, ::EPERS_SHUTDOWN_MAX_CANCEL, ::EPERS_SHUTDOWN_NO_PERMIT
 */
int pclLifecycleSet(int shutdown);


/** \} */

#ifdef __cplusplus
}
#endif

/** \} */ /* End of API */
/** \} */ /* End of MODULE */


#endif /* PERSISTENCY_CLIENT_LIBRARY_H */
