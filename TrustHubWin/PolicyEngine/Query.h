#pragma once
#include "trusthub_plugin.h"
#include <Windows.h>
#include <string>
#include <mutex>
#include <condition_variable>
#include "THLogger.h"


class Query {
public:
	Query(UINT64 flowHandle, UINT64 processId, char* processPath, UINT8 * raw_certificate, DWORD cert_len, UINT8 * client_hello, DWORD client_hello_len, UINT8 * server_hello, DWORD server_hello_len);
	~Query();


	std::mutex mutex;
	std::condition_variable threshold_met;
	int num_responses;
	int num_plugins;
	int* responses;

	int getId();
	void printQueryInfo();

private:
	UINT64 flowHandle;
	query_data_t data;
	char* processPath;
	UINT64 processId;
	static int next_id;
};

