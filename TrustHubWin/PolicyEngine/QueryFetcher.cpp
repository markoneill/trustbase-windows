#include "stdafx.h"
#include "QueryFetcher.h"

Query * QueryFetcher::fetch(FetchFileName ffn) {

	LPCWSTR filename;

	switch (ffn) {
		case VALID_CERT_FILENAME:
			filename = TEXT("./ValidCert.cer");
			break;
		case INVALID_CERT_FILENAME:
			filename = TEXT("./ExpiredCert.cer");
			break;
		case BAD_HOST_CERT_FILENAME:
			filename = TEXT("./WrongHostCert.cer");
			break;
		case NULL_HOST_CERT_FILENAME:
			filename = TEXT("./NullHostCert.cer");
			break;
		case MALFORMED_CERT_FILENAME:
			filename = TEXT("./MalformedCert.cer");
			break;
		default:
			return NULL;
	}

	HANDLE file_handle = CreateFile(
		filename,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);
	if (file_handle == INVALID_HANDLE_VALUE) {
		std::cout << "Could not open file ERROR: " << GetLastError() << ". filename: " << CW2A(filename) << std::endl;
		std::system("PAUSE");
		return NULL;
	}

	DWORD filesize = GetFileSize(file_handle, NULL);//Undefined behavior on file sizes greator than unsigned long
	if(GetLastError() != NO_ERROR)
	{
		std::cout << "Could get file size! ERROR " << GetLastError() << ". filename: "<< CW2A(filename) << std::endl;
		std::system("PAUSE");
		return NULL;
	}

	UCHAR * cert_data = new UCHAR[filesize];

	ReadFile(
		file_handle,
		cert_data,
		filesize,
		NULL,
		NULL
	);

	return new Query(
		cert_data, 
		filesize
	);
}
