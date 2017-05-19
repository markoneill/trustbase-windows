#include "stdafx.h"
#include "Plugin.h"
//app link include required here in order for plugins to use it openssl functionally correctly
#ifndef APPLINK_
#define APPLINK_
extern "C"
{
#include "openssl/applink.c"
}
#endif //APPLINK
static QueryQueue* qq;

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
	this->id = other.id;
}

Plugin::Plugin(int id, std::string name, std::string path, std::string handler, Plugin::Type type, std::string description, std::string version) {
	this->name = name;
	this->path = path;
	this->handler = handler;
	this->description = description;
	this->version = version;
	this->type = type;
	this->value = Plugin::IGNORED;
	this->id = id;

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

	// resolve function addresses
	query = (query_func_t)GetProcAddress(hDLL, "query");
	if (!query) {
		thlog() << "Could not locate 'query' function for plugin : " << name;
		return false;
	}

	initialize = (initialize_func_t)GetProcAddress(hDLL, "initialize");
	finalize = (finalize_func_t)GetProcAddress(hDLL, "finalize");

	if (initialize) {
		// initialize the plugin
		init_data_t idata;
		idata.callback = Plugin::async_callback;
		idata.plugin_id = this->id;
		idata.plugin_path = this->path.c_str();
		idata.log = thlog::pluginTHLog;
		initialize(&idata);
	}

	return true;
}

bool Plugin::plugin_loop() { //TODO
	// DEBUG
	thlog() << "Started Plugin loop for " << name;

	// loop waiting for queries
	while (Communications::keep_running) {
		// dequeue query
		Query* newquery = qq->dequeue(id);
		if (newquery == nullptr) {
			// done to wake from lock
			continue;
		}
		if (value == Plugin::IGNORED) {
			// respond ok
			newquery->setResponse(id, PLUGIN_RESPONSE_VALID);
		}
		
		thlog() << "Plugin " << id << " got called";

		// TODO set default response for this plugin

		// run the query plugin function
		int response = this->query(&(newquery->data));
		if (type == Plugin::SYNC) {
			// set response
			newquery->setResponse(id, response);
			thlog() << "For query " << newquery->getId() << " Plugin " << id << " synchronously returned " <<
				((response == PLUGIN_RESPONSE_VALID) ? "valid" : ((response == PLUGIN_RESPONSE_INVALID) ? "invalid" : ((response == PLUGIN_RESPONSE_ABSTAIN)?"abstain":"error")));
		}
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

Plugin::Value Plugin::getValue() {
	return value;
}

void Plugin::printInfo() {
	thlog() << "\t Plugin " << id << " {";
	thlog() << "\t\t Name: " << name;
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
	// set response
	Query* foundquery = qq->find_linked(query_id);
	if (foundquery == nullptr) {
		thlog() << "Tried to asynchronously reply to a query that is no longer tracked";
		return 0; // let plugin know it was unsuccessful
	}

	foundquery->setResponse(plugin_id, result);
	thlog() << "For Query " << query_id << " Plugin " << plugin_id << " asynchronously returned " <<
		((result == PLUGIN_RESPONSE_VALID) ? "valid" : ((result == PLUGIN_RESPONSE_INVALID) ? "invalid" : ((result == PLUGIN_RESPONSE_ABSTAIN) ? "abstain" : "error")));

	return 1; // let plugin know the callback was successful
}

void Plugin::set_QueryQueue(QueryQueue* qq_in) {
	qq = qq_in;
}
