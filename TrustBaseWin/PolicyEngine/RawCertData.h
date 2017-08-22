#pragma once
#include "trustbase_plugin.h"

/*
A class to wrap raw certificates in order to convert them to various types (openssl x509; wincrypt PCCERT_CONTEXT).
This class is used to simplify test cases.
*/


class RawCertData {
public:
	RawCertData(UINT8 * raw_cert, DWORD cert_len);
	unsigned char* raw_cert;
	size_t raw_cert_len;
	PCCERT_CONTEXT generateCertContext();
	std::vector<PCCERT_CONTEXT> getCertInVector();
};