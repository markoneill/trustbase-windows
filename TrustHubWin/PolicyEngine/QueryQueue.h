#pragma once
#include "Query.h"

class QueryQueue {
public:
	QueryQueue();
	~QueryQueue();

	bool enqueue(Query* query);
	Query* dequeue();
};

