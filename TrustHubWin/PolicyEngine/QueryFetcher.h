#pragma once
#include "Query.h"
#include <Windows.h>
#include <malloc.h>
#include <fstream>
#include <locale>
#include <string>
//#include <WinSock2.h>
//#include <WS2tcpip.h>

class QueryFetcher {
public:
	Query * fetch(char* address);
};