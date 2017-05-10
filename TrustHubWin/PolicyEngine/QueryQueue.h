#pragma once
#include <mutex>
#include <deque>
#include <condition_variable>
#include "Query.h"

class QueryQueue {
public:
	QueryQueue();
	~QueryQueue();

	bool enqueue_n_and_link(int n, Query* query);
	bool enqueue(Query* query);
	Query* dequeue();
	bool link(Query* query);
	Query* unlink(int id);

private:	
	// linked list of queries for async response
	std::mutex list_mux;
	std::deque<Query*> list;
	// queue of queries for plugins
	std::mutex queue_mux;
	std::condition_variable queue_hasdata;
	std::deque<Query*> queue;
};

