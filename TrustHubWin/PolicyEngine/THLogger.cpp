#include "stdafx.h"
#include "THLogger.h"

// define statics
std::ofstream thlog::log_file;
bool thlog::also_cout = false;
std::mutex thlog::mtx;

thlog::thlog() {
		// lock the mutex
		thlog::mtx.lock();
}

thlog::~thlog() {
		// when we are destructed, we write out
		// this can help prevent threading issues
		log_file << tmp_ss.str() << std::endl;

		if (also_cout) {
			std::cout << tmp_ss.str() << std::endl;
		}
		// release the mutex
		thlog::mtx.unlock();
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
