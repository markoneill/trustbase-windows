#pragma once
#include <Windows.h>
#include <string>
#include <LibLoaderAPI.h>
#include "THLogger.h"
#include "trusthub_plugin.h"

class Plugin {
public:
	enum Value {NEEDED, CONGRESS, IGNORED};

	Plugin();
	Plugin(const Plugin& other);
	Plugin(std::string name, std::string path, std::string handler, std::string description="", std::string version="");
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

	Plugin::Value value;

	query_func_t query;

};

