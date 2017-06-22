#pragma once
#include "Query.h"
#include <Windows.h>
#include <string>
#include <fstream>
#include <atlstr.h>
#include "RawCertData.h"
enum FetchFileName {
VALID_CERT_FILENAME,
INVALID_CERT_FILENAME,
BAD_HOST_CERT_FILENAME,
NULL_HOST_CERT_FILENAME,
MALFORMED_CERT_FILENAME,
SELF_SIGNED_CERT_FILENAME
};



class QueryFetcher {
public:
	RawCertData * fetch(FetchFileName file_selector);
};