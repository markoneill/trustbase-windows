#ifndef THLOGGER_H
#define THLOGGER_H

#include <iostream>
#include <sstream>
#include <fstream>


class thlog {
public:
	thlog() {};
	~thlog() {
		// when we are destructed, we write out
		// this can help prevent threading issues
		log_file << tmp_ss.str() << std::endl;

		if (also_cout) {
			std::cout << tmp_ss.str() << std::endl;
		}
	}

	template<class T>
	thlog& operator<<(const T &x) {
		tmp_ss << x; // write to our tmp stream
		return *this;
	}

	static bool setFile(const char* path, bool also_stdout=false) {
		if (log_file.is_open()) {
			log_file.close();
		}
		log_file.open(path, std::ofstream::app);

		also_cout = also_stdout;
		return true;
	}

	static bool close() {
		log_file.close();
	}

private:
	// disallow any copying
	thlog(thlog const&) = delete;
	void operator=(thlog const&) = delete;
	
	std::ostringstream tmp_ss;

	static std::ofstream log_file;
	static bool also_cout;
};


#endif // THLOGGER_H