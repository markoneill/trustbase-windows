#include <windows.h>
#include "tchar.h""
#include "TrustBaseService.h"
#include <cstdio>

void startService() {
	TrustBaseService service;
	service.run(&service);
}

/**
* The main entry point for the service.
* Accepts parameters "install" "uninstall" and "help"
*
*
**/
void __cdecl _tmain(int argc, TCHAR* argv[]) {
	
	if(argc > 1 && lstrcmpi(argv[1], TEXT("help")) == 0) {
		printf("TrustBase Service\r\nAccepts: 'install' 'uninstall' 'help'\r\n");
	}
	else if (argc > 1 && lstrcmpi(argv[1], TEXT("install")) == 0) {
		TrustBaseService service;
		service.install();
		return;
	} else if (argc > 1 && lstrcmpi(argv[1], TEXT("uninstall")) == 0) {
		TrustBaseService service;
		service.uninstall();
		return;
	}
	

	SERVICE_TABLE_ENTRY serviceTableEntry[2];
	serviceTableEntry[0].lpServiceName = "TrustBase";
	serviceTableEntry[0].lpServiceProc = (LPSERVICE_MAIN_FUNCTION) startService;

	serviceTableEntry[1].lpServiceName = NULL;
	serviceTableEntry[1].lpServiceProc = NULL;

	if (!StartServiceCtrlDispatcher(serviceTableEntry)) {
		HANDLE eventLog = OpenEventLog(NULL, TEXT("TrustBaseLog"));
		char * message = "TrustBase Failed";
		char * * messages = & message; 
		ReportEvent(
			eventLog,
			EVENTLOG_ERROR_TYPE,
			NULL,
			NULL,
			NULL,
			1,
			0,
			(LPCSTR *) messages,
			NULL
			);
		CloseEventLog(eventLog);
	}

	return;
}