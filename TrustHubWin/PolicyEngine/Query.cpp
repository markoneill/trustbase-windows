#include "stdafx.h"
#include "Query.h"


Query::Query(UINT64 flowHandle, char* hostname, UINT16 port, UINT8 * raw_certificate, DWORD cert_len, UINT8 * client_hello, DWORD client_hello_len, UINT8 * server_hello, DWORD server_hello_len) {

	data.hostname = hostname;
	data.port = port;
	data.raw_chain = raw_certificate;
	data.raw_chain_len = cert_len;
	data.client_hello = (char*)client_hello;
	data.client_hello_len = client_hello_len;
	data.server_hello = (char*)server_hello;
	data.server_hello_len = server_hello_len;

	data.id = Query::next_id;
	Query::next_id++;
}

Query::~Query() {
}

int Query::getId() {
	return data.id;
}
