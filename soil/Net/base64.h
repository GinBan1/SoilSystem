#ifndef __BASE64_H__
#define __BASE64_H__

#include "bsp.h"
#include <stddef.h>
#include <stdint.h>

#define MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL               -0x002A  /**< Output buffer too small. */
#define MBEDTLS_ERR_BASE64_INVALID_CHARACTER              -0x002C  /**< Invalid character in input. */

#ifdef __cplusplus
extern "C" {
#endif


int BASE64_Encode(unsigned char *dst, size_t dlen, size_t *olen,
						  const unsigned char *src, size_t slen);

int BASE64_Decode(unsigned char *dst, size_t dlen, size_t *olen,
						  const unsigned char *src, size_t slen);


#ifdef __cplusplus
}
#endif


#endif