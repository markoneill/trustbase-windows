#pragma once

#ifndef TB_EVENT_LOGGER_H
#define TB_EVENT_LOGGER_H

#include <iostream>
#include <sstream>
#include <fstream>
#include <mutex>
#include "TBLogger_Level.h"
#define LOG_NAME L"TrustBaseLog"

class tbeventlog {
public:

	tbeventlog();
	~tbeventlog();
	bool sendEvent(int accepted, std::string sourcePath, std::string hostName);

private:
	
	std::string blockedConnectionStr = "TrustBase blocked this connection";
	std::string acceptedConnectionStr = "TrustBase accepted this connection";
	WORD blockedType = EVENTLOG_WARNING_TYPE;
	WORD acceptedType = EVENTLOG_INFORMATION_TYPE;
	HANDLE hEventLog = NULL;

};

#endif // TB_EVENT_LOGGER_H