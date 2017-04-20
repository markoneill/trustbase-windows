#include "stdafx.h"
#include "PolicyContext.h"


PolicyContext::PolicyContext() {
	this->plugins = NULL;
	this->plugin_count = 0;
}


PolicyContext::~PolicyContext() {
	delete[] plugins;
}

bool PolicyContext::loadConfig(const char* path) {
	libconfig::Config cfg;
	int count;

	thlog() << "Loading configuration at " << path;
	try {
		cfg.readFile(path);
	}
	catch (const libconfig::FileIOException &fioex) {
		thlog() << "I/O error while reading file " << path;
		return false;
	}
	catch (const libconfig::ParseException &pex) {
		thlog() << "Parse error at " << pex.getFile() << ":" << pex.getLine() << " - " << pex.getError();
		return false;
	}


	try {
		// Parse addons
		//TODO

		// Parse plugins
		const libconfig::Setting& plugin_group = cfg.lookup("plugins");
		count = plugin_group.getLength();
		plugins = new Plugin[count];
		plugin_count = count;

		for (int i = 0; i < count; i++) {
			std::string name;
			std::string ppath;
			std::string handler;
			std::string description = "";
			std::string version = "";
			std::string type = "";
			if (!plugin_group[i].lookupValue("name", name)) {
				thlog() << "Could not parse name of plugin " << i;
				continue;
			}
			if (!plugin_group[i].lookupValue("path", ppath)) {
				thlog() << "Could not parse path of plugin " << name;
				continue;
			}
			if (!plugin_group[i].lookupValue("handler", handler)) {
				thlog() << "Could not parse handler of plugin " << name;
				continue;
			}
			plugin_group[i].lookupValue("description", description);
			plugin_group[i].lookupValue("version", version);
			plugin_group[i].lookupValue("type", type);

			Plugin::Type ttype = Plugin::SYNC;
			if (type.at(0) == 'a' || type.at(0) == 'A') {
				ttype = Plugin::ASYNC;
			}
			// Create and store plugin object
			plugins[i] = Plugin(name, ppath, handler, ttype, description, version);
		}

		// Parse aggregation
		const libconfig::Setting& ag_group = cfg.lookup("aggregation");

		if (!ag_group.lookupValue("congress_threshold", congress_threshold)) {
			thlog() << "Could not parse the congress threshold";
			return false;
		}

		const libconfig::Setting& necessary_group = ag_group["sufficient"]["necessary_group"];
		count = necessary_group.getLength();

		for (int i = 0; i < count; i++) {
			const char* plugin_name = necessary_group[i];
			// Find the plugin and set it necessary
			getPlugin(plugin_name)->setValue(Plugin::NEEDED);
		}

		const libconfig::Setting& congress_group = ag_group["sufficient"]["congress_group"];
		count = congress_group.getLength();

		for (int i = 0; i < count; i++) {
			const char* plugin_name = congress_group[i];
			// Find the plugin and set it to congress
			getPlugin(plugin_name)->setValue(Plugin::CONGRESS);
		}


	} catch (const libconfig::SettingNotFoundException &nfex) {
		thlog() << "Could not find setting " << nfex.getPath();
		return false;
	} catch (...) {
		thlog() << "Incorrectly formed config file " << path;
		return false;
	}

	return true;
}

Plugin * PolicyContext::getPlugin(const char * name) {
	for (int i = 0; i < plugin_count; i++) {
		if (plugins[i].getName() == name) {
			return &(plugins[i]);
		}
	}
	thlog() << "Could not find plugin " << name;
	throw std::out_of_range("Invalid name index for plugins");
	return nullptr;
}

bool PolicyContext::initPlugins() {
	for (int i = 0; i < plugin_count; i++) {
		if (!plugins[i].init()) {
			return false;
		}
	}
	return true;
}

void PolicyContext::printWelcome() {

	thlog() << "\n[#][#] Starting Trusthub [#][#]";
	thlog() << "Loaded plugins: [";
	for (int i = 0; i < plugin_count; i++) {
		plugins[i].printInfo();
	}
	thlog() << "]\n";
}
