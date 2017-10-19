#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <openssl/ssl.h>
#include <openssl/x509.h>
#include "NativeClient.h"


// TODO this program needs special permissions to not be intercepted by the kernel piece

SSL* openssl_connect_to_host(int sock, char* hostname);

#define MAX_LENGTH 1024

void print_certificate(X509* cert) {
	char subj[MAX_LENGTH + 1];
	char issuer[MAX_LENGTH + 1];
	X509_NAME_oneline(X509_get_subject_name(cert), subj, MAX_LENGTH);
	X509_NAME_oneline(X509_get_issuer_name(cert), issuer, MAX_LENGTH);
	printf("certificate: %s\n", subj);
	printf("\tissuer: %s\n\n", issuer);
}

void print_stack(STACK_OF(X509)* sk) {
	unsigned len = sk_num(sk);
	unsigned i;
	X509 *cert;
	printf("Begin Certificate Stack:\n");
	for (i = 0; i<len; i++) {
		cert = (X509*)sk_value(sk, i);
		print_certificate(cert);
	}
	printf("End Certificate Stack\n");
}


SSL* openssl_connect_to_host(int sock, char* hostname) {
	SSL_CTX* tls_ctx;
	SSL* tls;

	SSL_library_init();
	OpenSSL_add_all_algorithms();

	tls_ctx = SSL_CTX_new(TLS_client_method());
	if (tls_ctx == NULL) {
		fprintf(stderr, "Could not create SSL_CTX\n");
		exit(-1);
	}


	SSL_CTX_set_verify(tls_ctx, SSL_VERIFY_NONE, NULL);

	tls = SSL_new(tls_ctx);
	if (tls == NULL) {
		fprintf(stderr, "Could not create SSL\n");
		exit(-1);
	}

	SSL_set_tlsext_host_name(tls, hostname);

	SSL_set_fd(tls, sock);

	if (SSL_connect(tls) != 1) {
		fprintf(stderr, "Could not SSL_connect\n");
		exit(-1);
	}

	printf("Done\n");
	return tls;
}

#define HOST	"xkcd.com"
#define PORT_STR	"443"
#define PORT		443
int main(int argc, char** argv) {
	SOCKET sock;
	struct addrinfo* addr_list;
	struct addrinfo hints;
	WSADATA wsaData;
	SSL* tls;
	STACK_OF(X509)* sk;
	TRUSTBASE_RESPONSE tb_response;

	// Initialize Winsock
	if (WSAStartup(MAKEWORD(2, 2), &wsaData)) {
		printf("Initialization failed\n");
		exit(-1);
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	if (getaddrinfo(HOST, PORT_STR, &hints, &addr_list)) {
		printf("getaddrinfo failed\n");
		exit(-1);
	}

	// we should loop over addr_list, but whatever
	// make connection
	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock == INVALID_SOCKET) {
		printf("Socket failed with error %d\n", WSAGetLastError());
		exit(-1);
	}

	if (connect(sock, addr_list->ai_addr, (int)addr_list->ai_addrlen) == SOCKET_ERROR) {
		printf("Connection failed\n");
	} else {
		// Here we wrap the socket!
		tls = openssl_connect_to_host((int)sock, HOST);

		sk = SSL_get_peer_cert_chain(tls);
		//print_stack(sk);

		// do the native client test
		if (!query_cert_openssl(&tb_response, HOST, PORT, sk, NULL)) {
			printf("Things went badly\n");
		} else {
			printf("TrustBase responded positivly\n");
		}

		// Let's do a quick get
		/*len = sprintf_s(query, 2048, "GET / HTTP/1.1\r\nHost: %s\r\n\r\n", HOST);
		SSL_write(tls, query, len);
		SSL_read(tls, response, sizeof(response));

		printf("Received Response:\n%s\n", response);*/

		SSL_shutdown(tls);
		SSL_free(tls);
	}

	freeaddrinfo(addr_list);

	if (closesocket(sock) == SOCKET_ERROR) {
		printf("Couldn't close socket successfully\n");
	}
	WSACleanup();

	return 0;
}