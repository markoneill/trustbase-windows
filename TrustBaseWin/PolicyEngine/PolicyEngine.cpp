// PolicyEngine.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"
#include <Windows.h>
#include <thread>
#include <chrono>
#include "TBLogger.h"
#include "communications.h"
#include "Native.h"
#include "PolicyContext.h"
#include "Query.h"
#include "QueryQueue.h"
#include "UnbreakableCrypto.h"
#include "TestUnbreakableCrypto.h"
#include "TBEventLogger.h"

#define CONFIG_LOCATION		"./trustbase.cfg"
// Things that need to go in the config
#define TIMEOUT_TIME		15000

bool decider_loop(QueryQueue* qq, PolicyContext* context);

int main(){
	//Store certificates in root store removal
	UnbreakableCrypto UBC = UnbreakableCrypto();
	UBC.configure();
	UBC.removeAllStoredCertsFromRootStore();

	//Uncomment below to test UnbreakableCrypto

	//TestUnbreakableCrypto TUBC = TestUnbreakableCrypto();
	//return TUBC.Test();

	std::thread* plugin_threads;
	// Start Logging

	tblog::setFile("./policy_engine.log", true);
	tblog(LOG_INFO) << "Starting Policy Engine";

	// load configuration
	PolicyContext context;

	if (!context.loadConfig(CONFIG_LOCATION)) {
		tblog(LOG_ERROR) << "Failed to load configuration file";
		return -1;
	}

	// initialize addons
	if (!context.initAddons()) {
		tblog(LOG_ERROR) << "Failed to init Addons, exiting...";
		return -1;
	}
	// initialize plugins
	if (!context.initPlugins()) {
		tblog(LOG_ERROR) << "Failed to init Plugins, exiting...";
		return -1;
	}

	// Print start message
	context.printWelcome();

	// Get a Query Queue
	QueryQueue qq((int)context.plugin_count);
	// set it as the plugin's queue
	Plugin::set_QueryQueue(&qq);

	// start decider and plugin threads
	plugin_threads = new std::thread[context.plugin_count + 1];
	tblog(LOG_INFO) << "Starting Decider Thread";
	plugin_threads[context.plugin_count] = std::thread(decider_loop, &qq, &context);
	tblog(LOG_INFO) << "Starting " << context.plugin_count << " plugins...";
	for (int i = 0; i < context.plugin_count; i++) {
		tblog(LOG_INFO) << "Starting Plugin #" << i << " : " << context.plugins[i].getName();
		plugin_threads[i] = std::thread(&Plugin::plugin_loop, context.plugins[i]);
	}

	// init things

	//DEBUGGING (TURNED OFF NATIVE API)
	//if (!NativeAPI::init_native(&qq, (int)context.plugin_count)) {
	//	tblog(LOG_ERROR) << "Couldn't initialize native api, exiting...";
	//	return -1;
	//}

	if (!Communications::init_communication(&qq, (int)context.plugin_count)) {
		tblog(LOG_ERROR) << "Communications Initialization errors, exiting...";
		return -1;
	}
	tblog(LOG_INFO) << "Successfully initialized Policy Engine. Ready for queries";

	//DEBUGGING (TURNED OFF NATIVE API)
	// start native api loop
	//std::thread native_api_loop = std::thread(NativeAPI::listen_for_queries);

	// loop
	Communications::listen_for_queries();

	// cleanup threads

	//DEBUGGING (TURNED OFF NATIVE API)
	//	native_api_loop.join();

	// wait on all threads to finish
	for (int i = 0; i <= context.plugin_count; i++) {
		plugin_threads[i].join();
	}
	
	delete[] plugin_threads;

	// cleanup communication
	Communications::cleanup();

	for (int i = 0; i < context.addon_count; i++) {
		context.addons[i].cleanup();
	}
	UBC.removeAllStoredCertsFromRootStore();

	tblog(LOG_INFO) << "Finished, exiting...\n";

	std::system("PAUSE");
    return 0;
}

bool decider_loop(QueryQueue* qq, PolicyContext* context) {
	tbeventlog eventLog;
	bool sent_response = false;

	UnbreakableCrypto UBC = UnbreakableCrypto();
	UnbreakableCrypto_RESPONSE system_response;
	UBC.configure();

	while (Communications::keep_running) {
		// dequeue a query
		Query* query = qq->dequeue((int)context->plugin_count);
		if (query == nullptr) {
			// Done to wake from lock
			continue;
		}

		tblog() << "Decider dequeued query " << query->getId();
		
		// get system's response if it isn't a native request
		if (query->native_pipe == INVALID_HANDLE_VALUE) {
			system_response = UBC.evaluate(query);
			tblog() << "Certificate Evaluate says: " << system_response;
		}

		// get timeout time
		auto now = std::chrono::system_clock::now();
		auto timeout = now + std::chrono::milliseconds(TIMEOUT_TIME);

		// wait for all responses, timing out slow plugins
		std::unique_lock<std::mutex> lk(query->mutex);
		while (query->num_responses < query->num_plugins) {
			if (query->threshold_met.wait_until(lk, timeout) == std::cv_status::timeout) {
				tblog(LOG_WARNING) << "One or more plugins timed out!";
				break;
			}
		}
		query->accepting_responses = false;
		lk.unlock();
		tblog(LOG_INFO) << "Handling query "<< query->getId() << " with " << query->num_responses << " responses";
		// remove the query from the linked list
		qq->unlink(query->getId());

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
			if (context->plugins[i].getValue() == Plugin::CONGRESS) {
				congress_count += 1;
				if (resp == PLUGIN_RESPONSE_VALID) {
					congress_accept += 1.0f;
				}
			}
			// ignore Plugin::IGNORED
		}
		if (response == PLUGIN_RESPONSE_VALID && congress_count > 0) {
			// congress
			if ((congress_accept / (float)congress_count) < context->congress_threshold) {
				response = PLUGIN_RESPONSE_INVALID;
			}
		}

		tblog() << "Want to send response as " << ((response == PLUGIN_RESPONSE_VALID) ?  "valid" : "invalid");


		//Check if we need to trick the system to accept what the plugins say.

		// only if this is not a native query
		if (query->native_pipe == INVALID_HANDLE_VALUE && response == PLUGIN_RESPONSE_VALID && !(system_response==UnbreakableCrypto_ACCEPT)) {
			if (query->data.cert_context_chain->size() <= 0) {
				tblog() << "No PCCERT_CONTEXT in chain";
				return false;
			}
			int leaf_cert_index = 0;
			PCCERT_CONTEXT leaf_cert_context = query->data.cert_context_chain->at(leaf_cert_index);
			if (leaf_cert_context == NULL) {
				tblog() << "cert_context at index " << leaf_cert_index << "is NULL";
				return false;
			}

			bool insertRootSuccess = UBC.insertIntoRootStore(leaf_cert_context);
			tblog() << "insertIntoRootStore returned " << insertRootSuccess;

		}
		// send the response
		if (query->native_pipe == INVALID_HANDLE_VALUE) { // send to the kernel driver
			sent_response = Communications::send_response(response, query->getFlow());
		} else { // send to the proper native client
			sent_response = NativeAPI::send_response(response, query->native_pipe, query->getFlow());
		}

		if (sent_response) {
			tblog() << "Sent the query along";
			//if the response was sent successfully, then log an event
			eventLog.sendEvent(response, query->getProcessPath(), query->data.hostname);
		}

		// free that query
		delete query;
		tblog() << "Finished with query";
	}
	return true;
}