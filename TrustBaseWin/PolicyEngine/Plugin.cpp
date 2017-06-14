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
	this->handlerType = other.handlerType;

	this->query = other.query;
	this->initialize = other.initialize;
	this->finalize = other.finalize;

	this->query_by_addon = other.query_by_addon;
	this->finalize_by_addon = other.finalize_by_addon;

	this->id = other.id;
}

Plugin::Plugin(int id, std::string name, std::string path, std::string handler, Plugin::Type type, Plugin::HandlerType handlerType, std::string description, std::string version) {
	this->name = name;
	this->path = path;
	this->handler = handler;
	this->description = description;
	this->version = version;
	this->type = type;
	this->value = Plugin::IGNORED;
	this->handlerType = handlerType;
	this->id = id;

	this->query = NULL;
	this->initialize = NULL;
	this->finalize = NULL;

	this->query_by_addon = NULL;
	this->finalize_by_addon = NULL;
}

Plugin::~Plugin() {
}

bool Plugin::init(Addon* addons, size_t addon_count) {

	if (this->handlerType == RAW || this->handlerType == OPENSSL) {
		return init_native();
	}
	else{
		return init_addon(addons, addon_count);
	}
}

bool Plugin::init_native() {
	HINSTANCE hDLL;

	std::wstring wpath = std::wstring(path.begin(), path.end());

	hDLL = LoadLibraryEx(wpath.c_str(), NULL, NULL);

	if (!hDLL) {
		tblog() << "Could not load " << path << " as dynamic library";
		return false;
	}

	// resolve function addresses
	query = (query_func_t)GetProcAddress(hDLL, "query");
	if (!query) {
		tblog() << "Could not locate 'query' function for plugin : " << name;
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
		idata.log = tblog::pluginTBLog;
		initialize(&idata);
	}

	return true;
}
bool Plugin::init_addon(Addon* addons, size_t addon_count) {

	//find correct Addon
	Addon* correctAddon = NULL;
	for (int i = 0; i < addon_count; i++) {
		if (this->handler.compare(addons[i].getTypeHandled()) == 0) {
			correctAddon = &addons[i];
			break;
		}
	}
	if (correctAddon == NULL) {
		tblog() << "Could not find any matching addons";
		return false;
	}

	//found correct Addon
	//resolve addon function addresses
	this->handlerType = ADDON;
	if (this->type == SYNC) {
		if (correctAddon->loadPlugin(this->id, this->path, 0) == 0) {
			this->query_by_addon = correctAddon->getQueryFunction();
		}
		else {
			this->query_by_addon = NULL;
			tblog(LOG_WARNING) << "Could not load plugin "<<this->name;
		}
	}
	else if (this->type == ASYNC) {
		if (correctAddon->loadPlugin(this->id, this->path, 1) == 0) {
			this->query_by_addon = correctAddon->getAsyncQueryFunction();
		}
		else {
			this->query_by_addon = NULL;
			tblog(LOG_WARNING) << "Could not load plugin " << this->name;
		}
	}
	this->finalize_by_addon = correctAddon->getFinalizedFunction();

	return true;
}


bool Plugin::plugin_loop() { //TODO
	// DEBUG
	tblog() << "Started Plugin loop for " << name;

	// loop waiting for queries
	while (Communications::keep_running) {
		// dequeue query
		Query* newquery = qq->dequeue(id);
		if (newquery == nullptr) {
			// done to wake from lock
			continue;
		}

		
		tblog() << "Plugin " << id << " got called";

		// TODO set default response for this plugin

		// run the query plugin function
		int response;

		if (value == Plugin::IGNORED) {
			// respond ok
			response = PLUGIN_RESPONSE_VALID;
		}
		else
		{
			if (this->handlerType == RAW || this->handlerType == OPENSSL) {
				tblog() << "Running native query";
				if (this->query == NULL) {
					return PLUGIN_RESPONSE_ERROR;
				}
				response = this->query(&(newquery->data));
			}
			else if (this->handlerType == ADDON) {
				tblog() << "Running addon query";
				if (this->query_by_addon == NULL) {
					return PLUGIN_RESPONSE_ERROR;
				}
				response = this->query_by_addon(this->id, &(newquery->data));
			}
			else {
				tblog(LOG_ERROR) << "Plugin " << id << " had unknown handlerType set, skipping...";
				return false;
			}
		}

		if (type == Plugin::SYNC) {
			// set response
			newquery->setResponse(id, response);
			tblog() << "For query " << newquery->getId() << " Plugin " << id << " synchronously returned " <<
				((response == PLUGIN_RESPONSE_VALID) ? "valid" : ((response == PLUGIN_RESPONSE_INVALID) ? "invalid" : ((response == PLUGIN_RESPONSE_ABSTAIN)?"abstain":"error")));
		}
	}
	
	// clean up
	tblog() << "Ending Plugin loop for " << name;
	if (this->handlerType == RAW || this->handlerType == OPENSSL) {
		tblog() << "Running native finalize";
		if (this->finalize) {
			tblog() << "Running native finalize";
			this->finalize();
		}
	}
	else if (this->handlerType == ADDON) {
		if (this->query_by_addon) {
			tblog() << "Running addon finalize";
			this->finalize_by_addon(this->id);
		}
	}
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
	tblog(LOG_INFO) << "\t Plugin " << id << " {";
	tblog(LOG_INFO) << "\t\t Name: " << name;
	tblog(LOG_INFO) << "\t\t Description: " << description;
	tblog(LOG_INFO) << "\t\t Path: " << path;
	switch (value) {
	case Plugin::CONGRESS:
		tblog(LOG_INFO) << "\t\t Aggregation Group: Congress";
		break;
	case Plugin::NEEDED:
		tblog(LOG_INFO) << "\t\t Aggregation Group: Needed";
		break;
	default:
		tblog(LOG_INFO) << "\t\t Aggregation Group: None";
	}

	if (this->handlerType == Plugin::RAW) {
		tblog() << "\t\t Handler Type: Raw Data";
		tblog() << "\t\t Query Function: 0x" << std::hex << query;

	}
	else if (this->handlerType == Plugin::OPENSSL) {
		tblog() << "\t\t Handler Type: OpenSSL Data";
		tblog() << "\t\t Query Function: 0x" << std::hex << query;

	}
	else if (this->handlerType == Plugin::ADDON) {
		tblog() << "\t\t Handler Type: Addon-handled " << this->handler;
		tblog() << "\t\t Query_By_Addon Function: 0x" << std::hex << query_by_addon;

	}

	tblog(LOG_INFO) << "\t },";
}

int Plugin::async_callback(int plugin_id, int query_id, int result) {
	// add our response
	// set response
	Query* foundquery = qq->find_linked(query_id);
	if (foundquery == nullptr) {
		tblog() << "Tried to asynchronously reply to a query that is no longer tracked";
		return 0; // let plugin know it was unsuccessful
	}

	foundquery->setResponse(plugin_id, result);
	tblog(LOG_INFO) << "For Query " << query_id << " Plugin " << plugin_id << " asynchronously returned " <<
		((result == PLUGIN_RESPONSE_VALID) ? "valid" : ((result == PLUGIN_RESPONSE_INVALID) ? "invalid" : ((result == PLUGIN_RESPONSE_ABSTAIN) ? "abstain" : "error")));

	return 1; // let plugin know the callback was successful
}

void Plugin::set_QueryQueue(QueryQueue* qq_in) {
	qq = qq_in;
}
