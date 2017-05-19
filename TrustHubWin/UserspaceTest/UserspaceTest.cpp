// UserspaceTest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <Windows.h>
#include <malloc.h>
#include <fstream>

int main(int argc, char* argv[]) {
	HANDLE file;
	UINT8* buf;
	UINT8* respbuf;
	DWORD readlen;
	DWORD bufsize;
	UINT64 toRead = 0;
	UINT64 Read = 0;
	UINT64 flowHandle = 0;

	file = CreateFileW(L"\\\\.\\TrustHub", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
	if (file == NULL) {
		printf("Couldn't open file\r\n");
		return -1;
	}

	printf("Waiting for query...\r\n");

	int i = 1;

	bufsize = 4096;

	buf = (UINT8*)malloc(bufsize);

	while (true) {
		if (ReadFile(file, buf + Read, bufsize - 1 - Read, &readlen, NULL)) {
			printf("Read %d: Got %d bytes:\r\n", i, readlen);
			i++;
			if (toRead == 0 && readlen > sizeof(UINT64)) {
				toRead = ((UINT64*)buf)[0];
				printf("First Read, we should read a length of %llu bytes, (%llx):\r\n", toRead, toRead);
			} else if (toRead == 0) {
				printf("Serious problem, we didn't even read enough to get the length\r\n");
				break;
			}
			/*for (UINT64 i = 0; i < readlen; i++) {
				printf("%s%02x ", (!(i % 16) ? "\r\n" : (!(i % 8) ? "   " : (!(i % 4) ? " " : ""))), buf[i]);
			}*/
			printf("\r\n\r\n");

			Read += readlen;
		} else {
			printf("Couldn't read, got readlen %lu\r\n", readlen);
			break;
		}
		if (Read >= toRead) {
			printf("Finished, read %llu bytes total\r\n", Read);
			break;
		} else {
			// double the size
			bufsize = bufsize * 2;
			buf = (UINT8*)realloc(buf, bufsize);
		}
	}

	// parse the query
	UINT8* cursor = buf;

	printf("Length: %llx\r\n", *((UINT64*)cursor));
	cursor += sizeof(UINT64);

	flowHandle = *((UINT64*)cursor);
	printf("FlowHandle: %llx\r\n", flowHandle);
	cursor += sizeof(UINT64);

	printf("Process Id: %llx\r\n", *((UINT64*)cursor));
	cursor += sizeof(UINT64);

	DWORD processPathSize = *((UINT32*)cursor);
	printf("Process Path Size: %lx\r\n", processPathSize);
	cursor += sizeof(UINT32);

	for (int i = 0; i < processPathSize; i++) {
		printf("%s%02x ", (!(i % 16) ? "\r\n" : (!(i % 8) ? "   " : (!(i % 4) ? " " : ""))), *cursor);
		cursor++;
	}
	printf("\r\n");

	DWORD clientHelloSize = *((UINT32*)cursor);
	printf("Client Hello Size: %lx\r\n", clientHelloSize);
	cursor += sizeof(UINT32);

	for (int i = 0; i < clientHelloSize; i++) {
		printf("%s%02x ", (!(i % 16) ? "\r\n" : (!(i % 8) ? "   " : (!(i % 4) ? " " : ""))), *cursor);
		cursor++;
	}
	printf("\r\n");

	DWORD serverHelloSize = *((UINT32*)cursor);
	printf("Server Hello Size: %lx\r\n", serverHelloSize);
	cursor += sizeof(UINT32);

	for (int i = 0; i < serverHelloSize; i++) {
		printf("%s%02x ", (!(i % 16) ? "\r\n" : (!(i % 8) ? "   " : (!(i % 4) ? " " : ""))), *cursor);
		cursor++;
	}
	printf("\r\n");

	DWORD dataSize = *((UINT32*)cursor);
	printf("Certificate Size: %lx\r\n", dataSize);
	cursor += sizeof(UINT32);

	for (int i = 0; i < dataSize; i++) {
		printf("%s%02x ", (!(i % 16) ? "\r\n" : (!(i % 8) ? "   " : (!(i % 4) ? " " : ""))), *cursor);
		cursor++;
	}
	printf("\r\n");


	if (true) {
		// save buffer
		char* filename = "example_query.bin";

		std::ofstream(filename, std::ios::binary).write((const char*)buf, Read);

		printf("Saved query to file : %s\r\n", filename);
	}

	printf("Respond with succcess?\r\n>>(y/n)");
	char choice;
	while ((choice = getchar()) == '\n');

	if (choice == 'n') {
		printf("\r\nResponding to request as invalid\r\n");
	} else {
		printf("\r\nResponding to request as valid\r\n");
	}

	// do a write
	respbuf = (UINT8*)malloc(sizeof(UINT64) + sizeof(UINT8));
	((UINT64*)respbuf)[0] = flowHandle;
	((UINT8*)respbuf)[sizeof(UINT64)] = ((choice == 'n') ? 0 : 1);
	if (WriteFile(file, respbuf, 0x10, NULL, NULL)) {
		printf("Wrote Response %s\r\n", ((choice == 'n') ? "invalid" : "valid"));
	}

	free(respbuf);
	free(buf);

	return 0;
}
