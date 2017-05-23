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
#include "UnbreakableCrypto.h"
#include "TestUnbreakableCrypto.h"

#define CONFIG_LOCATION		"./trusthub.cfg"
// Things that need to go in the config
#define TIMEOUT_TIME		15000

bool decider_loop(QueryQueue* qq, PolicyContext* context);

int main()
{
	//Uncomment below to test UnbreakableCrypto
	TestUnbreakableCrypto TUBC = TestUnbreakableCrypto();
	return TUBC.Test();

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
	if (!context.initAddons()) {
		thlog() << "Failed to init Addons, exiting...";
		return -1;
	}
	// initialize plugins
	if (!context.initPlugins()) {
		thlog() << "Failed to init Plugins, exiting...";
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
	thlog() << "Starting Decider Thread";
	plugin_threads[context.plugin_count] = std::thread(decider_loop, &qq, &context);
	thlog() << "Starting " << context.plugin_count << " plugins...";
	for (int i = 0; i < context.plugin_count; i++) {
		thlog() << "Starting Plugin #" << i << " : " << context.plugins[i].getName();
		plugin_threads[i] = std::thread(&Plugin::plugin_loop, context.plugins[i]);
	}

	// init things
	if (!Communications::init_communication(&qq, (int)context.plugin_count)) {
		thlog() << "Initialization errors, exiting...";
		return -1;
	}

	// loop
	Communications::listen_for_queries();

	// cleanup threads
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

	thlog() << "Finished, exiting...\n";
    return 0;
}

bool decider_loop(QueryQueue* qq, PolicyContext* context) {
	UnbreakableCrypto UBC = UnbreakableCrypto();
	UBC.configure();
	while (Communications::keep_running) {
		// dequeue a query
		Query* query = qq->dequeue((int)context->plugin_count);
		if (query == nullptr) {
			// Done to wake from lock
			continue;
		}

		thlog() << "Decider dequeued query " << query->getId();
		
		// get system's response
		bool system_response = UBC.evaluate(query);

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
		thlog() << "Handling query "<< query->getId() << " with " << query->num_responses << " responses";
		query->accepting_responses = false;
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

		//Check if we need to trick the system to accept what the plugins say.
		if (response = PLUGIN_RESPONSE_VALID && !system_response) 
		{
			PCCERT_CONTEXT win_cert = CertCreateCertificateContext(
				(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING),
				query->data.raw_chain,
				query->data.raw_chain_len
			);
			UBC.insertIntoRootStore(win_cert);
		}

		// send the response
		Communications::send_response(response, query->getFlow());

		// free that query
		delete query;
	}

	return true;
}