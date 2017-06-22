#pragma once
#include <Windows.h>
#include <string>
#include <LibLoaderAPI.h>
#include "TBLogger.h"
#include "trustbase_plugin.h"
class Addon {
public:

	Addon();
	Addon(const Addon& other);
	Addon(int id, std::string name, std::string path, std::string type_handled, std::string description="", std::string version="");
	~Addon();

	bool init(size_t plugin_count, int(*callback)(int, int, int));
	void printInfo();
	int cleanup();
	int loadPlugin(int pluginId, std::string pluginPath, int isAsync);

	std::string getTypeHandled();

	addon_query_plugin getQueryFunction();
	addon_query_plugin getAsyncQueryFunction();
	addon_finalize_plugin getFinalizedFunction();
private:
	int id;
	std::string name;
	std::string path;
	std::string type_handled;
	std::string description;
	std::string version;

	addon_initialize initialize;
	addon_finalize finalize;
	addon_load_plugin load_plugin;
	addon_query_plugin query_plugin;
	addon_async_query_plugin query_plugin_async;
	addon_finalize_plugin finalize_plugin;
};

