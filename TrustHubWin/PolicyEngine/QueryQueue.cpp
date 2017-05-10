#include "stdafx.h"
#include "QueryQueue.h"


QueryQueue::QueryQueue() {
}


QueryQueue::~QueryQueue() {
}

bool QueryQueue::enqueue_n_and_link(int n, Query * query) {
	for (int i = 0; i < n; i++) {
		if (!enqueue(query)) {
			return false;
		}
	}
	link(query);
	return true;
}

bool QueryQueue::enqueue(Query * query) {
	std::unique_lock<std::mutex> lck(queue_mux);

	queue.push_front(query);

	queue_hasdata.notify_one();
	lck.unlock();
	return true;
}

Query* QueryQueue::dequeue() {
	std::unique_lock<std::mutex> lck(queue_mux);
	while (queue.empty()) {
		queue_hasdata.wait(lck);
	}
	
	Query* query = queue.back();
	queue.pop_back();
	
	lck.unlock();

	return query;
}

// links the query for async callbacks to find the correct query
bool QueryQueue::link(Query * query) {
	std::unique_lock<std::mutex> lck(list_mux);

	list.push_front(query);

	lck.unlock();

	return true;;
}

// unlink will remove the query from the linked list
Query* QueryQueue::unlink(int id) {
	std::unique_lock<std::mutex> lck(list_mux);

	for (std::deque<Query*>::iterator it = list.begin(); it != list.end(); ++it) {
		if ((*it)->getId() == id) {
			Query* query = (*it);
			list.erase(it);
			lck.unlock();
			return query;
		}
	}
	
	lck.unlock();

	return NULL;
}