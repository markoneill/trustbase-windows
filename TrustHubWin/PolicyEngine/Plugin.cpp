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
	this->value = Plugin::IGNORED;
}

Plugin::Plugin(std::string name, std::string path, std::string handler, std::string description, std::string version) {
	this->name = name;
	this->path = path;
	this->handler = handler;
	this->description = description;
	this->version = version;
	this->value = Plugin::IGNORED;
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
		thlog() << "Could not locate 'query' function for plugin " << name;
		return false;
	}

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
	thlog() << "\t\t Query Function: " << std::hex << query;
	thlog() << "\t },";
}
