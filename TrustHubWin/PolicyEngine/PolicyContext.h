#pragma once
#include <libconfig.h++>
#include <string>
#include "THLogger.h"
#include "Plugin.h"

class PolicyContext {
public:
	PolicyContext();
	~PolicyContext();

	bool loadConfig(const char* path);
	Plugin* getPlugin(const char* name);
	bool initPlugins();
	void printWelcome();

	// Plugins
	Plugin* plugins;
	size_t plugin_count;
	// Addons
	double congress_threshold;
	
	// Decider queue

	// timeout list
};