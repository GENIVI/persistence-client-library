#ifndef PERSISTENCE_CLIENT_LIBRARY_FILE_H
#define PERSISTENCE_CLIENT_LIBRARY_FILE_H

/******************************************************************************
 * Project         Persistency
 * (c) copyright   2011
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
 * @file           persistence_client_library_file.h
 * @ingroup        Persistence client library
 * @author         Ingo Huerner (XSe) / Guy Sagnes (Continental)
 * @brief          Header of the persistence client library.
 *                 Library provides an API to access persistent data
 * @see            
 */
/** \ingroup SSW_PERS */
/** \defgroup SSW_PERS_FILE Client: File access
 *  \{
 */
/** \defgroup SSW_PERS_FILE_INTERFACE API document
 *  \{
 */
#ifdef __cplusplus
extern "C" {
#endif


#define 	PERSIST_FILEAPI_INTERFACE_VERSION   (0x01000000U)



/**
 * @brief close the given POSIX file descriptor
 *
 * @param fd the file descriptor to close
 *
 * @return zero on success. On error a negative value will be returned with th follwoing error codes:
 *                          EPERS_LOCKFS
 */
int file_close(int fd);



/**
 * @brief get the size of the file given by the file descriptor
 *
 * @param fd the POSIX file descriptor
 *
 * @return positive value. On error the negative value -1 will be returned
 */
int file_get_size(int fd);



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
void* file_map_data(void* addr, long size, long offset, int fd);



/**
 * @brief open a file
 *
 * @param ldbid logical database ID
 * @param resource_id the resource ID
 * @param user_no  the user ID
 * @param seat_no  the seat number (seat 0 to 3)
 *
 * @return positive value: the POSIX file descriptor;
 * On error a negative value will be returned with th follwoing error codes:
 * EPERS_LOCKFS, EPERS_MAXHANDLE, EPERS_NOKEY, EPERS_NOKEYDATA, EPERS_NOPRCTABLE or EPERS_COMMON,
 */
int file_open(unsigned char ldbid, char* resource_id, unsigned char user_no, unsigned char seat_no);



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
int file_read_data(int fd, void * buffer, unsigned long buffer_size);



/**
 * @brief remove the file
 *
 * @param ldbid logical database ID
 * @param resource_id the resource ID
 * @param user_no  the user ID
 * @param seat_no  the seat number (seat 0 to 3)
 *
 * @return positive value: success;
 * On error a negative value will be returned with th follwoing error codes:
 * EPERS_LOCKFS or EPERS_COMMON
 */
int file_remove(unsigned char ldbid, char* resource_id, unsigned char user_no, unsigned char seat_no);



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
int file_seek(int fd, long int offset, int whence);



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
int file_unmap_data(void* address, long size);



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
int file_write_data(int fd, const void * buffer, unsigned long buffer_size);







#ifdef __cplusplus
}
#endif


/** \} */ /* End of API */
/** \} */ /* End of MODULE */


#endif /* PERSISTENCY_CLIENT_LIBRARY_FILE_H */

