#pragma once
#include <libconfig.h++>
#include <string>
#include "TBLogger.h"
#include "Plugin.h"
#include "Addon.h"

class PolicyContext {
public:
	PolicyContext();
	~PolicyContext();

	bool loadConfig(const char* path);

	bool initAddons();

	Plugin* getPlugin(const char* name);
	bool initPlugins();
	void printWelcome();

	// Plugins
	Plugin* plugins;
	size_t plugin_count;

	// Addons
	Addon* addons;
	size_t addon_count;

	double congress_threshold;
	
	// Decider queue

	// timeout list
};