
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include "NativeClient.h"

#define NATIVE_API_PIPE_NAME	L"\\\\.\\pipe\\TrustBaseNative"
#define MAX_HOSTLEN				256

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

typedef struct native_response_list {
	native_response response;
	struct native_response_list* next;
} native_response_list;

typedef struct response_entry {
	int id;
	TRUSTBASE_RESPONSE* response;
	OVERLAPPED overlapped;
	HANDLE userEvent;
	struct response_entry* next; // singly linked list;
} response_entry;

static DWORD WINAPI alert_thread(void* param);

static DWORD counter = 0;
static HANDLE TrustBasePipe = INVALID_HANDLE_VALUE;
static response_entry* rentry_head = NULL;
static HANDLE list_mux = NULL;
static native_response_list* nresponse_head = NULL;
static HANDLE ListenerThread = NULL;
static HANDLE ReadyEvent = NULL;
static HANDLE DoneEvent = NULL;

BOOL trustbase_connect(void) {
	DWORD pipeMode;

	if (TrustBasePipe != INVALID_HANDLE_VALUE) {
		return FALSE;
	}

	TrustBasePipe = CreateFile(NATIVE_API_PIPE_NAME, (GENERIC_READ|GENERIC_WRITE), 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);

	if (TrustBasePipe == INVALID_HANDLE_VALUE) {
		// here we could try WaitNamedPipe with a time out, in a loop with CreatFile
		// They will set last error for us, I guess.
		goto FAILED_INIT;
	}
	pipeMode = (PIPE_READMODE_MESSAGE | PIPE_WAIT);
	if (!SetNamedPipeHandleState(TrustBasePipe, &pipeMode, NULL, NULL)) {
		goto FAILED_INIT;
	}

	// create the mux for the response entry list
	if (list_mux != NULL) {
		CloseHandle(list_mux);
	}

	list_mux = CreateMutex(NULL, FALSE, NULL);

	if (list_mux == NULL) {
		goto FAILED_INIT;
	}

	// create the event
	if (ReadyEvent != NULL) {
		// I don't know that this should ever happen
		CloseHandle(ReadyEvent);
	}

	ReadyEvent = CreateEvent(
		NULL, // Can't be inherited
		TRUE, // Manual Reset
		FALSE, // start nonsignaled
		NULL); // No name

	if (ReadyEvent == NULL) {
		goto FAILED_INIT;
	}

	if (DoneEvent != NULL) {
		// again, shouldn't happen if you pain connect and disconnect
		CloseHandle(DoneEvent);
	}

	DoneEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	if (DoneEvent == NULL) {
		goto FAILED_INIT;
	}

	// start our listening thread
	ListenerThread = CreateThread(
		NULL, // Can't be inherited by child processes
		0, // default stack size
		alert_thread, // function
		ReadyEvent, // parameter
		0, // run right away
		NULL); // thread id out

	return TRUE;
FAILED_INIT:
	trustbase_disconnect();
	return FALSE;
}

void trustbase_disconnect(void) {
	// Close the Tread
	SetEvent(DoneEvent);
	// wait for the thread to finish
	WaitForSingleObject(ListenerThread, INFINITE);

	if (TrustBasePipe != INVALID_HANDLE_VALUE) {
		CloseHandle(TrustBasePipe);
		TrustBasePipe = INVALID_HANDLE_VALUE;
	}

	if (list_mux != NULL) {
		CloseHandle(list_mux);
		list_mux = NULL;
	}

	if (ReadyEvent != NULL) {
		CloseHandle(ReadyEvent);
		ReadyEvent = NULL;
	}
}

#define CERT_LENGTH_FIELD_SIZE	3

// translates a STACK_OF(X509)* to a raw cert chain, and sends it along
BOOL query_cert_openssl(__out TRUSTBASE_RESPONSE* response, __in char* host, __in UINT16 port, __in STACK_OF(X509)* chain, __in_opt HANDLE hEvent) {
	UINT8* raw_chain;
	DWORD raw_chain_len;
	UINT8* cursor;
	X509* cur_cert;
	int num_certs;
	unsigned int* cert_lengths;
	int i;
	BOOL query_cert_ret;

	num_certs = sk_X509_num(chain);
	raw_chain_len = 0;
	cert_lengths = (unsigned int*)malloc(sizeof(unsigned int) * num_certs);
	for (i = 0; i < num_certs; i++) {
		cur_cert = sk_X509_value(chain, i);
		cert_lengths[i] = i2d_X509(cur_cert, NULL);
		raw_chain_len += cert_lengths[i] + CERT_LENGTH_FIELD_SIZE;
	}

	raw_chain = (UINT8*)malloc(raw_chain_len);
	cursor = raw_chain;

	for (i = 0; i < num_certs; i++) {
		cur_cert = sk_X509_value(chain, i);
		
		// convert the size to a Big Endian 24 bit num
		cursor[0] = (cert_lengths[i] >> 16) & 0xff;
		cursor[1] = (cert_lengths[i] >> 8) & 0xff;
		cursor[2] = cert_lengths[i] & 0xff;
		cursor += CERT_LENGTH_FIELD_SIZE;

		i2d_X509(cur_cert, &cursor);
	}

	query_cert_ret = query_cert(response, host, port, raw_chain_len, raw_chain, hEvent);

	free(raw_chain);
	free(cert_lengths);
	return query_cert_ret;
}

// Sends a query to the Policy Engine
// If trustbase is not connected, it will automatically connect, and disconnect afterward
// If hEvent is NULL, the function will block until it recieves a response
// If hEvent is a an event HANDLE, then the caller wait on that event to know when the response is ready
// Returns Non-Zero if successful
// If the function fails, or is completing asynchronously, the returnvalue is zero, to get extended error information, call the GetLastError function
// Problably Should not be used both Blocking and Async
// GetLastError returns ERROR_IO_PENDING if the result is asynchronous
BOOL query_cert(__out TRUSTBASE_RESPONSE* response, __in char* host, __in UINT16 port, __in DWORD chain_len, __in UINT8* chain, __in_opt HANDLE hEvent) {
	BOOL status = TRUE;
	DWORD reqid;
	UINT8* reqbuf;
	DWORD reqbuf_len;
	native_request* req;
	native_response_list* resp_l;
	native_response_list* resp_l_ptr;
	native_response* resp;
	OVERLAPPED* overlapped;
	response_entry* re;
	response_entry* re_list_pntr;
	HANDLE block_event = NULL;
	BOOL CloseAfter = FALSE;

	if (TrustBasePipe == INVALID_HANDLE_VALUE) {
		if (!trustbase_connect()) {
			status = FALSE;
			CloseAfter = TRUE;
			goto SEND_QUERY_CLOSE;
		}
	}

	// set up the request and associated stuff
	reqbuf_len = chain_len + sizeof(native_request);

	// ready our request
	reqbuf = (UINT8*)HeapAlloc(GetProcessHeap(), 0, reqbuf_len);
	if (reqbuf == NULL) {
		status = FALSE;
		goto SEND_QUERY_CLOSE;
	}

	reqid = counter++;

	req = (native_request*)reqbuf;
	req->chain_len = chain_len;
	req->chain_type = REQUEST_TYPE_RAWCERT; // must be raw chain
	req->id = reqid;
	req->port = port;
	strcpy_s(req->host, MAX_HOSTLEN, host);
	memcpy_s((reqbuf + sizeof(native_request)), chain_len, chain, chain_len);

	// ready our response to be filled
	re = (response_entry*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(response_entry));
	re->id = reqid;
	re->next = NULL;
	re->response = response;
	overlapped = &(re->overlapped);
	overlapped->hEvent = ReadyEvent;
	// add the user's event, or if it is null, make our own we will block on
	if (hEvent == NULL) {
		// we have to create our own to block on later
		block_event = CreateEvent(NULL, TRUE, FALSE, NULL);
		re->userEvent = block_event;
	} else {
		re->userEvent = hEvent;
	}
	
	// add it to the linked list
	// lock the lists
	WaitForSingleObject(list_mux, INFINITE);
	if (rentry_head == NULL) {
		rentry_head = re;
	} else {
		// walk the list
		re_list_pntr = rentry_head;
		while (re_list_pntr->next != NULL) {
			re_list_pntr = re_list_pntr->next;
		}
		re_list_pntr->next = re;
	}
	
	// ready our buffer to catch the response
	resp_l = (native_response_list*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(response_entry));
	resp = &(resp_l->response);
	resp->id = -1;
	resp->reply = TRUSTBASE_RESPONSE_UNKNOWN;

	// add it to the linked list
	if (nresponse_head == NULL) {
		nresponse_head = resp_l;
	} else {
		// walk the list
		resp_l_ptr = nresponse_head;
		while (resp_l_ptr->next != NULL) {
			resp_l_ptr = resp_l_ptr->next;
		}
		resp_l_ptr->next = resp_l;
	}
	ReleaseMutex(list_mux);

	if (!TransactNamedPipe(TrustBasePipe, reqbuf, reqbuf_len, resp, sizeof(native_response), NULL, overlapped)) {
		if (GetLastError() != ERROR_IO_PENDING) {
			status = FALSE;
			goto SEND_QUERY_CLOSE;
		}
	}

	if (block_event) {
		WaitForSingleObject(block_event, INFINITE);
	}

SEND_QUERY_CLOSE:
	// do cleanup
	if (block_event) {
		CloseHandle(block_event);
	}

	if (CloseAfter) {
		trustbase_disconnect();
	}

	return status;
}

static DWORD WINAPI alert_thread(void* param) {
	// Here we wait for a global event meaning we have a response ready
	// When the event goes off, we read the buffer, and find the waiting entry, then hit the event for that entry
	HANDLE events[2] = { DoneEvent, (HANDLE)param };
	DWORD event_i;
	native_response_list* resp_cur;
	native_response_list* prev_resp_cur;
	native_response_list* old_resp_cur;
	response_entry* rentry_cur;
	response_entry* prev_rentry_cur;

	while (TRUE) {
		event_i = WaitForMultipleObjects(2, events, FALSE, INFINITE);
		if (event_i == WAIT_FAILED) {
			//BadBadNotGood
			break;
		}
		event_i -= WAIT_OBJECT_0; // get the index of the event that fired

		// reset the event
		ResetEvent(events[event_i]);
		
		if (event_i == 0) {
			// this is our done event! finish
			break;
		}

		// We have one or more responses somewhere in our native response list
		// lock the lists
		WaitForSingleObject(list_mux, INFINITE);

		// walk the list
		// find all the non-unknown responses (up to the max in our array)
		// For each response found, find its entry in the list, and set the user event
		prev_resp_cur = NULL;
		resp_cur = nresponse_head;

		while (resp_cur != NULL) {
			// check if this has a response
			if (resp_cur->response.reply != TRUSTBASE_RESPONSE_UNKNOWN) { // maybe should double check this, or find a way to lock it?
				// found a response!
				// find the cooresponding response_entry
				prev_rentry_cur = NULL;
				rentry_cur = rentry_head;
				while (rentry_cur != NULL) {
					if (rentry_cur->id == resp_cur->response.id) {
						*(rentry_cur->response) = resp_cur->response.reply;
						SetEvent(rentry_cur->userEvent);
						// clean up this
						//remove from chain first
						if (prev_rentry_cur != NULL) {
							prev_rentry_cur->next = rentry_cur->next;
						} else {
							rentry_head = rentry_cur->next;
						}
						// now free it
						HeapFree(GetProcessHeap(), 0, rentry_cur);
						// they will handle the event
						break;
					} else {
						// keep looking
						prev_rentry_cur = rentry_cur;
						rentry_cur = rentry_cur->next;
					}
				}
				if (rentry_cur == NULL) {
					// We didn't find it!
					// This is a big problem
					// TODO
				}
				// remove this one from the chain resp chain
				if (prev_resp_cur != NULL) {
					prev_resp_cur->next = resp_cur->next;
				} else {
					// this one was the head
					nresponse_head = resp_cur->next;
				}
				// go to the next one
				old_resp_cur = resp_cur;
				resp_cur = resp_cur->next;
				// free the used one
				HeapFree(GetProcessHeap(), 0, old_resp_cur);
			} else {
				// no response for this one yet
				prev_resp_cur = resp_cur;
				resp_cur = resp_cur->next;
			}
		}
		
		// release the mutex
		ReleaseMutex(list_mux);
	}
	return 0;
}

