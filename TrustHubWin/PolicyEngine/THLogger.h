#ifndef THLOGGER_H
#define THLOGGER_H

#include <iostream>
#include <sstream>
#include <fstream>
#include <mutex>


class thlog {
public:
	thlog();

	~thlog();

	template<class T>
	thlog& operator<<(const T &x) {
		tmp_ss << x; // write to our tmp stream
		return *this;
	}

	static bool setFile(const char* path, bool also_stdout = false);

	static bool close();

private:
	// disallow any copying
	thlog(thlog const&) = delete;
	void operator=(thlog const&) = delete;
	
	std::ostringstream tmp_ss;

	static std::ofstream log_file;
	static bool also_cout;
	static std::mutex mtx;
};


#endif // THLOGGER_H