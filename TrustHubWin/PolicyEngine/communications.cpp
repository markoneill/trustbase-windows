#include "stdafx.h"
#include "communications.h"

namespace Communications {
	HANDLE file;
	UINT8* buf;
	DWORD bufsize;
	UINT8* response_buf;
	DWORD response_bufsize;
	std::atomic<bool> keep_running (true);

	QueryQueue* qq;
	int plugin_count;
}

bool Communications::send_response(int result, UINT64 flowHandle) {
	((UINT64*)response_buf)[0] = flowHandle;
	((UINT8*)response_buf)[sizeof(UINT64)] = (UINT8)result;

	if (COMMUNICATIONS_DEBUG_MODE) {
		thlog() << "Would have responded " << ((result == PLUGIN_RESPONSE_VALID) ? "valid" : "invalid");
		return true;
	}
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
			buf = bufcur;
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

	// parse the message
	Query* query = parse_query(buf, Read);
	if (!query) {
		thlog() << "Could not parse the query";
		return false;
	}

	thlog() << "Querying plugins";
	// Poll the plugins
	qq->enqueue_and_link(query);
	return true;
}

bool Communications::debug_recv_query() {
	// parse an example query, so we can test the policy engine without having to actually start the kernel module
	std::ifstream input(COMMUNICATIONS_DEBUG_QUERY, std::ios::binary|std::ios::ate);
	std::streamsize size = input.tellg();
	input.seekg(0, std::ios::beg);

	UINT8* buf = new UINT8[size];
	if (!input.read((char*)buf, size)) {
		thlog() << "Error, could not read in debug query";
	}
	input.close();

	for (int i = 0; i < 3; i++) {
		Query* query = parse_query(buf, size);

		if (!query) {
			thlog() << "Could not parse the debug query";
			return false;
		}

		query->printQueryInfo();

		thlog() << "Enqueing query and querying plugins";

		qq->enqueue_and_link(query); // plugin count + 1 for decider thread
		Sleep(1000);
	}

	// flip keep running
	Communications::keep_running = false;
	qq->enqueue(nullptr);
	return true;
}

Query* Communications::parse_query(UINT8* buffer, UINT64 buflen) {
	UINT8* cursor = buffer;
	UINT64 length;
	UINT64 flowHandle;
	UINT64 processId;
	UINT32 processPathSize;
	char* processPath = NULL;
	UINT32 clientHelloSize;
	UINT8* clientHello = NULL;
	UINT32 serverHelloSize;
	UINT8* serverHello = NULL;
	UINT32 certSize;
	UINT8* cert = NULL;

	try {
		length = *((UINT64*)cursor);
		cursor += sizeof(UINT64);
		if ((UINT64)(cursor - buffer) > buflen) {
			thlog() << "Parsing length : buflen=" << buflen << ", cursor=" << (UINT64)(cursor - buffer);
			throw std::runtime_error("");
		}

		flowHandle = *((UINT64*)cursor);
		cursor += sizeof(UINT64);
		if ((UINT64)(cursor - buffer) > buflen) {
			thlog() << "Parsing flowHandle : buflen=" << buflen << ", cursor=" << (UINT64)(cursor - buffer);
			throw std::runtime_error("");
		}

		processId = *((UINT64*)cursor);
		cursor += sizeof(UINT64);
		if ((UINT64)(cursor - buffer) > buflen) {
			thlog() << "Parsing processId: buflen=" << buflen << ", cursor=" << (UINT64)(cursor - buffer);
			throw std::runtime_error("");
		}

		processPathSize = *((UINT32*)cursor);
		cursor += sizeof(UINT32);
		if ((UINT64)(cursor + processPathSize - buffer) > buflen) {
			thlog() << "Parsing path: buflen=" << buflen << ", cursor=" << (UINT64)(cursor - buffer);
			throw std::runtime_error("");
		}
		processPath = new char[processPathSize];
		if (!processPath) {
			thlog() << "Could not allocate " << processPathSize << " bytes while parseing query";
			throw std::bad_alloc();
		}
		memcpy(processPath, cursor, processPathSize);
		cursor += processPathSize;

		clientHelloSize = *((UINT32*)cursor);
		cursor += sizeof(UINT32);
		if ((UINT64)(cursor + clientHelloSize - buffer) > buflen) {
			thlog() << "Parsing client hello: buflen=" << buflen << ", cursor=" << (UINT64)(cursor - buffer) << ", size=" << clientHelloSize;
			throw std::runtime_error("");
		}
		clientHello = new UINT8[clientHelloSize];
		if (!clientHello) {
			thlog() << "Could not allocate " << clientHelloSize << " bytes while parseing query";
			throw std::bad_alloc();
		}
		memcpy(clientHello, cursor, clientHelloSize);
		cursor += clientHelloSize;

		serverHelloSize = *((UINT32*)cursor);
		cursor += sizeof(UINT32);
		if ((UINT64)(cursor + serverHelloSize - buffer) > buflen) {
			thlog() << "Parsing server hello: buflen=" << buflen << ", cursor=" << (UINT64)(cursor - buffer) << ", size=" << serverHelloSize;
			throw std::runtime_error("");
		}
		serverHello = new UINT8[serverHelloSize];
		if (!serverHello) {
			thlog() << "Could not allocate " << serverHelloSize << " bytes while parseing query";
			throw std::bad_alloc();
		}
		memcpy(serverHello, cursor, serverHelloSize);
		cursor += serverHelloSize;

		certSize = *((UINT32*)cursor);
		cursor += sizeof(UINT32);
		if ((UINT64)(cursor + certSize - buffer) > buflen) {
			thlog() << "Parsing cert: buflen=" << buflen << ", cursor=" << (UINT64)(cursor - buffer) << ", size=" << certSize;
			throw std::runtime_error("");
		}
		cert = new UINT8[certSize];
		if (!clientHello) {
			thlog() << "Could not allocate " << clientHelloSize << " bytes while parsing query";
			throw std::bad_alloc();
		}
		memcpy(cert, cursor, certSize);
		cursor += certSize;
	} catch (const std::runtime_error& e) {
		delete[] processPath;
		delete[] cert;
		delete[] serverHello;
		delete[] clientHello;
		thlog() << "Error while parsing Query " << e.what();
		return nullptr;
	} catch (const std::bad_alloc& e) {
		delete[] processPath;
		delete[] cert;
		delete[] serverHello;
		delete[] clientHello;
		thlog() << "Error allocating while parsing Query " << e.what();
		return nullptr;
	}

	// create a query
	Query* query = new Query(flowHandle, processId, processPath, cert, certSize, clientHello, clientHelloSize, serverHello, serverHelloSize, plugin_count);
	return query;
}

bool Communications::init_communication(QueryQueue* in_qq, int in_plugin_count) {
	qq = in_qq;
	plugin_count = in_plugin_count;
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
		debug_recv_query();
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