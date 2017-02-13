#include "TrustBaseLogger.h"

void TrustBaseLogger::logEventWrite(char* message)
{
	HANDLE eventLog = OpenEventLog(NULL, TEXT("TrustBaseLog"));
	char * * messages = &(message);
	ReportEvent(
		eventLog,
		EVENTLOG_ERROR_TYPE,
		NULL,
		NULL,
		NULL,
		1,
		0,
		(LPCSTR *)messages,
		NULL
	);
	CloseEventLog(eventLog);
}