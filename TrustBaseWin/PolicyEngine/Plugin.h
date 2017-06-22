#pragma once
#include <Windows.h>
#include <string>
#include <LibLoaderAPI.h>
#include "TBLogger.h"
#include "trustbase_plugin.h"
#include "communications.h"
#include "QueryQueue.h"
#include "Query.h"
#include "Addon.h"

class Plugin {
public:
	enum Value {NEEDED, CONGRESS, IGNORED};
	enum Type {SYNC, ASYNC};
	enum HandlerType { UNKNOWN, OPENSSL, RAW, ADDON};

	Plugin();
	Plugin(const Plugin& other);
	Plugin(int id, std::string name, std::string path, std::string handler, Plugin::Type type=Plugin::SYNC, Plugin::HandlerType handlerType=Plugin::UNKNOWN, std::string description="", std::string version="");
	~Plugin();

	bool init(Addon* addons, size_t addon_count);
	bool init_native();
	bool init_addon(Addon* addons, size_t addon_count);

	bool plugin_loop();

	std::string getName();
	void setValue(Plugin::Value val);
	Plugin::Value getValue();
	void printInfo();

	static int async_callback(int plugin_id, int query_id, int result);
	static void set_QueryQueue(QueryQueue* qq);

private:
	std::string name;
	std::string path;
	std::string handler;
	std::string description;
	std::string version;
	Plugin::Type type;
	Plugin::Value value;
	Plugin::HandlerType handlerType;

	query_func_t query;
	initialize_func_t initialize;
	finalize_func_t finalize;

	addon_query_plugin query_by_addon;
	addon_finalize_plugin finalize_by_addon;

	int id;

	static QueryQueue* qq_in;
};

