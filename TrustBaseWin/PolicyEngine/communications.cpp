#include "stdafx.h"
#include "communications.h"
#include <ctime>


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
		tblog() << "Would have responded " << ((result == PLUGIN_RESPONSE_VALID) ? "valid" : "invalid") << flowHandle;
		return true;
	}
	else{
		tblog() << "about to respond with " << ((result == PLUGIN_RESPONSE_VALID) ? "valid" : "invalid") << " for handle " << flowHandle;
		//Using Overlapped to allow async read/write. We only allow upto a single read and single write at the same time. 
		//This allows the ReadFile call to block on its own  thread, without also blocking the WriteFile function. 
		OVERLAPPED overlappedWrite = { 0 };
		overlappedWrite.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		DWORD dwBytesWritten = 0;

		if (!overlappedWrite.hEvent){
			//bad
			tblog() << "CreateEvent failed";
			return false;
		}
		overlappedWrite.Offset = 0;

		/*
		std::time_t t = std::time(0);
		bool writeFileSuccessful = WriteFile(file, response_buf, 0x10, NULL, NULL);
		std::time_t e = std::time(0);
		tblog() << "Current Time after write " << e-t;
		*/

		bool writeFileSuccessful = WriteFile(file, response_buf, 0x10, NULL, &overlappedWrite);

		DWORD dwWaitRes = WaitForSingleObject(overlappedWrite.hEvent, INFINITE); //Could wait forever
		if (dwWaitRes == WAIT_FAILED){
			//bad
			tblog() << "WAIT_FAILED";
			tblog() << "quitting write function";
			return false;
		}
		
		//is write done? if not, we need to wait for overlapped async write
		if (!writeFileSuccessful) {
			if (GetLastError() != ERROR_IO_PENDING) {
				tblog() << "Unsuccessfully ";
				tblog() << "NOT ERROR_IOPENDING ";
				tblog() << "quitting write function";
				return false;
			}
			else {
				writeFileSuccessful = (bool)GetOverlappedResult(file, &overlappedWrite, &dwBytesWritten, true); 
			}
		}

		//async write is over
		if (writeFileSuccessful) {
			tblog() << "Successfully ";
		}
		else{
			tblog() << "Unsuccessfully ";
		}

		if (!writeFileSuccessful) {
			if (GetLastError() != ERROR_IO_PENDING) {
				tblog() << "Unsuccessfully ";
				tblog() << "NOT ERROR_IOPENDING ";
				tblog() << "quitting write function";
				return false;
			}
		}

		if (overlappedWrite.hEvent){
			CloseHandle(overlappedWrite.hEvent);
		}

		tblog(LOG_INFO) << "responded with " << ((result == PLUGIN_RESPONSE_VALID) ? "valid" : "invalid");
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
	/*
	while (true) {
		// read until we have read a whole query
		// We don't have to do an overlapped read here, because the write it is in it's own thread
		if (!ReadFile(file, (LPVOID)bufcur, (DWORD)((bufsize - 1) - (bufcur - buf)), &readlen, NULL)) {
			//failed for some reason?
			tblog() << "Could not read query file! GLE = " << GetLastError();
			return false;
		}

		if (toRead == 0) {
			toRead = ((UINT64*)buf)[0];
			flowHandle = ((UINT64*)buf)[1];
			tblog() << "Got a query of " << toRead << " bytes, with flow handle " << flowHandle << " and got " << readlen << "of it";
		}
		Read += readlen;

		if (Read >= toRead) {
			// we are done
			break;
		}
		else if ((2 * Read) > bufsize) {
			// we need to double our buffer, copy our buf, drop it, and point bufcur to the end of the data
			tblog() << "Doubling the buffer size from " << bufsize << " to " << bufsize * 2 << " for queries";
			bufsize = bufsize * 2;
			UINT8* newbuf = new UINT8[bufsize];
			memcpy(newbuf, buf, Read);

			delete[] buf;
			buf = newbuf;
			bufcur = buf + Read;
		} else {
			tblog() << "Didn't get it all on this round, but didn't fill the buffer";
			bufcur = bufcur + Read; // should never happen really
		}
	}
	*/
	

	
	while (true) {
		// read until we have read a whole query

		//Using Overlapped to allow async read/write. We only allow upto a single read and single write at the same time. 
		//This allows the ReadFile call to block on its own  thread, without also blocking the WriteFile function. 
		OVERLAPPED overlappedRead = { 0 };
		overlappedRead.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		DWORD dwBytesRead = 0;
		if (!overlappedRead.hEvent){
			//bad
			return false;
		}
		overlappedRead.Offset = 0;


		bool readFileSuccessful = ReadFile(file, bufcur, (DWORD)((bufsize - 1) - (bufcur - buf)), NULL, &overlappedRead);
		DWORD dwWaitRes = WaitForSingleObject(overlappedRead.hEvent, INFINITE);
		if (dwWaitRes == WAIT_FAILED){
			//bad
			if (overlappedRead.hEvent){
				CloseHandle(overlappedRead.hEvent);
			}
			return false;
		}
		//is read done? if not, we need to wait for overlapped async read
		if (readFileSuccessful){
			//no action needed
		}
		else{
			if (GetLastError() != ERROR_IO_PENDING){
				if (overlappedRead.hEvent){
					CloseHandle(overlappedRead.hEvent);
				}
				tblog(LOG_WARNING) << "Couldn't read query!";
				return false;
			}
			readFileSuccessful = GetOverlappedResult(file, &overlappedRead, &dwBytesRead, true);
		}

		//async read is over
		if(readFileSuccessful){
			if (toRead == 0){
				toRead = ((UINT64*)buf)[0];
				flowHandle = ((UINT64*)buf)[1];
				tblog() << "Got a query of " << toRead << " bytes, with flow handle " << flowHandle;
			}
			readlen = overlappedRead.InternalHigh - overlappedRead.Offset;
			Read += readlen;
		} else {
			if (overlappedRead.hEvent){
				CloseHandle(overlappedRead.hEvent);
			}
			tblog(LOG_WARNING) << "Couldn't read query!";
			return false;
		}

		if (Read >= toRead) {
			buf = bufcur;
			if (overlappedRead.hEvent){
				CloseHandle(overlappedRead.hEvent);
			}
			break;
		} else if (2 * Read < bufsize) {
			// we need to double our buffer, copy our buf, drop it, and point bufcur to the end of the data
			tblog() << "Doubling the buffer size from " << bufsize << " to " << bufsize * 2 << " for queries";
			bufsize = bufsize * 2;
			bufcur = new UINT8[bufsize];
			memcpy(bufcur, buf, Read);

			delete[] buf;
			buf = bufcur;
			bufcur = buf + Read;
		}
		if (overlappedRead.hEvent){
			CloseHandle(overlappedRead.hEvent);
		}
	}
	
	// parse the message
	Query* query = parse_query(buf, Read);
	if (!query) {
		tblog(LOG_WARNING) << "Could not parse the query";
		return false;
	}

	tblog(LOG_INFO) << "Querying plugins";
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
		tblog() << "Error, could not read in debug query";
	}
	input.close();

	for (int i = 0; i < 3; i++) {
		Query* query = parse_query(buf, size);

		if (!query) {
			tblog() << "Could not parse the debug query";
			return false;
		}

		query->printQueryInfo();

		tblog() << "Enqueing query and querying plugins";

		qq->enqueue_and_link(query); // plugin count + 1 for decider thread
		Sleep(1000);
	}
	return true;
}

void Communications::hex_dump(const UINT8* buf, UINT64 len) {
	const UINT8* c;
	char* dump;
	char* dend;
	char* dc;
	int i;

	dump = new char[len * 3];
	dend = dump + (len * 3);
	dc = dump;

	c = buf;

	i = 0;
	while ((c + 4) < (buf + len)) {
		if (!(i % 4)) {
			dc[0] = '\n';
			dc++;
		}
		i++;


		dc += snprintf(dc, dend - dc, "%02x%02x%02x%02x ", c[0], c[1], c[2], c[3]);
		c += 4;
	}

	while (c < (buf + len)) {
		dc += snprintf(dc, dend - dc, "%02x", c[0]);
		c++;
	}

	tblog() << dump;

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
			tblog(LOG_ERROR) << "Parsing length : buflen=" << buflen << ", cursor=" << (UINT64)(cursor - buffer);
			throw std::runtime_error("");
		}

		flowHandle = *((UINT64*)cursor);
		cursor += sizeof(UINT64);
		if ((UINT64)(cursor - buffer) > buflen) {
			tblog(LOG_ERROR) << "Parsing flowHandle : buflen=" << buflen << ", cursor=" << (UINT64)(cursor - buffer);
			throw std::runtime_error("");
		}

		processId = *((UINT64*)cursor);
		cursor += sizeof(UINT64);
		if ((UINT64)(cursor - buffer) > buflen) {
			tblog(LOG_ERROR) << "Parsing processId: buflen=" << buflen << ", cursor=" << (UINT64)(cursor - buffer);
			throw std::runtime_error("");
		}

		processPathSize = *((UINT32*)cursor);
		cursor += sizeof(UINT32);
		if ((UINT64)(cursor + processPathSize - buffer) > buflen) {
			tblog(LOG_ERROR) << "Parsing path: buflen=" << buflen << ", cursor=" << (UINT64)(cursor - buffer);
			tblog() << "Len  :" << std::hex << length;
			tblog() << "Flow :" << std::hex << flowHandle;
			tblog() << "Pid  :" << std::hex << processId;
			tblog() << "ERR Path Size :" << std::hex << processPathSize;
			hex_dump(buffer, buflen);
			throw std::runtime_error("");
		}
		processPath = new char[processPathSize];
		if (!processPath) {
			tblog(LOG_ERROR) << "Could not allocate " << processPathSize << " bytes while parseing query";
			throw std::bad_alloc();
		}
		memcpy(processPath, cursor, processPathSize);
		cursor += processPathSize;

		clientHelloSize = *((UINT32*)cursor);
		cursor += sizeof(UINT32);
		if ((UINT64)(cursor + clientHelloSize - buffer) > buflen) {
			tblog(LOG_ERROR) << "Parsing client hello: buflen=" << buflen << ", cursor=" << (UINT64)(cursor - buffer) << ", size=" << clientHelloSize;
			throw std::runtime_error("");
		}
		clientHello = new UINT8[clientHelloSize];
		if (!clientHello) {
			tblog(LOG_ERROR) << "Could not allocate " << clientHelloSize << " bytes while parseing query";
			throw std::bad_alloc();
		}
		memcpy(clientHello, cursor, clientHelloSize);
		cursor += clientHelloSize;

		serverHelloSize = *((UINT32*)cursor);
		cursor += sizeof(UINT32);
		if ((UINT64)(cursor + serverHelloSize - buffer) > buflen) {
			tblog(LOG_ERROR) << "Parsing server hello: buflen=" << buflen << ", cursor=" << (UINT64)(cursor - buffer) << ", size=" << serverHelloSize;
			throw std::runtime_error("");
		}
		serverHello = new UINT8[serverHelloSize];
		if (!serverHello) {
			tblog(LOG_ERROR) << "Could not allocate " << serverHelloSize << " bytes while parseing query";
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
			tblog(LOG_ERROR) << "Parsing cert: buflen=" << buflen << ", cursor=" << (UINT64)(cursor - buffer) << ", size=" << certObjectSize;
			throw std::runtime_error("");
		}

		certSize = ntoh24(cursor); 
		cursor += sizeof(UINT8) + sizeof(UINT16);
		if ((UINT64)(cursor + certSize - buffer) > buflen) {
			tblog(LOG_ERROR) << "Parsing cert: buflen=" << buflen << ", cursor=" << (UINT64)(cursor - buffer) << ", size=" << certSize;
			throw std::runtime_error("");
		}

		cert = new UINT8[certSize];
		if (!cert) {
			tblog(LOG_ERROR) << "Could not allocate " << certSize << " bytes while parsing query";
			throw std::bad_alloc();
		}
		memcpy(cert, cursor, certSize);
		cursor += certSize;
	} catch (const std::runtime_error& e) {
		delete[] processPath;
		delete[] cert;
		delete[] serverHello;
		delete[] clientHello;
		tblog(LOG_ERROR) << "Error while parsing Query " << e.what();
		return nullptr;
	} catch (const std::bad_alloc& e) {
		delete[] processPath;
		delete[] cert;
		delete[] serverHello;
		delete[] clientHello;
		tblog(LOG_ERROR) << "Error allocating while parsing Query " << e.what();
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
		tblog() << "STARTING COMMUNICATIONS IN DEBUG MODE";
		return true;
	}

	//So far it only works using Overlapped I/O
	file = CreateFileW(TRUSTBASEKERN, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
	//file = CreateFileW(TRUSTBASEKERN, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (file == NULL ||  file == INVALID_HANDLE_VALUE) {
		tblog(LOG_ERROR) << "Couldn't open trustbase kernel file.";
		tblog(LOG_ERROR) << GetLastError();
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
			tblog(LOG_WARNING) << "Could not handle query";
			
			//WARNING
			// DANGEROUS TEST, USUALLY YOU SHOULD STOP RUNNING OR FIGURE IT OUT IF THAT FAILS
			//success = false;
			// flip keep running to close the Policy Engine
			//keep_running = false;
			
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