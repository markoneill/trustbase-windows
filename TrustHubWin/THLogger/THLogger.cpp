#include "stdafx.h"
#include "THLogger.h"

// define statics
std::ofstream thlog::log_file;
bool thlog::also_cout = false;
std::mutex thlog::mtx;
thlog_level_t thlog::minimum_level = LOG_DEBUG;

thlog::thlog() : thlog(LOG_DEBUG) {}
thlog::thlog(thlog_level_t log_level) {
	level = log_level;
	// lock the mutex
	thlog::mtx.lock();
}

thlog::~thlog() {
		// when we are destructed, we write out
		// this can help prevent threading issues

		// If the log level is below the minimum, ditch it
		if (minimum_level > level) {
		}
		else
		{
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
		thlog::mtx.unlock();
}

bool thlog::setMinimumLevel(thlog_level_t min_level) {
	minimum_level = min_level;
	return true;
}

bool thlog::setFile(const char * path, bool also_stdout) {
	if (log_file.is_open()) {
		log_file.close();
	}
	log_file.open(path, std::ofstream::app);

	also_cout = also_stdout;
	return true;
}

bool thlog::close() {
	log_file.close();
	return true;
}
