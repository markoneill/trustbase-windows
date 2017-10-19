#include "stdafx.h"
#include "Native.h"

namespace NativeAPI {
	QueryQueue* qq;
	int plugin_count;
}

bool NativeAPI::init_native(QueryQueue* in_qq, int in_plugin_count) {
	plugin_count = in_plugin_count;
	qq = in_qq;
	return true;
}

bool NativeAPI::listen_for_queries() {
	HANDLE hConnectEvent;
	OVERLAPPED oConnect;
	HANDLE hPipe;
	DWORD lasterr;
	DWORD waitval;
	BOOL pendingIO;
	BOOL readstatus;
	native_inst* instance;

	// Create the event for the connect
	hConnectEvent = CreateEvent(NULL, TRUE, TRUE, NULL); // manual reste, initially signaled
	if (hConnectEvent == NULL) {
		tblog() << "Could not create an event for listening to native connections";
		return false;
	}
	oConnect.hEvent = hConnectEvent;

	// do this a lot
	//TODO stop 
	while (1) {
		// create an instance of the named pipe
		hPipe = CreateNamedPipe(NATIVE_API_PIPE_NAME, (PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED), (PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT), PIPE_UNLIMITED_INSTANCES, sizeof(native_response), REQBUFSIZE, NMPWAIT_USE_DEFAULT_WAIT, NULL);
		if (hPipe == INVALID_HANDLE_VALUE) {
			tblog() << "Could not create a instance of the named pipe";
			return false;
		}

		// connect to a client-
		pendingIO = FALSE;
		ConnectNamedPipe(hPipe, &oConnect); // this should return zero, because we are using overlapped
		lasterr = GetLastError();
		switch (lasterr) {
		case ERROR_IO_PENDING:
			pendingIO = TRUE;
			break;
		case ERROR_PIPE_CONNECTED:
			// aleady connected, so we should manually signal the event
			if (SetEvent(hConnectEvent) == FALSE) {
				tblog() << "Couldn't set the event for the native listener, something bad happened";
				return false;
			}
			break;
		default:
			// bad thing happened
			tblog() << "Could not connect pipe, had error " << lasterr;
			return false;
		}

		// now wait on the connect event
		// we use the Ex version to be wakeable to an APC
		waitval = WaitForSingleObjectEx(hConnectEvent, INFINITE, TRUE);

		// if we wake for the completion routine (read or write of a connection), then just loop again
		if (waitval == WAIT_IO_COMPLETION) {
			continue;
		}
		// otherwise we have a new connection, let's set it up

		// if the pipe wasn't already connected, we need to get the result of the thing
		if (pendingIO) {
			if (!GetOverlappedResult(hPipe, &oConnect, &waitval, FALSE)) {
				tblog() << "Couldn't get the result of pending io after event";
				// clean up here
				CloseHandle(hPipe);
				return false;
			}
		}

		// Allocate the instance for a storage
		instance = new native_inst();
		// Allocate the instance inbuf
		instance->inbufsz = REQBUFSIZE;
		instance->inbuf = new UINT8[instance->inbufsz];

		instance->hPipe = hPipe;

		// start asynchronously reading from this pipe
		readstatus = ReadFileEx(hPipe, instance->inbuf, instance->inbufsz, (OVERLAPPED*)instance, readCompletion); // We actually can use the overlapped param as whatever we want to pass to the completion routine

		if (!readstatus) {
			tblog() << "Something happened trying to read from a native client";
			// clean up here
			CloseHandle(hPipe);
			delete instance;
		}
		// loop
	}

	return true;
}

void WINAPI NativeAPI::readCompletion(DWORD err, DWORD bytesread, OVERLAPPED* overlapped) {
	native_inst* instance = (native_inst*)overlapped;
	native_request* req;
	UINT8* buf;
	BOOL readstatus;

	// check the err
	if (err != ERROR_SUCCESS) {
		tblog() << "Error reading native request";
		return;
	}
	
	// check if we need to read more
	if (!GetOverlappedResult(instance->hPipe, &(instance->olp), &(instance->byteRead), FALSE)) {
		if (GetLastError() == ERROR_MORE_DATA) {
			// read more data
			// reallocate
			// triple the buffer size
			instance->inbufsz *= EXPAND_AMOUNT;

			buf = new UINT8[instance->inbufsz];
			
			memcpy(buf, instance->inbuf, bytesread);
			delete[] instance->inbuf;
			instance->inbuf = buf;
			
			// put the cursor where we want to read more
			buf += bytesread;

			// read again
			readstatus = ReadFileEx(instance->hPipe, buf, instance->inbufsz - bytesread, (OVERLAPPED*)instance, readCompletion);
			
			// TODO possibly keep looping if you wanted to do one query per connection
			return;
		} else {
			tblog() << "Error during GetOberlappedResult";
			return;
		}
	}

	// if we have everything, create a query and send it off
	req = (native_request*)instance->inbuf;

	// we need to check the cert len isn't over what we read
	if (req->chain_len > (instance->inbufsz - sizeof(native_request))) {
		tblog() << "Bad request length in native request";
		return;
	}

	//Query* query = new Query(req->id, req.chain_len, hPipe, req.host, req.port, plugin_count);
	Query* query = new Query(req->id, instance->inbuf + sizeof(native_request), req->chain_len, instance->hPipe, req->host, req->port, plugin_count);
	tblog(LOG_INFO) << "Querying plugins for native client";
	// Poll the plugins
	qq->enqueue_and_link(query);

	// somehow keep reading for this connection? I donno
	//TODO modify the native client to just do that
}

bool NativeAPI::send_response(int result, HANDLE hPipe, int id) {
	DWORD bytesout;
	native_response resp;
	resp.id = id;
	resp.reply = result;
	bool success = true;

	// try to send the response on the pipe
	//TODO change this to async? Possibly WriteFileEx
	if (!WriteFile(hPipe, &resp, sizeof(native_response), &bytesout, NULL)) {
		tblog() << "Couldn't write response to native client";
		success = false;
	}

	// We are closing the handle here, because we are only doing a query per connection right now
	CloseHandle(hPipe);

	return success;
}
