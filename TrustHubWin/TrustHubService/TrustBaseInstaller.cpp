#include "TrustBaseInstaller.h"
#include <stdio.h>

//installs a service with all access
void TrustBaseInstaller::install()
{
	char lpFilename[MAX_PATH];
	DWORD copied = GetModuleFileName(NULL, lpFilename, MAX_PATH);
	if(copied == 0)
	{
		printf("GetModuleFileName did not copy anything");
		return;
	}

	SC_HANDLE handle = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (handle == NULL)
	{
		printf("OpenSCManager failed");
		return;
	}

	SC_HANDLE service = CreateService(
		handle,
		SERVICE_NAME,
		DISPLAY_NAME,
		SERVICE_ALL_ACCESS,
		SERVICE_WIN32_OWN_PROCESS,
		SERVICE_AUTO_START,
		SERVICE_ERROR_NORMAL,
		lpFilename,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL
	);

	if (service == NULL)
	{
		printf("CreateService failed");
		CloseServiceHandle(handle);
		handle = NULL;
		return;
	}

	printf("service installed successfully!");
}

//first the function will try to stop the service
//then it will delete it
void TrustBaseInstaller::uninstall()
{
	SERVICE_STATUS_PROCESS serviceStatus;
	DWORD dwBytesNeeded;
	DWORD dwStartTime = GetTickCount();
	DWORD dwTimeout = 5;
	DWORD dwWaitTime;

	SC_HANDLE handle = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (handle == NULL)
	{
		printf("OpenSCManager failed");
		return;
	}

	SC_HANDLE service = OpenService(handle, SERVICE_NAME, SERVICE_STOP |
		SERVICE_QUERY_STATUS | DELETE);

	if (service == NULL)
	{
		printf("OpenService failed");
		CloseServiceHandle(handle);
		handle = NULL;
		return;
	}

	if (!QueryServiceStatusEx(
		service,
		SC_STATUS_PROCESS_INFO,
		(LPBYTE)&serviceStatus,
		sizeof(SERVICE_STATUS_PROCESS),
		&dwBytesNeeded))
	{
		printf("QueryServiceStatusEx failed");
		CloseServiceHandle(handle);
		CloseServiceHandle(service);
		handle = NULL;
		service = NULL;
		return;
	}

	//start stop
	if (serviceStatus.dwCurrentState != SERVICE_STOPPED && serviceStatus.dwCurrentState != SERVICE_STOP_PENDING)
	{
		if (!ControlService(
			service,
			SERVICE_CONTROL_STOP,
			(LPSERVICE_STATUS)&serviceStatus))
		{
			printf("ControlService failed (trying to stop service)");
			CloseServiceHandle(handle);
			CloseServiceHandle(service);
			handle = NULL;
			service = NULL;
			return;
		}
	}

	if (serviceStatus.dwCurrentState == SERVICE_STOPPED)
	{
		printf("Service is has stopped.\n");
	}
	else if(serviceStatus.dwCurrentState == SERVICE_STOP_PENDING)
	{
		while (serviceStatus.dwCurrentState == SERVICE_STOP_PENDING)
		{
			printf("Service stop pending...\n");

			dwWaitTime = serviceStatus.dwWaitHint / 10;

			if (dwWaitTime < 1000)
				dwWaitTime = 1000;
			else if (dwWaitTime > 10000)
				dwWaitTime = 10000;

			Sleep(dwWaitTime);

			if (!QueryServiceStatusEx(
				service,
				SC_STATUS_PROCESS_INFO,
				(LPBYTE)&serviceStatus,
				sizeof(SERVICE_STATUS_PROCESS),
				&dwBytesNeeded))
			{
				printf("QueryServiceStatusEx failed");
				CloseServiceHandle(handle);
				CloseServiceHandle(service);
				handle = NULL;
				service = NULL;
				return;
			}

			if (serviceStatus.dwCurrentState == SERVICE_STOPPED)
			{
				printf("Service stopped successfully.\n");
				break;
			}

			if (GetTickCount() - dwStartTime > dwTimeout)
			{
				printf("Service stop timed out.\n");
				CloseServiceHandle(handle);
				CloseServiceHandle(service);
				handle = NULL;
				service = NULL;
				return;
			}
		}
	}

	if (!DeleteService(service))
	{
		printf("DeleteService failed");
		CloseServiceHandle(handle);
		CloseServiceHandle(service);
		handle = NULL;
		service = NULL;
		return;
	}

	printf("service was removed successfully");
	CloseServiceHandle(handle);
	CloseServiceHandle(service);
	handle = NULL;
	service = NULL;
}