#include "stdafx.h"
#include "Plugin.h"

Plugin::Plugin() {
}

Plugin::Plugin(const Plugin & other) {
	this->name = other.name;
	this->path = other.path;
	this->handler = other.handler;
	this->description = other.description;
	this->version = other.version;
	this->type = other.type;
	this->value = other.value;

	this->query = other.query;
	this->initialize = other.initialize;
	this->finalize = other.finalize;
}

Plugin::Plugin(std::string name, std::string path, std::string handler, Plugin::Type type, std::string description, std::string version) {
	this->name = name;
	this->path = path;
	this->handler = handler;
	this->description = description;
	this->version = version;
	this->type = type;
	this->value = Plugin::IGNORED;

	this->query = NULL;
	this->initialize = NULL;
	this->finalize = NULL;
}

Plugin::~Plugin() {
}

bool Plugin::init() {
	HINSTANCE hDLL;

	std::wstring wpath = std::wstring(path.begin(), path.end());

	hDLL = LoadLibraryEx(wpath.c_str(), NULL, NULL);

	if (!hDLL) {
		thlog() << "Could not load " << path << " as dynamic library";
		return false;
	}

	// resolve function address
	query = (query_func_t)GetProcAddress(hDLL, "query");
	if (!query) {
		thlog() << "Could not locate 'query' function for plugin : " << name;
		return false;
	}

	return true;
}

bool Plugin::plugin_loop(QueryQueue* qq) { //TODO
	// DEBUG
	thlog() << "Started Plugin loop for " << name;

	// setup

	// initialize the plugin
	if (initialize != NULL) {

	}

	// loop waiting for queries
	while (Communications::keep_running) {
		// dequeue query
		Sleep(200);
		// run the query plugin function

	}
	
	// clean up
	thlog() << "Ending Plugin loop for " << name;

	return true;
}

std::string Plugin::getName() {
	return this->name;
}

void Plugin::setValue(Plugin::Value val) {
	this->value = val;
}

void Plugin::printInfo() {
	thlog() << "\t " << name << " {";
	thlog() << "\t\t Description: " << description;
	thlog() << "\t\t Path: " << path;
	switch (value) {
	case Plugin::CONGRESS:
		thlog() << "\t\t Aggregation Group: Congress";
		break;
	case Plugin::NEEDED:
		thlog() << "\t\t Aggregation Group: Needed";
		break;
	default:
		thlog() << "\t\t Aggregation Group: None";
	}
	thlog() << "\t\t Query Function: 0x" << std::hex << query;
	thlog() << "\t },";
}

int Plugin::async_callback(int plugin_id, int query_id, int result) { //TODO
	// add our response


	return 1; // let plugin know the callback was successful
}
