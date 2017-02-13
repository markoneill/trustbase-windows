#include <windows.h>
#include "tchar.h""
#include "TrustBaseService.h"
#include "TrustBaseInstaller.h"
#include <cstdio>


/**
* The main entry point for the service.
* Accepts parameters "install" "uninstall" and "help"
*
**/
void __cdecl _tmain(int argc, TCHAR* argv[]) {
	
	
	if(argc > 1 && lstrcmpi(argv[1], TEXT("help")) == 0) {
		printf("TrustBase Service\r\nAccepts: 'install' 'uninstall' 'help'\r\n");
		return;
	}
	else if (argc > 1 && lstrcmpi(argv[1], TEXT("install")) == 0) {
		printf("TrustBase Service installer started\n");

		TrustBaseInstaller installer;
		installer.install();
		return;
	} else if (argc > 1 && lstrcmpi(argv[1], TEXT("uninstall")) == 0) {
		printf("TrustBase Service uninstaller started\n");

		TrustBaseInstaller installer;
		installer.uninstall();
		return;
	}

	//note: the code below cannot be run in from the command line
	//The code below can only be run and will be run when the service starts

	printf("TrustBase Service started\n");

	TrustBaseService service;

	if (!TrustBaseService::startService(service))
	{
		printf("TrustBase Failed2\n");
		DWORD error = GetLastError();
		printf("%lu", error);
		TrustBaseLogger::logEventWrite("TrustBase Failed");
	}

	return;
}