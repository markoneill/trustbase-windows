#pragma once
#include "trusthub_plugin.h"
#include <Windows.h>
#include <string>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include "THLogger.h"
#include "SNI_Parser.h"


class Query {
public:
	Query(UINT64 flowHandle, UINT64 processId, char* processPath, UINT8 * raw_certificate, DWORD cert_len, UINT8 * client_hello, DWORD client_hello_len, UINT8 * server_hello, DWORD server_hello_len, int plugin_count);
	~Query();


	std::mutex mutex;
	std::condition_variable threshold_met;
	int num_responses;
	int num_plugins;
	int* responses;
	std::atomic<bool> accepting_responses;
	query_data_t data;

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
};

