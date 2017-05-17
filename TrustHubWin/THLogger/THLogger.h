#ifndef THLOGGER_H
#define THLOGGER_H

#include <iostream>
#include <sstream>
#include <fstream>
#include <mutex>

typedef enum thlog_level_t { LOG_DEBUG = 0, LOG_INFO = 1, LOG_WARNING = 2, LOG_ERROR = 3, LOG_NONE = 4 } thlog_level_t;

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