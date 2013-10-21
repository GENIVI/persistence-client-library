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


#define  PERSIST_FILEAPI_INTERFACE_VERSION   (0x03000000U)

#include "persistence_client_library.h"

/** \defgroup PCL_FILE functions file access
 * \{
 */

/**
 * @brief close the given POSIX file descriptor
 *
 * @param fd the file descriptor to close
 *
 * @return zero on success. On error a negative value will be returned with th follwoing error codes:
 *                          ::EPERS_LOCKFS, ::EPERS_MAXHANDLE
 */
int pclFileClose(int fd);



/**
 * @brief get the size of the file given by the file descriptor
 *
 * @param fd the POSIX file descriptor
 *
 * @return positive value. On error the negative value -1 will be returned
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
 * @return positive value: the POSIX file descriptor;
 * On error a negative value will be returned with th follwoing error codes:
 * EPERS_LOCKFS, EPERS_MAXHANDLE, EPERS_NOKEY, EPERS_NOKEYDATA, EPERS_NOPRCTABLE or EPERS_COMMON,
 */
int pclFileOpen(unsigned int ldbid, const char* resource_id, unsigned int user_no, unsigned int seat_no);



/**
 * @brief read persistent data from a file
 *
 * @param fd POSIX file descriptor
 * @param buffer buffer to read the data
 * @param buffer_size the size buffer for reading
 *
 * @return positive value: the size read;
 * On error a negative value will be returned with th follwoing error codes:
 * EPERS_LOCKFS or EPERS_COMMON
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
 * @return positive value: success;
 * On error a negative value will be returned with th follwoing error codes:
 * EPERS_LOCKFS or EPERS_COMMON
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
 * @return positive value: resulting offset location;
 * On error a negative value will be returned with th follwoing error codes:
 * EPERS_LOCKFS or EPERS_COMMON
 */
int pclFileSeek(int fd, long int offset, int whence);



/**
 * @brief unmap the file from the memory
 *
 * @param address the address to unmap
 * @param size the size in bytes to unmap
 *
 * @return on success 0;
 * On error a negative value will be returned with th follwoing error codes:
 * EPERS_LOCKFS or EPERS_COMMON
 */
int pclFileUnmapData(void* address, long size);



/**
 * @brief write persistent data to file
 *
 * @param fd the POSIX file descriptor
 * @param buffer the buffer to write
 * @param buffer_size the size of the buffer to write in bytes
 *
 * @return positive value: bytes written;
 * On error a negative value will be returned with th follwoing error codes:
 * EPERS_LOCKFS or EPERS_COMMON
 */
int pclFileWriteData(int fd, const void * buffer, int buffer_size);

/** \} */ 

#ifdef __cplusplus
}
#endif


/** \} */ /* End of API */
/** \} */ /* End of MODULE */


#endif /* PERSISTENCY_CLIENT_LIBRARY_FILE_H */
