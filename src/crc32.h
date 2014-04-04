#ifndef CRC32_H
#define CRC32_H

/******************************************************************************
 * Project         Persistency
 * (c) copyright   2012
 * Company         XS Embedded GmbH
 *****************************************************************************/
/******************************************************************************
 * Copyright
 *
 * This Source Code Form is subject to the terms of the
 * Mozilla Public License, v. 2.0. If a  copy of the MPL was not distributed
 * with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
******************************************************************************/
 /**
 * @file           crc32.h
 * @ingroup        Persistence client library
 * @author         Ingo Huerner
 * @brief          Header of crc32 checksum generation
 * @see            
 */

#ifdef __cplusplus
extern "C" {
#endif


#define  PERSIST_CLIENT_LIBRARY_INTERFACE_VERSION   (0x01000000U)

#include <string.h>

unsigned int pclCrc32(unsigned int crc, const unsigned char *buf, size_t theSize);


#ifdef __cplusplus
}
#endif

#endif /* CRC32_H */
