#include "stdafx.h"
#include "TBLogger.h"
#include <stdarg.h>

// define statics
std::ofstream tblog::log_file;
bool tblog::also_cout = false;
std::mutex tblog::mtx;
tblog_level_t tblog::minimum_level = LOG_DEBUG;
tblog::tblog() : tblog(LOG_DEBUG) {}
tblog::tblog(tblog_level_t log_level) {
	level = log_level;
	// lock the mutex
	tblog::mtx.lock();
}

tblog::~tblog() {
		// when we are destructed, we write out
		// this can help prevent threading issues

		// If the log level is below the minimum, ditch it
		if (minimum_level <= level) {
			std::string level_indicator;
			switch (level) {
			case LOG_DEBUG:
				level_indicator =" :DBG: ";
				break;
			case LOG_INFO:
				level_indicator = " :INF: ";
				break;
			case LOG_WARNING:
				level_indicator = " :WRN: ";
				break;
			case LOG_ERROR:
				level_indicator = " :ERR: ";
				break;
			case LOG_NONE:
				level_indicator = " :";
				break;
			}

			if (also_cout) {
				std::cout << level_indicator << tmp_ss.str() << std::endl;
			}

			log_file << level_indicator << tmp_ss.str() << std::endl;
		}

		// release the mutex
		tblog::mtx.unlock();
}

bool tblog::setMinimumLevel(tblog_level_t min_level) {
	minimum_level = min_level;
	return true;
}

bool tblog::setFile(const char * path, bool also_stdout) {
	if (log_file.is_open()) {
		log_file.close();
	}
	log_file.open(path, std::ofstream::app);

	also_cout = also_stdout;
	return true;
}

bool tblog::close() {
	log_file.close();
	return true;
}

int tblog::pluginTBLog(tblog_level_t level, const char* format, ...){
	va_list args;
	// Parse the args
	va_start(args, format);
	int count = _vscprintf(format, args);
	char* buffer = (char*)malloc(count + 1);
	vsprintf(buffer, format, args);
	tblog(level) << buffer;

	va_end(args);
	free(buffer);
	return 1;
}