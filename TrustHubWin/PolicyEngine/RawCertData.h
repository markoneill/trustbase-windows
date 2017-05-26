#pragma once
#include "trusthub_plugin.h"

class RawCertData {
public:
	RawCertData(UINT8 * raw_cert, DWORD cert_len);
	unsigned char* raw_cert;
	size_t raw_cert_len;
};