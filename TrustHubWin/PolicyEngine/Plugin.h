#pragma once
#include <Windows.h>
#include <string>
#include <LibLoaderAPI.h>
#include "THLogger.h"
#include "trusthub_plugin.h"

class Plugin {
public:
	enum Value {NEEDED, CONGRESS, IGNORED};
	enum Type {SYNC, ASYNC};

	Plugin();
	Plugin(const Plugin& other);
	Plugin(std::string name, std::string path, std::string handler, Plugin::Type type=Plugin::SYNC, std::string description="", std::string version="");
	~Plugin();

	bool init();

	std::string getName();
	void setValue(Plugin::Value val);
	void printInfo();

private:
	std::string name;
	std::string path;
	std::string handler;
	std::string description;
	std::string version;
	Plugin::Type type;
	Plugin::Value value;

	query_func_t query;

};

