#ifndef PERSISTENCE_CLIENT_LIBRARY_KEY_H
#define PERSISTENCE_CLIENT_LIBRARY_KEY_H

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
 * @file           persistence_client_library_key.h
 * @ingroup        Persistence client library
 * @author         Ingo Huerner (XSe) / Guy Sagnes (Continental)
 * @brief          Header of the persistence client library.
 *                 Library provides an API to access persistent data
 * @see            
 */

#ifdef __cplusplus
extern "C" {
#endif


#define 	PERSIST_KEYVALUEAPI_INTERFACE_VERSION   (0x01000000U)



/**
 * @brief delete persistent data
 *
 * @param ldbid logical database ID
 * @param resource_id the resource ID
 * @param user_no  the user ID
 * @param seat_no  the seat number (seat 0 to 3)
 *
 * @return positive value: success; negative value: error code
 */
int key_delete(unsigned char ldbid, char* resource_id, unsigned char user_no, unsigned char seat_no);



/**
 * @brief gets the size of persistent data in bytes
 *
 * @param ldbid logical database ID
 * @param resource_id the resource ID
 * @param user_no  the user ID
 * @param seat_no  the seat number (seat 0 to 3)
 *
 * @return positive value: the size; negative value: error code
 */
int key_get_size(unsigned char ldbid, char* resource_id, unsigned char user_no, unsigned char seat_no);

/**
 * @brief close the access to a key-value identified by key handle
 *
 * @param key_handle key value handle return by key_handle_open()
 *
 * @return positive value: success; negative value: error code
 */
int key_handle_close(int key_handle);



/**
 * @brief gets the size of persistent data in bytes identified by key handle
 *
 * @param key_handle key value handle return by key_handle_open()
 *
 * @return positive value: the size; negative value: error code
 */
int key_handle_get_size(int key_handle);



/**
 * @brief open a key-value
 *
 * @param ldbid logical database ID
 * @param resource_id the resource ID
 * @param user_no  the user ID
 * @param seat_no  the seat number (seat 0 to 3)
 *
 * @return positive value: the key handle to access the value; negative value: Error code
 */
int key_handle_open(unsigned char ldbid, char* resource_id, unsigned char user_no, unsigned char seat_no);



/**
 * @brief reads persistent data identified by key handle
 *
 * @param key_handle key value handle return by key_handle_open()
 * @param buffer the buffer for persistent data
 * @param buffer_size size of buffer for reading
 *
 * @return positive value: the bytes read; negative value: error code
 */
int key_handle_read_data(int key_handle, unsigned char* buffer, unsigned long buffer_size);



/**
 * @brief register a change notification for persistent data
 *
 * @param key_handle key value handle return by key_handle_open()
 *
 * @return positive value: registration OK; negative value: error code
 */
int key_handle_register_notify_on_change(int key_handle);



/**
 * @brief writes persistent data identified by key handle
 *
 * @param key_handle key value handle return by key_handle_open()
 * @param buffer the buffer containing the persistent data to write
 * @param buffer_size the number of bytes to write
 *
 * @return positive value: the bytes written; negative value: error code
 */
int key_handle_write_data(int key_handle, unsigned char* buffer, unsigned long buffer_size);



/**
 * @brief reads persistent data identified by ldbid and resource_id
 *
 * @param ldbid logical database ID
 * @param resource_id the resource ID
 * @param user_no  the user ID
 * @param seat_no  the seat number (seat 0 to 3)
 * @param buffer the buffer to read the persistent data
 * @param buffer_size size of buffer for reading
 *
 * @return positive value: the bytes read; negative value: error code
 */
int key_read_data(unsigned char ldbid, char* resource_id, unsigned char user_no, unsigned char seat_no, unsigned char* buffer, unsigned long buffer_size);



/**
 * @brief register a change notification for persistent data
 *
 * @param ldbid logical database ID of the resource to monitor
 * @param resource_id the resource ID
 * @param user_no  the user ID
 * @param seat_no  the seat number (seat 0 to 3)
 *
 * @return positive value: registration OK; negative value: error code
 */
int key_register_notify_on_change(unsigned char ldbid, char* resource_id, unsigned char user_no, unsigned char seat_no);



/**
 * @brief writes persistent data identified by ldbid and resource_id
 *
 * @param ldbid logical database ID
 * @param resource_id the resource ID
 * @param user_no  the user ID
 * @param seat_no  the seat number (seat 0 to 3)
 * @param buffer the buffer containing the persistent data to write
 * @param buffer_size the number of bytes to write
 *
 * @return positive value: the bytes written; negative value: error code
 */
int key_write_data(unsigned char ldbid, char* resource_id, unsigned char user_no, unsigned char seat_no, unsigned char* buffer, unsigned long buffer_size);


#ifdef __cplusplus
}
#endif


#endif /* PERSISTENCY_CLIENT_LIBRARY_KEY_H */

