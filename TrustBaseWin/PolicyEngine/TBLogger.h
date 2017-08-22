#pragma once
#ifndef TB_LOGGER_H
#define TB_LOGGER_H
#include <iostream>
#include <sstream>
#include <fstream>
#include <mutex>
#include "TBLogger_Level.h"
class tblog {
public:
	tblog();
	tblog(tblog_level_t);

	~tblog();

	template<class T>
	tblog& operator<<(const T &x) {
		tmp_ss << x; // write to our tmp stream
		return *this;
	}

	static int pluginTBLog(tblog_level_t level, const char* format, ...);

	static bool setFile(const char* path, bool also_stdout = false);

	static bool setMinimumLevel(tblog_level_t minimum_level);

	static bool close();
private:
	// disallow any copying
	tblog(tblog const&) = delete;
	void operator=(tblog const&) = delete;
	
	std::ostringstream tmp_ss;
	tblog_level_t level;

	static std::ofstream log_file;
	static bool also_cout;
	static std::mutex mtx;
	static tblog_level_t minimum_level;
};
#endif // TB_LOGGER_H