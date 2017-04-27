#pragma once
#include "trusthub_plugin.h"
#include <Windows.h>
#include <mutex>
#include <condition_variable>


class Query {
public:
	Query();
	~Query();

	std::mutex mutex;
	std::condition_variable threshold_met;
	int num_responses;
	int num_plugins;
	int* responses;

private:
	UINT64 flowHandle;
	query_data_t data;
};

