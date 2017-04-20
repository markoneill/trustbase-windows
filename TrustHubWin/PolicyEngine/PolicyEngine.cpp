// PolicyEngine.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <Windows.h>
#include "THLogger.h"
#include "communications.h"
#include "PolicyContext.h"

int main()
{
	// Start Logging

	thlog::setFile("./policy_engine.log", true);
	thlog() << "Starting Policy Engine";

	// load configuration
	PolicyContext context;

	if (!context.loadConfig("C:/Users/ilab/temp/trusthub-linux/policy-engine/trusthub.cfg")) {
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

	return;
	// init things
	if (!Communications::init_communication()) {
		thlog() << "Initialization errors, exiting...";
		return -1;
	}

	// start decider and plugin threads

	// loop
	Communications::listen_for_queries();

	// cleanup
	Communications::cleanup();

    return 0;
}