// UserspaceTest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <Windows.h>

#define BUFSIZE 4096

int main(int argc, char* argv[]) {
	HANDLE file;
	char buf[BUFSIZE];
	DWORD readlen;
	file = CreateFileW(L"\\\\.\\TrustHub", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	if (ReadFile(file, buf, BUFSIZE - 1, &readlen, NULL)) {
		printf("Got %d bytes:\n", readlen);
		for (int i = 0; i < readlen; i++) {
			printf("%s%02x ", (!(i%16)? "\n": (!(i%8)? "   " : (!(i % 4) ? " " : ""))), (unsigned char)buf[i]);
		}
	} else {
		printf("Couldn't read\n");
	}
	return 0;
}
