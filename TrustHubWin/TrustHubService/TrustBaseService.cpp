#include "TrustBaseService.h"
#include <cstdio>
#include <random>

void TrustBaseService::install() {
	printf("Installing Trustbase. Please Wait. Do not close this terminal. Do not pass GO. Do not collect $200.");
}

void TrustBaseService::uninstall(){
	printf("Installing Trustbase. Please Wait...");
}

VOID WINAPI TrustBaseService::SvcCtrlHandler(DWORD controlCode) {
	switch (controlCode) {
		case SERVICE_CONTROL_CONTINUE:
			break;
		case SERVICE_CONTROL_INTERROGATE:
			break;
		case  SERVICE_CONTROL_NETBINDADD:
			break;
		case  SERVICE_CONTROL_NETBINDDISABLE:
			break;
		case SERVICE_CONTROL_NETBINDREMOVE:
			break;
		case  SERVICE_CONTROL_PARAMCHANGE:
			break;
		case  SERVICE_CONTROL_PAUSE:
			break;
		case  SERVICE_CONTROL_SHUTDOWN:
			break;
		case  SERVICE_CONTROL_STOP:
			break;
		default:
			break;
	}
}

bool TrustBaseService::validate(byte * cert)
{
	//TODO
	return true;
}

void TrustBaseService::logEventWrite(char* message)
{
	HANDLE eventLog = OpenEventLog(NULL, TEXT("TrustBaseLog"));
	char * * messages = &(message);
	ReportEvent(
		eventLog,
		EVENTLOG_ERROR_TYPE,
		NULL,
		NULL,
		NULL,
		1,
		0,
		(LPCSTR *)messages,
		NULL
	);
	CloseEventLog(eventLog);
}

void WINAPI TrustBaseService::run(TrustBaseService * service) {
	

	service->statusHandle = ::RegisterServiceCtrlHandler(
		"TrustBase",
		SvcCtrlHandler
	);

	service->status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	service->status.dwCurrentState = SERVICE_RUNNING;
	service->status.dwControlsAccepted = NULL;
	service->status.dwWin32ExitCode = NO_ERROR;
	service->status.dwServiceSpecificExitCode = NULL;
	service->status.dwWaitHint = 1000;

	HANDLE interceptorHandle = CreateFile(
			"TrustBaseInterceptor",
			(GENERIC_READ | GENERIC_WRITE),
			(FILE_SHARE_READ | FILE_SHARE_WRITE),
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL
		);

	SetServiceStatus(service->statusHandle, &(service->status));
	printf("Running Trustbase...");

	byte readBuffer[16500];
	DWORD count;

	while (1) {
		DeviceIoControl(
			interceptorHandle,
			IOCTL_DISK_REQUEST_DATA,
			NULL,
			NULL,
			readBuffer,
			sizeof(readBuffer),
			&count,
			NULL
		);
		if (validate(&readBuffer[0])) {
			DeviceIoControl(
				interceptorHandle,
				IOCTL_CHANGER_SET_ACCESS,
				"!REJECTED",
				10,
				NULL,
				NULL,
				&count,
				NULL
			);
		}
		else {
			DeviceIoControl(
				interceptorHandle,
				IOCTL_CHANGER_SET_ACCESS,
				"REJECTED!",
				10,
				NULL,
				NULL,
				&count,
				NULL
			);
			logEventWrite("TRUSTBASE has blocked something");
		}

	}
}



