#pragma once
#include "Query.h"
#include <Windows.h>
#include <string>
#include <fstream>
#include <atlstr.h>

enum FetchFileName {
VALID_CERT_FILENAME,
INVALID_CERT_FILENAME,
BAD_HOST_CERT_FILENAME,
NULL_HOST_CERT_FILENAME,
MALFORMED_CERT_FILENAME
};



class QueryFetcher {
public:
	Query * fetch(FetchFileName file_selector);
};