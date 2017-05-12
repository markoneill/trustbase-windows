#pragma once
#include <string>
#include "THLogger.h"
#include <stdint.h>
#include <string>




namespace SNI_Parser {

	char* sni_get_hostname(char* client_hello, int client_hello_len);

	typedef struct { uint8_t b[2]; } be16, le16;
	typedef struct { uint8_t b[3]; } be24, le24;

	// Big Endian to CPU helper functions
	inline unsigned int   be24_to_cpu(be24 x);
	inline unsigned short be16_to_cpu(be16 x);

};