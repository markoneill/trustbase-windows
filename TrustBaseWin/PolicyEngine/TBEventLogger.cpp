#include "stdafx.h"
#include "TBEventLogger.h"
#include "trustbase_plugin.h"
#include <stdarg.h>

tbeventlog::tbeventlog() {
	
	hEventLog = RegisterEventSource(NULL, LOG_NAME);
	if (NULL == hEventLog)
	{
		//failed;
	}
}

tbeventlog::~tbeventlog() {
	if (hEventLog)
		DeregisterEventSource(hEventLog);
}

bool tbeventlog::sendEvent(int accepted, std::string sourcePath, std::string hostName) {

	if (NULL == hEventLog)
	{
		//failed;
		return false;
	}

	std::string message;
	WORD messageType;

	if (accepted == PLUGIN_RESPONSE_VALID)
	{
		message = acceptedConnectionStr;
		messageType = acceptedType;
	}
	else
	{
		message = blockedConnectionStr;
		messageType = blockedType;
	}

	std::string fullMessage = sourcePath + "|" + hostName + "|" + message;
	LPWSTR fullMessageLPCWSTR = new WCHAR[strlen(fullMessage.c_str()) + 1];
	MultiByteToWideChar(CP_OEMCP, 0, fullMessage.c_str(), -1, fullMessageLPCWSTR, strlen(fullMessage.c_str()) + 1);
	LPWSTR pInsertStrings[1] = { fullMessageLPCWSTR };
	if (!ReportEvent(hEventLog, messageType, accepted, 0, NULL, 1, 0, (LPCWSTR*)pInsertStrings, NULL))
	{
		return false;
	}
	return true;
}
