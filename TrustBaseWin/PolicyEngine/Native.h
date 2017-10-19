#pragma once
#include <Windows.h>
#include "TBLogger.h"
#include "Query.h"
#include "QueryQueue.h"
#include "communications.h"

#define NATIVE_API_PIPE_NAME	L"\\\\.\\pipe\\TrustBaseNative"
#define MAX_HOSTLEN				256
#define EXPAND_AMOUNT			3
#define REQBUFSIZE				4096 + sizeof(native_request)
#define RESPBUFSIZE				sizeof(native_response)

#define REQUEST_TYPE_STACKOFX509	1
#define REQUEST_TYPE_RAWCERT		0

typedef struct native_request {
	int id;
	char host[MAX_HOSTLEN];
	short port;
	int chain_type;
	int chain_len;
	// chain after native_request in buf
} native_request;

typedef struct native_response {
	int id;
	int reply;
} native_response;

typedef struct native_inst{
	OVERLAPPED olp;
	HANDLE hPipe;
	DWORD inbufsz;
	UINT8* inbuf;
	DWORD byteRead;
} native_inst;

namespace NativeAPI {
	bool init_native(QueryQueue* in_qq, int in_plugin_count);
	bool listen_for_queries();
	void handle_client(HANDLE hPipe);
	bool send_response(int result, HANDLE hPipe, int id);

	void WINAPI readCompletion(DWORD err, DWORD bytesread, OVERLAPPED* overlapped);
}