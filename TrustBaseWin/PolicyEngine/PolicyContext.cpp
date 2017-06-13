#include "stdafx.h"
#include "PolicyContext.h"


PolicyContext::PolicyContext() {
	this->plugins = NULL;
	this->plugin_count = 0;
	this->addons = NULL;
	this->addon_count = 0;
}


PolicyContext::~PolicyContext() {
	delete[] plugins;
	delete[] addons;
}

bool PolicyContext::loadConfig(const char* path) {
	libconfig::Config cfg;
	int count;

	tblog() << "Loading configuration at " << path;
	try {
		cfg.readFile(path);
	}
	catch (const libconfig::FileIOException &fioex) {
		(void)fioex;
		tblog() << "I/O error while reading file " << path;
		return false;
	}
	catch (const libconfig::ParseException &pex) {
		tblog() << "Parse error at " << pex.getFile() << ":" << pex.getLine() << " - " << pex.getError();
		return false;
	}

	try {
		// Parse addons
		const libconfig::Setting& addons_group = cfg.lookup("addons");
		count = addons_group.getLength();
		addons = new Addon[count];
		addon_count = count;

		for (int i = 0; i < count; i++) {
			std::string name;
			std::string description = "";
			std::string version = "";
			std::string type;
			std::string addonpath;

			if (!addons_group[i].lookupValue("name", name)) {
				tblog() << "Could not parse name of addon " << i;
				continue;
			}

			addons_group[i].lookupValue("description", description);
			addons_group[i].lookupValue("version", version);

			if (!addons_group[i].lookupValue("type", type)) {
				tblog() << "Could not parse type of addon " << name;
				continue;
			}

			if (!addons_group[i].lookupValue("path", addonpath)) {
				tblog() << "Could not parse path of addon " << name;
				continue;
			}

			// Create and store addon object
			addons[i] = Addon(i, name, addonpath, type, description, version);

		}


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
			int openssl = 0;
			if (!plugin_group[i].lookupValue("name", name)) {
				tblog() << "Could not parse name of plugin " << i;
				continue;
			}
			if (!plugin_group[i].lookupValue("path", ppath)) {
				tblog() << "Could not parse path of plugin " << name;
				continue;
			}
			if (!plugin_group[i].lookupValue("handler", handler)) {
				tblog() << "Could not parse handler of plugin " << name;
				continue;
			}
			plugin_group[i].lookupValue("description", description);
			plugin_group[i].lookupValue("version", version);
			plugin_group[i].lookupValue("type", type);
			plugin_group[i].lookupValue("openssl", openssl);

			Plugin::Type ttype = Plugin::SYNC;
			if (type.at(0) == 'a' || type.at(0) == 'A') {
				ttype = Plugin::ASYNC;
			}

			Plugin::HandlerType httype = Plugin::UNKNOWN;
			if (handler.compare("native") == 0) {
				if (openssl==0) {
					httype = Plugin::RAW;
				}
				else
				{
					httype = Plugin::OPENSSL;
				}
			}

			// Create and store plugin object
			plugins[i] = Plugin(i, name, ppath, handler, ttype, httype, description, version);
		}

		// Parse aggregation
		const libconfig::Setting& ag_group = cfg.lookup("aggregation");

		if (!ag_group.lookupValue("congress_threshold", congress_threshold)) {
			tblog() << "Could not parse the congress threshold";
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
		tblog() << "Could not find setting " << nfex.getPath();
		return false;
	} catch (...) {
		tblog() << "Incorrectly formed config file " << path;
		return false;
	}

	return true;
}

bool PolicyContext::initAddons() {
	for (int i = 0; i < addon_count; i++) {
		if (!addons[i].init(this->plugin_count,Plugin::async_callback)) {
			return false;
		}
	}
	return true;
}

Plugin * PolicyContext::getPlugin(const char * name) {
	for (int i = 0; i < plugin_count; i++) {
		if (plugins[i].getName() == name) {
			return &(plugins[i]);
		}
	}
	tblog() << "Could not find plugin " << name;
	throw std::out_of_range("Invalid name index for plugins");
	return nullptr;
}

bool PolicyContext::initPlugins() {
	for (int i = 0; i < plugin_count; i++) {
		if (!plugins[i].init(addons, addon_count)) {
			return false;
		}
	}
	return true;
}

void PolicyContext::printWelcome() {

	tblog() << "\n[#][#] Starting Trustbase [#][#]";
	tblog() << "Loaded plugins: [";
	for (int i = 0; i < plugin_count; i++) {
		plugins[i].printInfo();
	}
	tblog() << "]\n";
}
