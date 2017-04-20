// UserspaceTest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <Windows.h>
#include <malloc.h>

int main(int argc, char* argv[]) {
	HANDLE file;
	UINT8* buf;
	UINT8* respbuf;
	DWORD readlen;
	DWORD bufsize;
	UINT64 toRead = 0;
	UINT64 Read = 0;
	UINT64 flowHandle = 0;
	int i = 1;

	bufsize = 4096;

	buf = (UINT8*)malloc(bufsize);

	file = CreateFileW(L"\\\\.\\TrustHub", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
	if (file == NULL) {
		printf("Couldn't open file\n");
		return -1;
	}

	while (true) {
		if (ReadFile(file, buf, bufsize - 1, &readlen, NULL)) {
			printf("Read %d: Got %d bytes:\n", i, readlen);
			i++;
			if (toRead == 0 && readlen > sizeof(UINT64)) {
				toRead = ((UINT64*)buf)[0];
				printf("First Read, we should read a length of %llu bytes, (%llx):\n", toRead, toRead);
				flowHandle = ((UINT64*)buf)[1];
				printf("First Read, we see a flowHandle of %x:\n", flowHandle);
			} else if (toRead == 0) {
				printf("Serious problem, we didn't even read enough to get the length\n");
				break;
			}
			for (UINT64 i = 0; i < readlen; i++) {
				printf("%s%02x ", (!(i % 16) ? "\n" : (!(i % 8) ? "   " : (!(i % 4) ? " " : ""))), buf[i]);
			}
			printf("\n\n");

			Read += readlen;
		} else {
			printf("Couldn't read, got readlen %lu\n", readlen);
			break;
		}
		if (Read >= toRead) {
			printf("Finished, read %llu bytes total\n", Read);
			break;
		}
	}

	// do a write
	respbuf = (UINT8*)malloc(sizeof(UINT64) + sizeof(UINT8));
	((UINT64*)respbuf)[0] = flowHandle;
	((UINT8*)respbuf)[sizeof(UINT64)] = 1;
	if (WriteFile(file, respbuf, 0x10, NULL, NULL)) {
		printf("Wrote\n");
	}

	free(respbuf);
	free(buf);

	return 0;
}
