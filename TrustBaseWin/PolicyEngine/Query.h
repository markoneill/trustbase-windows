#pragma once
#include "trustbase_plugin.h"
#include <string>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include "TBLogger.h"
#include "SNI_Parser.h"
#include <vector>

#define CERT_LENGTH_FIELD_SIZE	3
class Query {
public:
	Query(UINT64 flowHandle, UINT64 processId, char* processPath, UINT8* raw_certificate, DWORD cert_len, UINT8* client_hello, DWORD client_hello_len, UINT8 * server_hello, DWORD server_hello_len, int plugin_count); // from Kernel
	Query(UINT64 native_id, UINT8* raw_certificate, DWORD cert_len, HANDLE pipe, char* in_hostname, uint16_t in_port, int plugin_count); // from Native Client
	~Query();
	std::mutex mutex;
	std::condition_variable threshold_met;
	int num_responses;
	int num_plugins;
	int* responses;
	std::atomic<bool> accepting_responses;
	query_data_t data;
	HANDLE native_pipe; // if this != INVALID_HANDLE_VALUE, then this query is from a native client, and we will try to send it back their way

	void setResponse(int plugin_id, int response);
	int getResponse(int plugin_id);
	int getId();
	UINT64 getFlow();
	void printQueryInfo();

private:
	UINT64 flowHandle;
	char* processPath;
	UINT64 processId;

	static int next_id;
	static std::mutex id_mux;

	STACK_OF(X509)* parse_chain(unsigned char* data, size_t len);
	std::vector<PCCERT_CONTEXT>* parse_cert_context_chain(UINT8* raw_chain, UINT64 chain_len);
	bool clean_cert_context_chain(std::vector<PCCERT_CONTEXT>*);

	unsigned int ntoh24(const unsigned char* data);
};

