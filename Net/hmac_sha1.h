#ifndef __HMAC_SHA1_H__
#define __HMAC_SHA1_H__

#include "bsp.h"

//
#define MAX_MESSAGE_LENGTH		1024


void hmac_sha1(
	unsigned char *key,
	int key_length,
	unsigned char *data,
	int data_length,
	unsigned char *digest
);
void sha1(
	unsigned char *message,
	int message_length,
	unsigned char *digest
);

#endif


