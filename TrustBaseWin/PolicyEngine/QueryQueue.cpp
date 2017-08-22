#include "stdafx.h"
#include "QueryQueue.h"

QueryQueue::QueryQueue(int plugin_count) {
	this->plugin_count = plugin_count;
	queues = new PluginQueue[plugin_count + 1];
}

QueryQueue::~QueryQueue() {
	delete[] queues;
}

bool QueryQueue::enqueue_and_link(Query * query) {
	if (!enqueue(query)) {
		return false;
	}
	link(query);
	return true;
}

bool QueryQueue::enqueue(Query * query) {
	for (int i = 0; i <= plugin_count; i++) { // enqueue 1 for the decided thread too
		std::unique_lock<std::mutex> lck(queues[i].queue_mux);

		queues[i].queue.push_front(query);

		queues[i].queue_hasdata.notify_one();
		lck.unlock();
	}
	return true;
}

Query* QueryQueue::dequeue(int plugin_id) {
	std::unique_lock<std::mutex> lck(queues[plugin_id].queue_mux);
	while (queues[plugin_id].queue.empty()) {
		queues[plugin_id].queue_hasdata.wait(lck);
	}
	
	Query* query = queues[plugin_id].queue.back();
	queues[plugin_id].queue.pop_back();
	
	lck.unlock();

	return query;
}

// links the query for async callbacks to find the correct query
bool QueryQueue::link(Query * query) {
	std::unique_lock<std::mutex> lck(list_mux);

	list.push_front(query);

	lck.unlock();
	return true;
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

Query* QueryQueue::find_linked(int query_id) {
	std::unique_lock<std::mutex> lck(list_mux);
	Query* found = nullptr;

	// iterate reverse over our stored queries
	for (std::deque<Query*>::reverse_iterator it = list.rbegin(); it != list.rend(); ++it) {
		if ((*it)->getId() == query_id) {
			found = (*it);
			break;
		}
	}

	lck.unlock();

	return found;
}
