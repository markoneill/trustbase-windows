#ifndef THLOGGER_H
#define THLOGGER_H

#include <iostream>
#include <sstream>
#include <fstream>
#include <mutex>
#include "THLogger_Level.h"

class thlog {
public:


	thlog();
	thlog(thlog_level_t);

	~thlog();

	template<class T>
	thlog& operator<<(const T &x) {
		tmp_ss << x; // write to our tmp stream
		return *this;
	}

	static int pluginTHLog(thlog_level_t level, const char* format, ...);

	static bool setFile(const char* path, bool also_stdout = false);

	static bool setMinimumLevel(thlog_level_t minimum_level);

	static bool close();
private:
	// disallow any copying
	thlog(thlog const&) = delete;
	void operator=(thlog const&) = delete;
	
	std::ostringstream tmp_ss;
	thlog_level_t level;

	static std::ofstream log_file;
	static bool also_cout;
	static std::mutex mtx;
	static thlog_level_t minimum_level;
};



#endif // THLOGGER_H