#include "stdafx.h"
#include "Query.h"

// Things that need to go in the config
#define DEFAULT_RESPONSE	PLUGIN_RESPONSE_VALID

int Query::next_id;

Query::Query(UINT64 flowHandle, UINT64 processId, char* processPath, UINT8 * raw_certificate, DWORD cert_len, UINT8 * client_hello, DWORD client_hello_len, UINT8 * server_hello, DWORD server_hello_len, int plugin_count) {
	this->flowHandle = flowHandle;
	this->processId = processId;
	this->processPath = processPath;

	data.hostname = NULL;
	data.port = 0;
	data.raw_chain = raw_certificate;
	data.raw_chain_len = cert_len;
	data.client_hello = (char*)client_hello;
	data.client_hello_len = client_hello_len;
	data.server_hello = (char*)server_hello;
	data.server_hello_len = server_hello_len;

	data.id = Query::next_id;
	Query::next_id++;

	//Get Hostname
	data.hostname = SNI_Parser::sni_get_hostname((char*)client_hello, client_hello_len);

	// allocate responses
	this->num_plugins = plugin_count;
	this->num_responses = 0;
	responses = new int[plugin_count];
	for (int i = 0; i < plugin_count; i++) {
		responses[i] = DEFAULT_RESPONSE;
	}
	accepting_responses = true;
}

Query::~Query() {
	delete[] responses;
	delete[] processPath;
	delete[] data.raw_chain;
	delete[] data.server_hello;
	delete[] data.client_hello;
	delete[] data.hostname;
}

void Query::setResponse(int plugin_id, int response) {
	if (accepting_responses) {
		responses[plugin_id] = response;
		num_responses++;
		if (num_responses >= num_plugins) {
			std::unique_lock<std::mutex> lck(mutex);
			threshold_met.notify_all();
			lck.unlock();
		}
	}
	thlog() << "Set response called, " << num_responses << "/" << num_plugins;
}

int Query::getResponse(int plugin_id) {
	return responses[plugin_id];
}

int Query::getId() {
	return data.id;
}

UINT64 Query::getFlow() {
	return flowHandle;
}

void Query::printQueryInfo() {
	thlog() << "Query :";
	thlog() << "\tFlow : " << std::hex << flowHandle;
	std::wstring wpath((wchar_t*)processPath);
	std::string path(wpath.begin(), wpath.end());
	thlog() << "\tPath : " << path;
	thlog() << "\tHostname : " << data.hostname;
}
