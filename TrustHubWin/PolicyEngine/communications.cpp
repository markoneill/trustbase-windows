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
	std::mutex readWrite_mux;
	std::condition_variable canRead_cv;
	std::condition_variable canWrite_cv;

	bool canReadCantWrite = true;
}

bool Communications::send_response(int result, UINT64 flowHandle) {
	
	((UINT64*)response_buf)[0] = flowHandle;
	((UINT8*)response_buf)[sizeof(UINT64)] = (UINT8)result;

	if (COMMUNICATIONS_DEBUG_MODE) {
		thlog() << "Would have responded " << ((result == PLUGIN_RESPONSE_VALID) ? "valid" : "invalid") << flowHandle;
		return true;
	}
	else
	{
		thlog() << "****** about to respond with " << ((result == PLUGIN_RESPONSE_VALID) ? "valid" : "invalid") << flowHandle;
	
		//synchronous lock for reading and writing
		//todo: find a soultion that allows us to quickly read and write without issue (see read)
		std::unique_lock<std::mutex> lck(readWrite_mux);
		//wait for write access
		while (canReadCantWrite == true) {
			canWrite_cv.wait(lck);
		}
		if (WriteFile(file, response_buf, 0x10, NULL, NULL)) {
			thlog() << "Successfully ";
		}
		else
		{
			thlog() << "Unsuccessfully ";
		}
		thlog() << "------responded with " << ((result == PLUGIN_RESPONSE_VALID) ? "valid" : "invalid") << flowHandle;
		
		//allow read
		canRead_cv.notify_one();
		canReadCantWrite = true;
		lck.unlock();
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

	//synchronous lock for reading and writing
	//todo: find a soultion that allows us to quickly read and write without issue
	//With out a synchronous lock, we run into a read blocking problem.
	//example: the policy engine reads 4 queries before answering
	//			next, the policy engine blocks on read access.
	//			then, one of the queries has an anwser. They cannot answer until 
	//				the read is done blocking or in otherwords, until we receive another message.
	//			When reads finally do come in, the queries with answers finally get to send their answer, but they are much too late.
	//			Also, if there are lots of anwsers ready, they will have to wait for another read block to end.
	//			
	//			Essentially because in order to write we have to wait for a read, we can only load a page if we try to load like 10 pages.
	//			Still only the first 1 or 2 will load.
	std::unique_lock<std::mutex> lck(readWrite_mux);

	//wait for read access
	while (canReadCantWrite == false) {
		canRead_cv.wait(lck);
	}
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
			thlog(LOG_WARNING) << "Couldn't read query!";
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
	//allow write
	canWrite_cv.notify_one();
	canReadCantWrite = false;
	lck.unlock();
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
	UINT32 certObjectSize;
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

		//the cert object follows the following structure:
		//certObjSize (4 bytes) + certSize (3 bytes) + cert
		//example: 21 0d 00 00 00 0d 1e ... [start cert]
		// certObjSize 21 0d 00 00 is read right to left = 3361
		// certSize 00 0d 1e is read left to right = 3358

		certObjectSize = *((UINT32*)cursor);
		cursor += sizeof(UINT32);
		if ((UINT64)(cursor + certObjectSize - buffer) > buflen) {
			thlog() << "Parsing cert: buflen=" << buflen << ", cursor=" << (UINT64)(cursor - buffer) << ", size=" << certObjectSize;
			throw std::runtime_error("");
		}

		certSize = ntoh24(cursor); 
		cursor += sizeof(UINT8) + sizeof(UINT16);
		if ((UINT64)(cursor + certSize - buffer) > buflen) {
			thlog() << "Parsing cert: buflen=" << buflen << ", cursor=" << (UINT64)(cursor - buffer) << ", size=" << certSize;
			throw std::runtime_error("");
		}

		cert = new UINT8[certSize];
		if (!cert) {
			thlog() << "Could not allocate " << certSize << " bytes while parsing query";
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

unsigned int Communications::ntoh24(const UINT8* data) {
	unsigned int ret = (data[0] << 16) | (data[1] << 8) | data[2];
	return ret;
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
	if (file == NULL ||  file == INVALID_HANDLE_VALUE) {
		thlog(LOG_ERROR) << "Couldn't open trusthub kernel file.";
		thlog(LOG_ERROR) << GetLastError();
		delete[] buf;
		delete[] response_buf;
		return false;
	}
	return true;
}

bool Communications::listen_for_queries() {
	bool success = true;
	if (COMMUNICATIONS_DEBUG_MODE) {
		debug_recv_query();
		keep_running = false;
	}

	while (keep_running) {
		if (!recv_query()) {
			thlog(LOG_WARNING) << "Could not handle query";
			success = false;
			// flip keep running to close the Policy Engine
			keep_running = false;
		}
	}

	// unlock the plugins so they can close
	qq->enqueue(nullptr);

	return success;
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