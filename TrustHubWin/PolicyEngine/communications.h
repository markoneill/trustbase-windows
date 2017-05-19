#pragma once

#include <Windows.h>
#include <atomic>
#include <exception>
#include "THLogger.h"
#include "Query.h"
#include "QueryQueue.h"

#define TRUSTHUBKERN	L"\\\\.\\TrustHub"
#define INITIALBUFSIZE	16384
#define COMMUNICATIONS_DEBUG_MODE	true
#define COMMUNICATIONS_DEBUG_QUERY	"./example_query_Luke.bin"

namespace Communications {

	bool send_response(int result, UINT64 flowHandle);
	bool recv_query();
	bool debug_recv_query();
	Query* parse_query(UINT8* buffer, UINT64 buflen);
	bool init_communication(QueryQueue* qq, int plugin_count);
	bool listen_for_queries();
	void cleanup();

	extern std::atomic<bool> keep_running;
}