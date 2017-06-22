#pragma once
#include <Windows.h>
#include "tchar.h"
#include <cstring>

class TrustBaseLogger {
public:
	static void logEventWrite(char* message);
};