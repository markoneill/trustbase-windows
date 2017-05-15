#pragma once
#include <mutex>
#include <deque>
#include <condition_variable>
#include "Query.h"

class QueryQueue {
public:
	QueryQueue(int plugin_count);
	~QueryQueue();

	bool enqueue_and_link(Query* query);
	bool enqueue(Query* query);
	Query* dequeue(int plugin_id);
	bool link(Query* query);
	Query* unlink(int id);
	Query* find_linked(int query_id);

private:
	struct PluginQueue {
		// queue of queries for plugins
		std::mutex queue_mux;
		std::condition_variable queue_hasdata;
		std::deque<Query*> queue;
	};

	PluginQueue* queues;
	int plugin_count;

	// linked list of queries for async response
	std::mutex list_mux;
	std::deque<Query*> list;
};