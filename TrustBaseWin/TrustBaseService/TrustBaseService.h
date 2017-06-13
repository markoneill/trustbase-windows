#pragma once
#include <Windows.h>
#include "tchar.h"
#include <cstring>
#include "TrustBaseLogger.h"
#define SERVICE_NAME     "TrustBase"
#define DISPLAY_NAME     "TrustBase Service"
class TrustBaseService {
public:
	TrustBaseService() {}
	~TrustBaseService() {}

	static BOOL startService(TrustBaseService &service);
	
private:
	static void WINAPI run(DWORD dwArgc, LPSTR *pszArgv);

	static TrustBaseService *serviceInstance;
	static void WINAPI SvcCtrlHandler(DWORD dwControl);
	static bool validate(byte * cert);
	SERVICE_STATUS_HANDLE statusHandle;
	SERVICE_STATUS status;
	LPSERVICE_STATUS lp;
};