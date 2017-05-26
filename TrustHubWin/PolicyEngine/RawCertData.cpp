#include "stdafx.h"
#include "RawCertData.h"

RawCertData::RawCertData(UINT8 * raw_cert, DWORD cert_len)
{
	this->raw_cert = raw_cert;
	this->raw_cert_len = cert_len;
}