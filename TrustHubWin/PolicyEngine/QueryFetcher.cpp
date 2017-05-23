#include "stdafx.h"
#include "QueryFetcher.h"


//struct {
//	FLOAT32 client_version;
//	Random * random;
//	SessionID session_id;
//	CipherSuite cipher_suites;
//	CompressionMethod compression_methods;
//	select(extensions_present) {
//		case false:
//			struct {};
//		case true:
//			Extension extensions<0..2 ^ 16 - 1>;
//	};
//} ClientHello;

Query * QueryFetcher::fetch(char* address) {
	//HANDLE file;
	//UINT8* buf;
	//UINT8* respbuf;
	//DWORD readlen;
	//DWORD bufsize;
	//UINT64 toRead = 0;
	//UINT64 Read = 0;
	//UINT64 flowHandle = 0;

	//file = CreateFileW(L"\\\\.\\TrustHub", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
	//if (file == NULL) {
	//	printf("Couldn't open file\r\n");
	//	return NULL;
	//}

	//printf("Waiting for query...\r\n");

	//int i = 1;

	//bufsize = 4096;

	//buf = (UINT8*)malloc(bufsize);

	//while (true) {
	//	if (ReadFile(file, buf + Read, bufsize - 1 - Read, &readlen, NULL)) {
	//		printf("Read %d: Got %d bytes:\r\n", i, readlen);
	//		i++;
	//		if (toRead == 0 && readlen > sizeof(UINT64)) {
	//			toRead = ((UINT64*)buf)[0];
	//			printf("First Read, we should read a length of %llu bytes, (%llx):\r\n", toRead, toRead);
	//		}
	//		else if (toRead == 0) {
	//			printf("Serious problem, we didn't even read enough to get the length\r\n");
	//			break;
	//		}
	//		/*for (UINT64 i = 0; i < readlen; i++) {
	//		printf("%s%02x ", (!(i % 16) ? "\r\n" : (!(i % 8) ? "   " : (!(i % 4) ? " " : ""))), buf[i]);
	//		}*/
	//		printf("\r\n\r\n");

	//		Read += readlen;
	//	}
	//	else {
	//		printf("Couldn't read, got readlen %lu\r\n", readlen);
	//		break;
	//	}
	//	if (Read >= toRead) {
	//		printf("Finished, read %llu bytes total\r\n", Read);
	//		break;
	//	}
	//	else {
	//		// double the size
	//		bufsize = bufsize * 2;
	//		buf = (UINT8*)realloc(buf, bufsize);
	//	}
	//}

	//// parse the query
	//UINT8* cursor = buf;

	//printf("Length: %llx\r\n", *((UINT64*)cursor));
	//cursor += sizeof(UINT64);

	//flowHandle = *((UINT64*)cursor);
	//printf("FlowHandle: %llx\r\n", flowHandle);
	//cursor += sizeof(UINT64);

	//printf("Process Id: %llx\r\n", *((UINT64*)cursor));
	//cursor += sizeof(UINT64);

	//DWORD processPathSize = *((UINT32*)cursor);
	//printf("Process Path Size: %lx\r\n", processPathSize);
	//cursor += sizeof(UINT32);

	//for (int i = 0; i < processPathSize; i++) {
	//	printf("%s%02x ", (!(i % 16) ? "\r\n" : (!(i % 8) ? "   " : (!(i % 4) ? " " : ""))), *cursor);
	//	cursor++;
	//}
	//printf("\r\n");

	//DWORD clientHelloSize = *((UINT32*)cursor);
	//printf("Client Hello Size: %lx\r\n", clientHelloSize);
	//cursor += sizeof(UINT32);

	//for (int i = 0; i < clientHelloSize; i++) {
	//	printf("%s%02x ", (!(i % 16) ? "\r\n" : (!(i % 8) ? "   " : (!(i % 4) ? " " : ""))), *cursor);
	//	cursor++;
	//}
	//printf("\r\n");

	//DWORD serverHelloSize = *((UINT32*)cursor);
	//printf("Server Hello Size: %lx\r\n", serverHelloSize);
	//cursor += sizeof(UINT32);

	//for (int i = 0; i < serverHelloSize; i++) {
	//	printf("%s%02x ", (!(i % 16) ? "\r\n" : (!(i % 8) ? "   " : (!(i % 4) ? " " : ""))), *cursor);
	//	cursor++;
	//}
	//printf("\r\n");

	//DWORD dataSize = *((UINT32*)cursor);
	//printf("Certificate Size: %lx\r\n", dataSize);
	//cursor += sizeof(UINT32);

	//for (int i = 0; i < dataSize; i++) {
	//	printf("%s%02x ", (!(i % 16) ? "\r\n" : (!(i % 8) ? "   " : (!(i % 4) ? " " : ""))), *cursor);
	//	cursor++;
	//}
	//printf("\r\n");
/*

	WSADATA wsaData;
	SOCKET Socket;
	SOCKADDR_IN SockAddr;
	int lineCount = 0;
	int rowCount = 0;
	struct hostent * host;
	std::locale local;
	char buffer[10000];
	int i = 0;
	int nDataLength;
	std::string website_HTML;

	// website url
	std::string url = "www.google.com";

	//HTTP GET
	std::string get_http = "GET / HTTP/1.1\r\nHost: " + url + "\r\nConnection: close\r\n\r\n";


	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		std::cout << "WSAStartup failed.\n";
		system("pause");
		//return 1;
	}

	Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	host = gethostbyname(url.c_str());

	SockAddr.sin_port = htons(443);
	SockAddr.sin_family = AF_INET;
	SockAddr.sin_addr.s_addr = *((unsigned long*)host->h_addr);

	if (connect(Socket, (SOCKADDR*)(&SockAddr), sizeof(SockAddr)) != 0) {
		std::cout << "Could not connect";
		system("pause");
		//return 1;
	}

	// send GET / HTTP
	send(Socket, get_http.c_str(), strlen(get_http.c_str()), 0);

	// recieve html
	while ((nDataLength = recv(Socket, buffer, 10000, 0)) > 0) {
		int i = 0;
		while (buffer[i] >= 32 || buffer[i] == '\n' || buffer[i] == '\r') {

			website_HTML += buffer[i];
			i += 1;
		}
	}

	closesocket(Socket);
	WSACleanup();

// Display HTML source 
	std::cout << website_HTML;

// pause
	std::cout << "\n\nPress ANY key to close.\n\n";
	std::cin.ignore(); std::cin.get();


	return new Query(
		0, //FlowHandle
		0, //ProcessID
		"test.exe", //ProcessID 
		UINT8 * raw_certificate, 
		DWORD cert_len, 
		client_hello, 
		sizeof(client_hello), 
		NULL, //Server Hello
		0, //Server Hello Length
		int plugin_count
	);
	*/
return NULL;
}
