#ifndef PERSISTENCE_CLIENT_LIBRARY_FILE_H
#define PERSISTENCE_CLIENT_LIBRARY_FILE_H

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
 * @file           persistence_client_library_file.h
 * @ingroup        Persistence client library
 * @author         Ingo Huerner (XSe) / Guy Sagnes (Continental)
 * @brief          Header of the persistence client library.
 *                 Library provides an API to access persistent data
 * @see
 */
/** \ingroup GEN_PERS */
/** \defgroup PERS_FILE Client: File access
 *  \{
 */
/** \defgroup PERS_FILE_INTERFACE API document
 *  \{
 */
#ifdef __cplusplus
extern "C" {
#endif


#define  PERSIST_FILEAPI_INTERFACE_VERSION   (0x03010000U)

#include "persistence_client_library.h"

/** \defgroup PCL_FILE functions file access
 * \{
 */

/**
 * @brief close the given POSIX file descriptor
 *
 * @param fd the file descriptor to close
 *
 * @return zero on success.
 * On error a negative value will be returned with th following error codes:
 * ::EPERS_MAXHANDLE ::EPERS_COMMON
 * If ::EPERS_COMMON will be returned errno will be set.
 */
int pclFileClose(int fd);



/**
 * @brief get the size of the file given by the file descriptor
 *
 * @param fd the POSIX file descriptor
 *
 * @return positive value (0 or greater). On error ::EPERS_NOT_INITIALIZED, ::EPERS_COMMON
 * If ::EPERS_COMMON will be returned errno will be set.
 */
int pclFileGetSize(int fd);



/**
 * @brief map a file into the memory
 *
 * @param addr if NULL, kernel chooses address
 * @param size the size in bytes to map into the memory
 * @param offset in the file to map
 * @param fd the POSIX file descriptor of the file to map
 *
 * @return a pointer to the mapped area, or on error the value MAP_FAILED or
 *  EPERS_MAP_FAILEDLOCK if filesystem is currrently locked
 */
void* pclFileMapData(void* addr, long size, long offset, int fd);



/**
 * @brief open a file
 *
 * @param ldbid logical database ID
 * @param resource_id the resource ID
 * @param user_no  the user ID; user_no=0 can not be used as user-ID beacause ‘0’ is defined as System/node
 * @param seat_no  the seat number
 *
 * @return positive value (0 or greater): the POSIX file descriptor;
 * On error a negative value will be returned with th following error codes:
 * ::EPERS_LOCKFS, ::EPERS_MAXHANDLE, ::EPERS_NOKEY, ::EPERS_NOKEYDATA,
 * ::EPERS_NOPRCTABLE, ::EPERS_NOT_INITIALIZED, ::EPERS_COMMON
 * If ::EPERS_COMMON will be returned errno will be set.
 */
int pclFileOpen(unsigned int ldbid, const char* resource_id, unsigned int user_no, unsigned int seat_no);



/**
 * @brief read persistent data from a file
 *
 * @param fd POSIX file descriptor
 * @param buffer buffer to read the data
 * @param buffer_size the size buffer for reading
 *
 * @return positive value (0 or greater): the size read;
 * On error a negative value will be returned with th following error codes:
 * ::EPERS_NOT_INITIALIZED, ::EPERS_LOCKFS, ::EPERS_COMMON.
 * If ::EPERS_COMMON will be returned errno will be set
 */
int pclFileReadData(int fd, void * buffer, int buffer_size);



/**
 * @brief remove the file
 *
 * @param ldbid logical database ID
 * @param resource_id the resource ID
 * @param user_no  the user ID; user_no=0 can not be used as user-ID beacause ‘0’ is defined as System/node
 * @param seat_no  the seat number
 *
 * @return positive value (0 or greater): success;
 * On error a negative value will be returned with th following error codes:
 * ::EPERS_NOT_INITIALIZED, ::EPERS_LOCKFS, ::EPERS_COMMON.
 * If ::EPERS_COMMON will be returned errno will be set
 */
int pclFileRemove(unsigned int ldbid, const char* resource_id, unsigned int user_no, unsigned int seat_no);



/**
 * @brief reposition the file descriptor
 *
 * @param fd the POSIX file descriptor
 * @param offset the reposition offset
 * @param whence the direction to reposition
                 SEEK_SET
                      The offset is set to offset bytes.
                 SEEK_CUR
                      The offset is set to its current location plus offset bytes.
                 SEEK_END
                      The offset is set to the size of the file plus offset bytes.
 *
 * @return positive value (0 or greater): resulting offset location;
 * On error a negative value will be returned with th following error codes:
 * ::EPERS_LOCKFS, ::EPERS_NOT_INITIALIZED, ::EPERS_COMMON.
 * If ::EPERS_COMMON will be returned errno will be set
 */
int pclFileSeek(int fd, long int offset, int whence);



/**
 * @brief unmap the file from the memory
 *
 * @param address the address to unmap
 * @param size the size in bytes to unmap
 *
 * @return on success 0;
 * On error a negative value will be returned with th following error codes:
 * ::EPERS_LOCKFS, EPERS_NOT_INITIALIZED, ::EPERS_COMMON.
 * If ::EPERS_COMMON will be returned errno will be set
 */
int pclFileUnmapData(void* address, long size);



/**
 * @brief write persistent data to file
 *
 * @param fd the POSIX file descriptor
 * @param buffer the buffer to write
 * @param buffer_size the size of the buffer to write in bytes
 *
 * @return positive value (0 or greater): bytes written;
 * On error a negative value will be returned with th following error codes:
 * ::EPERS_LOCKFS, ::EPERS_NOT_INITIALIZED or ::EPERS_COMMON ::EPERS_RESOURCE_READ_ONLY
 * If ::EPERS_COMMON will be returned errno will be set.
 */
int pclFileWriteData(int fd, const void * buffer, int buffer_size);



/**
 * @brief create a path to a file
 *
 * @param ldbid logical database ID
 * @param resource_id the resource ID
 * @param user_no  the user ID; user_no=0 can not be used as user-ID beacause ‘0’ is defined as System/node
 * @param seat_no  the seat number
 * @param path the path to the file
 * @param size the size of the path
 *
 * @note the allocated memory for the path string will be freed in pclFileReleasePath
 *
 * @return positive value (0 or greater) on success a path to a file corresponding to the storage resource addressed
 *         by the given ldbid/resource_id/user_no/seat_no.
 *         This function can be used to a legacy program or other program that needs direct file storage (e.g. sqlite)
 *         while still giving that program the benefit from the persistence layer and cause the program to use the
 *         storage settings that have been configured for the given resource_id.
 *         This way of access should only be used as a last resort if using the given key-value or file API is not feasible.
 *         On error a negative value will be returned with th following error codes:
 *         ::EPERS_LOCKFS or ::EPERS_COMMON
 *         If ::EPERS_COMMON will be returned errno will be set.
 */
int pclFileCreatePath(unsigned int ldbid, const char* resource_id, unsigned int user_no, unsigned int seat_no, char** path, unsigned int* size);


/**
 * @brief release a file path
 *
 * @param pathHandle the path to the file
 *
 * @note the allocated memory in pclFileCreatePath for the path will freed in the function
 *
 * @return positive value (0 or greater): success;
 * On error a negative value will be returned with th following error codes:
 * ::EPERS_LOCKFS or ::EPERS_COMMON
 * If ::EPERS_COMMON will be returned errno will be set.
 */
int pclFileReleasePath(int pathHandle);

/** \} */ 

#ifdef __cplusplus
}
#endif


/** \} */ /* End of API */
/** \} */ /* End of MODULE */


#endif /* PERSISTENCY_CLIENT_LIBRARY_FILE_H */
