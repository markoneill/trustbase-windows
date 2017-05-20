#include "stdafx.h"
#include "Addon.h"

Addon::Addon() {
}

Addon::Addon(const Addon & other) {

	this->id = other.id;
	this->name = other.name;
	this->path = other.path;
	this->type_handled = other.type_handled;
	this->description = other.description;
	this->version = other.version;

	this->initialize = other.initialize;
	this->finalize = other.finalize;
	this->load_plugin = other.load_plugin;
	this->query_plugin = other.query_plugin;
	this->query_plugin_async = other.query_plugin_async;
	this->finalize_plugin = other.finalize_plugin;
}

Addon::Addon(int id, std::string name, std::string path, std::string type_handled, std::string description, std::string version)
{
	this->id = id;
	this->name = name;
	this->path = path;
	this->type_handled = type_handled;
	this->description = description;
	this->version = version;

	this->initialize = NULL;
	this->finalize = NULL;
	this->load_plugin = NULL;
	this->query_plugin = NULL;
	this->query_plugin_async = NULL;
	this->finalize_plugin = NULL;
}

Addon::~Addon() {
}

bool Addon::init(size_t plugin_count, int(*callback)(int, int, int)) {

	HINSTANCE hDLL;

	std::wstring wpath = std::wstring(path.begin(), path.end());
	hDLL = LoadLibraryEx(wpath.c_str(), NULL, NULL);

	// Load shared object
	if (!hDLL) {
		thlog() << "Could not load " << path << " as dynamic library";
		return false;
	}

	// Load functions within shared object
	initialize = (addon_initialize)GetProcAddress(hDLL, "initialize");
	if (initialize == NULL) {
		thlog() << "Failed to load initialize function for addon '" << path << "': " << GetLastError();
		return false;
	}
	finalize = (addon_finalize)GetProcAddress(hDLL, "finalize");
	if (finalize == NULL) {
		thlog() << "Failed to load finalize function for addon '" << path << "': " << GetLastError();
		return false;
	}
	load_plugin = (addon_load_plugin)GetProcAddress(hDLL, "load_plugin");
	if (load_plugin == NULL) {
		thlog() << "Failed to load load_plugin function for addon '" << path << "': " << GetLastError();
		return false;
	}
	query_plugin = (addon_query_plugin)GetProcAddress(hDLL, "query_plugin");
	if (query_plugin == NULL) {
		thlog() << "Failed to load query_plugin function for addon '" << path << "': " << GetLastError();
		return false;
	}
	query_plugin_async = (addon_async_query_plugin)GetProcAddress(hDLL, "query_plugin_async");
	if (query_plugin_async == NULL) {
		thlog() << "Failed to load async_query_plugin function for addon '" << path << "': " << GetLastError();
		return false;
	}
	finalize_plugin = (addon_finalize_plugin)GetProcAddress(hDLL, "finalize_plugin");
	if (finalize_plugin == NULL) {
		thlog() << "Failed to load finalize_plugin function for addon '" << path << "': " << GetLastError();
		return false;
	}

	// initialize the addon
	init_addon_data_t i_addon_data;
	i_addon_data.plugin_count = plugin_count;
	i_addon_data.plugin_dir = ".";
	i_addon_data.callback = callback;
	i_addon_data.lib_file = this->path.c_str();
	i_addon_data.log = thlog::pluginTHLog;
	initialize(&i_addon_data);

	return true;
}

void Addon::printInfo() {

	thlog() << "\t Addon " << id << " {";
	thlog() << "\t\t Name: " << name;
	thlog() << "\t\t Description: " << description;
	thlog() << "\t\t Path: " << path;
	thlog() << "\t\t Type Handled: " << type_handled;
	thlog() << "\t\t Version: %s" << version;
}

std::string Addon::getTypeHandled()
{
	return this->type_handled;
}
addon_query_plugin Addon::getQueryFunction()
{
	return this->query_plugin;
}
addon_async_query_plugin Addon::getAsyncQueryFunction()
{
	return this->query_plugin_async;
}
addon_finalize_plugin Addon::getFinalizedFunction()
{
	return this->finalize_plugin;

}

int Addon::loadPlugin(int pluginId, std::string pluginPath, int isAsync)
{
	char* path = strdup(pluginPath.c_str());
	int ret = load_plugin(pluginId, path, isAsync);
	free(path);
	return ret;
}
int Addon::cleanup()
{
	return finalize();
}