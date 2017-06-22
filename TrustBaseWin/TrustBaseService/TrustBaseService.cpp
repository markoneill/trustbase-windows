#include "TrustBaseService.h"
#include <stdio.h>
#include <windows.h>
#include <cstdio>
#include <random>

TrustBaseService *TrustBaseService::serviceInstance = NULL;

BOOL TrustBaseService::startService(TrustBaseService &service) {
	SERVICE_TABLE_ENTRY serviceTable[] =
	{
		{ SERVICE_NAME, run },
		{ NULL, NULL }
	};

	serviceInstance = &service;
	return StartServiceCtrlDispatcher(serviceTable);
}

VOID WINAPI TrustBaseService::SvcCtrlHandler(DWORD controlCode) {
	switch (controlCode) {
		case SERVICE_CONTROL_CONTINUE:
			serviceInstance->status.dwCurrentState = SERVICE_CONTINUE_PENDING;
			SetServiceStatus(serviceInstance->statusHandle, &(serviceInstance->status));

			serviceInstance->status.dwCurrentState = SERVICE_RUNNING;
			SetServiceStatus(serviceInstance->statusHandle, &(serviceInstance->status));
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
			serviceInstance->status.dwCurrentState = SERVICE_PAUSE_PENDING;
			SetServiceStatus(serviceInstance->statusHandle, &(serviceInstance->status));

			serviceInstance->status.dwCurrentState = SERVICE_PAUSED;
			SetServiceStatus(serviceInstance->statusHandle, &(serviceInstance->status));
			break;
		case  SERVICE_CONTROL_SHUTDOWN:
			serviceInstance->status.dwCurrentState = SERVICE_STOPPED;
			SetServiceStatus(serviceInstance->statusHandle, &(serviceInstance->status));
			break;
		case  SERVICE_CONTROL_STOP:
			serviceInstance->status.dwCurrentState = SERVICE_STOP_PENDING;
			SetServiceStatus(serviceInstance->statusHandle, &(serviceInstance->status));

			serviceInstance->status.dwCurrentState = SERVICE_STOPPED;
			SetServiceStatus(serviceInstance->statusHandle, &(serviceInstance->status));
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



void WINAPI TrustBaseService::run(DWORD dwArgc, LPSTR *pszArgv) {
	serviceInstance->statusHandle = ::RegisterServiceCtrlHandler(
		"TrustBase",
		SvcCtrlHandler
	);
	serviceInstance->status.dwCurrentState = SERVICE_START_PENDING;

	serviceInstance->status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	serviceInstance->status.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN | SERVICE_ACCEPT_PAUSE_CONTINUE;
	serviceInstance->status.dwWin32ExitCode = NO_ERROR;
	serviceInstance->status.dwServiceSpecificExitCode = NULL;
	serviceInstance->status.dwWaitHint = 1000;

	SetServiceStatus(serviceInstance->statusHandle, &(serviceInstance->status));
	
	serviceInstance->status.dwCurrentState = SERVICE_RUNNING;
	SetServiceStatus(serviceInstance->statusHandle, &(serviceInstance->status));

	printf("Running Trustbase...");

	//todo: uncomment lines below (they are untested I think) and comm with the driver
	
	/*
	HANDLE interceptorHandle = CreateFile(
			L"TrustBaseInterceptor",
			(GENERIC_READ | GENERIC_WRITE),
			(FILE_SHARE_READ | FILE_SHARE_WRITE),
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL
		);

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
			TrustBaseLogger::logEventWrite("TRUSTBASE has blocked something");
		}

	}*/
}



