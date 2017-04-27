#include "stdafx.h"
#include "communications.h"

namespace Communications {
	HANDLE file;
	UINT8* buf;
	DWORD bufsize;
	UINT8* response_buf;
	DWORD response_bufsize;
	std::atomic<bool> keep_running (true);
}

bool Communications::send_response(Communications::THResponseType result, UINT64 flowHandle) {
	((UINT64*)response_buf)[0] = flowHandle;
	((UINT8*)response_buf)[sizeof(UINT64)] = (UINT8)result;
	return true;
}

bool Communications::recv_query() {
	DWORD readlen;
	UINT64 toRead = 0;
	UINT64 Read = 0;
	UINT64 flowHandle = 0;
	UINT8* bufcur;

	bufcur = buf;

	while (true) {
		// read until we have read a whole query
		if (ReadFile(file, bufcur, (DWORD)((bufsize - 1) - (bufcur - buf)), &readlen, NULL)) {
			if (toRead == 0) {
				toRead = ((UINT64*)buf)[0];
				flowHandle = ((UINT64*)buf)[1];
				thlog() << "Got a query of " << toRead << " bytes, with flow handle " << flowHandle;
			}

			Read += readlen;
		} else {
			thlog() << "Couldn't read query!";
			return false;
		}
		if (Read >= toRead) {
			break;
		} else if (2 * Read < bufsize) {
			// we need to double our buffer, copy our buf, drop it, and point bufcur to the end of the data
			thlog() << "Doubling the buffer size from " << bufsize << " to " << bufsize * 2 << " for queries";
			bufsize = bufsize * 2;
			bufcur = new UINT8[bufsize];
			memcpy(bufcur, buf, Read);

			delete[] buf;
			buf = bufcur;
			bufcur = buf + Read;
		}
	}

	thlog() << "Querying plugins";
	//send_response(RESPONSE_ALLOW, flowHandle);
	return true;
}

bool Communications::init_communication() {
	if (!buf) {
		bufsize = INITIALBUFSIZE;
		buf = new UINT8[bufsize];
	}

	if (!response_buf) {
		response_bufsize = sizeof(UINT64) + 1;
		response_buf = new UINT8[response_bufsize];
	}

	if (COMMUNICATIONS_DEBUG_MODE) {
		thlog() << "STARTING COMMUNICATIONS IN DEBUG MODE";
		return true;
	}

	file = CreateFileW(TRUSTHUBKERN, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
	if (file == NULL) {
		thlog() << "Couldn't open trusthub kernel file.";
		delete[] buf;
		delete[] response_buf;
		return false;
	}
	return true;
}

bool Communications::listen_for_queries() {
	if (COMMUNICATIONS_DEBUG_MODE) {
		// debug_recv_query();
		return true;
	}

	while (keep_running) {
		if (!recv_query()) {
			thlog() << "Could not handle query";
		}
	}
	return true;
}

void Communications::cleanup() {
	if (buf) {
		delete[] buf;
	}
	if (response_buf) {
		delete[] response_buf;
	}
	if (file) {
		CloseHandle(file);
	}
}