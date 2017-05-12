#pragma once
#include <string>
#include "THLogger.h"
#include <stdint.h>
#include <string>

#ifndef be16_to_cpu
#define be16_to_cpu(x) \
({ \
	be16 _x = (x); \
	(uint16_t) ((_x.b[0] << 8) | (_x.b[1])); \
})
#endif

#ifndef be24_to_cpu
#define be24_to_cpu(x) \
({ \
	be24 _x = (x); \
	(uint32_t) ((_x.b[0] << 16) | (_x.b[1] << 8) | (_x.b[2])); \
})
#endif


namespace SNI_Parser {

	char* sni_get_hostname(char* client_hello, int client_hello_len);

	typedef struct { uint8_t b[2]; } be16, le16;
	typedef struct { uint8_t b[3]; } be24, le24;

	// Big Endian to CPU helper functions

};