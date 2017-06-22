// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once
//To get WinCrypt to complile
#pragma comment(lib, "Crypt32.lib")
#define CERT_CHAIN_PARA_HAS_EXTRA_FIELDS true

#include "targetver.h"

#include <stdio.h>
#include <tchar.h>
#include <Windows.h>
#include <wincrypt.h>

// TODO: reference additional headers your program requires here
