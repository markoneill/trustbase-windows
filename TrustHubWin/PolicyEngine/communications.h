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
#define COMMUNICATIONS_DEBUG_QUERY	"C:/Users/ilab/Source/Repos/TrustKern/TrustHubWin/PolicyEngine/example_query.bin"

namespace Communications {
	typedef enum THResponseType { RESPONSE_ALLOW = 0, RESPONSE_BLOCK = 1, WAITING_ON_RESPONSE = 7 } THResponseType;

	bool send_response(THResponseType result, UINT64 flowHandle);
	bool recv_query();
	bool debug_recv_query();
	Query* parse_query(UINT8* buffer, UINT64 buflen);
	bool init_communication(QueryQueue* qq, int plugin_count);
	bool listen_for_queries();
	void cleanup();

	extern std::atomic<bool> keep_running;
}