#pragma once
#include <Windows.h>
#include <string>
#include <LibLoaderAPI.h>
#include "THLogger.h"
#include "trusthub_plugin.h"
#include "communications.h"
#include "QueryQueue.h"
#include "Query.h"

class Plugin {
public:
	enum Value {NEEDED, CONGRESS, IGNORED};
	enum Type {SYNC, ASYNC};

	Plugin();
	Plugin(const Plugin& other);
	Plugin(int id, std::string name, std::string path, std::string handler, Plugin::Type type=Plugin::SYNC, std::string description="", std::string version="");
	~Plugin();

	bool init();
	bool plugin_loop(QueryQueue* qq);

	std::string getName();
	void setValue(Plugin::Value val);
	Plugin::Value getValue();
	void printInfo();

	static int async_callback(int plugin_id, int query_id, int result);

private:
	std::string name;
	std::string path;
	std::string handler;
	std::string description;
	std::string version;
	Plugin::Type type;
	Plugin::Value value;

	query_func_t query;
	initialize_func_t initialize;
	finalize_func_t finalize;

	int id;
};

