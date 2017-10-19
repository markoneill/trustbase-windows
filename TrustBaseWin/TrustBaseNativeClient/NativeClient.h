
#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <openssl/x509.h>


#define TRUSTBASE_RESPONSE_ERROR	-1
#define TRUSTBASE_RESPONSE_VALID	1
#define TRUSTBASE_RESPONSE_INVALID	0
#define TRUSTBASE_RESPONSE_ABSTAIN	2 // will never happen
#define TRUSTBASE_RESPONSE_UNKNOWN	3 // Async, not yet returned

typedef int TRUSTBASE_RESPONSE;

// Exports rules are in the .def file

// creates a connection to the Policy Engine
// also initializes the shared event and other needed things
// Returns Non-Zero if successful
BOOL trustbase_connect(void);

// closes the connection to the Policy Engine
// cleans up
// Returns Non-Zero if successful
void trustbase_disconnect(void);

BOOL query_cert(__out TRUSTBASE_RESPONSE* response, __in char* host, __in UINT16 port, __in DWORD chain_len, __in UINT8* chain, __in_opt HANDLE hEvent);

BOOL query_cert_openssl(__out TRUSTBASE_RESPONSE* response, __in char* host, __in UINT16 port, __in STACK_OF(X509)* chain, __in_opt HANDLE hEvent);


// NOTE! Currently Trustbase only supports one query per connection