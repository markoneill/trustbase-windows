#pragma once
#include <stdint.h>

#define PLUGIN_RESPONSE_ERROR	-1
#define PLUGIN_RESPONSE_VALID	1
#define PLUGIN_RESPONSE_INVALID	0
#define PLUGIN_RESPONSE_ABSTAIN	2

#define PLUGIN_INITIALIZE_OK	0
#define PLUGIN_INITIALiZE_ERR	-1

#define PLUGIN_FINALIZE_OK		0
#define PLUGIN_FINALIZE_ERR		-1

typedef struct query_data_t {
	int id;
	char* hostname;
	uint16_t port;
	unsigned char* raw_chain;
	size_t raw_chain_len;
	char* client_hello;
	size_t client_hello_len;
	char* server_hello;
	size_t server_hello_len;
} query_data_t;

typedef struct init_data_t {
	int plugin_id;
	const char* plugin_path;
	int(*callback)(int plugin_id, int query_id, int plugin_response);
} init_data_t;


typedef int(__stdcall *query_func_t)(query_data_t*);
typedef int(__stdcall *initialize_func_t)(init_data_t*);
typedef int(__stdcall *finalize_func_t)(void);