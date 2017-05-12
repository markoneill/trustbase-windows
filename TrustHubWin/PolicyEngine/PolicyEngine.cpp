// PolicyEngine.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <Windows.h>
#include <thread>
#include <chrono>
#include "THLogger.h"
#include "communications.h"
#include "PolicyContext.h"
#include "Query.h"
#include "QueryQueue.h"

#define CONFIG_LOCATION		"C:/Users/ilab/Source/Repos/TrustKern/TrustHubWin/PolicyEngine/trusthub.cfg"
// Things that need to go in the config
#define TIMEOUT_TIME		1000

bool decider_loop(QueryQueue* qq, PolicyContext* context);

int main()
{
	std::thread* plugin_threads;
	// Start Logging

	thlog::setFile("./policy_engine.log", true);
	thlog() << "Starting Policy Engine";

	// load configuration
	PolicyContext context;

	if (!context.loadConfig(CONFIG_LOCATION)) {
		thlog() << "Failed to load configuration file";
		return -1;
	}

	// initialize addons

	// initialize plugins
	if (!context.initPlugins()) {
		thlog() << "Failed to init Plugins, exiting...";
		return -1;
	}

	// Print start message
	context.printWelcome();

	// Get a Query Queue
	QueryQueue qq((int)context.plugin_count);

	// start decider and plugin threads
	plugin_threads = new std::thread[context.plugin_count + 1];
	plugin_threads[context.plugin_count] = std::thread(decider_loop, &qq, &context);
	thlog() << "Starting " << context.plugin_count << " plugins";
	for (int i = 0; i < context.plugin_count; i++) {
		thlog() << "Starting #" << i << " " << context.plugins[i].getName();
		plugin_threads[i] = std::thread(&Plugin::plugin_loop, context.plugins[i], &qq);
	}

	// init things
	if (!Communications::init_communication(&qq, (int)context.plugin_count)) {
	thlog() << "Initialization errors, exiting...";
	return -1;
	}

	// loop
	Communications::listen_for_queries();

	// cleanup threads
	//TODO

	// wait on all threads to finish
	for (int i = 0; i <= context.plugin_count; i++) {
		plugin_threads[i].join();
	}

	// cleanup communication
	Communications::cleanup();

	thlog() << "Finished, exiting...\n";
    return 0;
}

bool decider_loop(QueryQueue* qq, PolicyContext* context) {
	while (Communications::keep_running) {
		// dequeue a query
		Query* query = qq->dequeue((int)context->plugin_count);

		thlog() << "Decider dequeued query " << query->getId();
		
		// get system's response
		//TODO

		// get timeout time
		auto now = std::chrono::system_clock::now();
		auto timeout = now + std::chrono::milliseconds(TIMEOUT_TIME);

		// wait for all responses, timing out slow plugins
		std::unique_lock<std::mutex> lk(query->mutex);
		while (query->num_responses < query->num_plugins) {
			if (query->threshold_met.wait_until(lk, timeout) == std::cv_status::timeout) {
				thlog() << "One or more plugins timed out!";
			}
		}
		lk.unlock();
		query->accepting_responses = false;

		// aggregate the responses
		int response = PLUGIN_RESPONSE_VALID;
		int congress_count = 0;
		float congress_accept = 0.0f;
		for (int i = 0; i < context->plugin_count; i++) {
			int resp = query->getResponse(i);
			if (context->plugins[i].getValue() == Plugin::NEEDED && resp == PLUGIN_RESPONSE_INVALID) {
				response = PLUGIN_RESPONSE_INVALID;
				break;
			}
			
		}
		if (response == PLUGIN_RESPONSE_VALID && congress_count > 0) {
			// congress
			if ((congress_accept / (float)congress_count) < context->congress_threshold) {
				response = PLUGIN_RESPONSE_INVALID;
			}
		}

		// send the response
		Communications::send_response(response, query->getFlow());
	}

	return true;
}