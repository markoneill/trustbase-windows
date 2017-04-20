// PolicyEngine.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <Windows.h>
#include "THLogger.h"
#include "communications.h"
#include "PolicyContext.h"

#define CONFIG_LOCATION	"C:/Users/ilab/Source/Repos/TrustKern/TrustHubWin/PolicyEngine/trusthub.cfg"

int main()
{
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

	return 0;
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