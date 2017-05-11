#include "stdafx.h"
#include "Query.h"

int Query::next_id;

Query::Query(UINT64 flowHandle, UINT64 processId, char* processPath, UINT8 * raw_certificate, DWORD cert_len, UINT8 * client_hello, DWORD client_hello_len, UINT8 * server_hello, DWORD server_hello_len) {
	this->flowHandle = flowHandle;
	this->processId = processId;
	this->processPath = processPath;

	data.hostname = "";
	data.port = 0;
	data.raw_chain = raw_certificate;
	data.raw_chain_len = cert_len;
	data.client_hello = (char*)client_hello;
	data.client_hello_len = client_hello_len;
	data.server_hello = (char*)server_hello;
	data.server_hello_len = server_hello_len;

	data.id = Query::next_id;
	Query::next_id++;

	//TODO get hostname from client_hello
}

Query::~Query() {
}

int Query::getId() {
	return data.id;
}

void Query::printQueryInfo() {
	thlog() << "Query :";
	thlog() << "\tFlow : " << std::hex << flowHandle;
	std::wstring wpath((wchar_t*)processPath);
	std::string path(wpath.begin(), wpath.end());
	thlog() << "\tPath : " << path;
}
