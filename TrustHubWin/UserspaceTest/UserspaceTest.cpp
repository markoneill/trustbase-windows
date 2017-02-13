// UserspaceTest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <Windows.h>

#define BUFSIZE 1024

int main(int argc, char* argv[]) {
	HANDLE file;
	char buf[BUFSIZE];
	file = CreateFileW(L"\\\\.\\TrustHub", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	if (ReadFile(file, buf, BUFSIZE - 1, NULL, NULL)) {
		printf("Output: %x @ %x, %s\n", *((UINT64*)buf), buf, buf);
	}
	return 0;
}
