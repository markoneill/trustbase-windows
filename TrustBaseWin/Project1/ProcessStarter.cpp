#include <windows.h>
#include <stdio.h>
#include <tchar.h>

void _tmain(int argc, TCHAR *argv[])
{
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	DWORD creation_flags = 0;
	

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	if (argc != 3)
	{
		printf("Usage: %s [cmdline] [Process Name] [foreground or background] (-fg, -bg)\n", argv[0]);
		return;
	}

	if (!strcmp(argv[2],"-bg")) {
		creation_flags = CREATE_NO_WINDOW;
	}


	HKEY subKey = NULL;
	HKEY subKey2 = NULL;
	DWORD lpdwDisposition;
	LSTATUS result = RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\Microsoft\\TrustBase\\Pip", 0, KEY_READ | KEY_WRITE, &subKey);
	if (result != ERROR_SUCCESS) {
		printf("Installing python dependencies and the TrustBase Plugin manager\n");
		result = system("..\\install_extra.bat");
		//printf("Result of the batch script was: %d\r\n", result);
		if (result != ERROR_SUCCESS) {
			system("Pause");
			return;
		}
		
		result = RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\Microsoft\\TrustBase", 0, KEY_READ | KEY_WRITE, &subKey);
		if (result != ERROR_SUCCESS) {
			printf("Problem getting registry key \"HKCU\\Software\\Microsoft\\TrustBase\"");
		}
		result = RegCreateKeyEx(subKey, "Pip", 0, NULL, 0,  KEY_READ | KEY_WRITE, NULL, &subKey2, &lpdwDisposition);
		if (result != ERROR_SUCCESS) {
			printf("Problem creating registry key \"HKCU\\Software\\Microsoft\\TrustBase\Pip\"");
		}
		system("Pause");
		return;
	}

	system("net start TrustBaseWin");	//Start the kernel driver
	 
	// Start the policy engine 
	if (!CreateProcess(NULL,   // No module name (use command line)
		argv[1],        // Command line
		NULL,           // Process handle not inheritable
		NULL,           // Thread handle not inheritable
		FALSE,          // Set handle inheritance to FALSE
		creation_flags,    //creation flags
		NULL,           // Use parent's environment block
		NULL,           // Use parent's starting directory 
		&si,            // Pointer to STARTUPINFO structure
		&pi)           // Pointer to PROCESS_INFORMATION structure
		)
	{
		printf("CreateProcess failed (%d).\n", GetLastError());
		system("Pause");
		return;
	}

	// Wait until child process exits.
	//WaitForSingleObject(pi.hProcess, INFINITE);

	// Close process and thread handles. 

	//Wait for like 2 seconds and then check if the process still exists
	Sleep(2000);
	if (WaitForSingleObject(pi.hProcess, 0) != WAIT_TIMEOUT) {
		printf("There was a problem while trying to start the Policy Engine (Error code: %d)\n", GetLastError());
	}

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	system("Pause");
}