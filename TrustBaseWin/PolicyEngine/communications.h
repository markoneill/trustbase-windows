#pragma once
#include <Windows.h>
#include <atomic>
#include <exception>
#include "Query.h"
#include "QueryQueue.h"
#include <stdio.h>

#define TRUSTBASEKERN	L"\\\\.\\TrustBase"
#define INITIALBUFSIZE	16384

#define COMMUNICATIONS_DEBUG_MODE	false
#define COMMUNICATIONS_DEBUG_QUERY	"./example_query.bin"

namespace Communications {

	bool send_response(int result, UINT64 flowHandle);
	bool recv_query();
	bool debug_recv_query();
	Query* parse_query(UINT8* buffer, UINT64 buflen);
	bool init_communication(QueryQueue* qq, int plugin_count);
	bool listen_for_queries();
	void cleanup();
	unsigned int ntoh24(const UINT8* data);
	void hex_dump(const UINT8* buf, UINT64 len);

	extern std::atomic<bool> keep_running;
}