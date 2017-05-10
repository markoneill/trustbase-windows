#pragma once
#include "trusthub_plugin.h"
#include <Windows.h>
#include <mutex>
#include <condition_variable>


class Query {
public:
	Query(UINT64 flowHandle, char* hostname, UINT16 port, UINT8* raw_certificate, DWORD cert_len, UINT8* client_hello, DWORD client_hello_len, UINT8* server_hello, DWORD server_hello_len);
	~Query();

	std::mutex mutex;
	std::condition_variable threshold_met;
	int num_responses;
	int num_plugins;
	int* responses;

	int getId();

private:
	UINT64 flowHandle;
	query_data_t data;
	static int next_id;
};

