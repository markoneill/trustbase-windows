#pragma once
#include <Windows.h>
#include "tchar.h""
#include <cstring>

class TrustBaseService {
public:
	TrustBaseService() {}
	~TrustBaseService() {}

	void install();
	void uninstall();
	static void WINAPI run(TrustBaseService * service);
	
private:
	static void WINAPI SvcCtrlHandler(DWORD dwControl);
	static bool validate(byte* cert);
	static void logEventWrite(char* message);
	SERVICE_STATUS_HANDLE statusHandle;
	SERVICE_STATUS status;
	LPSERVICE_STATUS lp;
};