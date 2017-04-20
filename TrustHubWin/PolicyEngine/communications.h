#pragma once

#include <Windows.h>
#include "THLogger.h"

#define TRUSTHUBKERN	L"\\\\.\\TrustHub"
#define INITIALBUFSIZE	16384
#define COMMUNICATIONS_DEBUG_MODE	true
#define COMMUNICATIONS_DEBUG_QUERY	"./debug_query.data"

namespace Communications {
	typedef enum THResponseType { RESPONSE_ALLOW = 0, RESPONSE_BLOCK = 1, WAITING_ON_RESPONSE = 7 } THResponseType;

	bool send_response(THResponseType result, UINT64 flowHandle);
	bool recv_query();
	bool init_communication();
	bool listen_for_queries();
	void cleanup();
}