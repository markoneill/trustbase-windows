#include "stdafx.h"
#include "RawCertData.h"

/*
A class to wrap raw certificates in order to convert them to various types (openssl x509; wincrypt PCCERT_CONTEXT).
This class is used to simplify test cases.
*/

RawCertData::RawCertData(UINT8 * raw_cert, DWORD cert_len)
{
	this->raw_cert = raw_cert;
	this->raw_cert_len = cert_len;
}

PCCERT_CONTEXT RawCertData::generateCertContext()
{
	return CertCreateCertificateContext(
	X509_ASN_ENCODING,
		raw_cert,
		raw_cert_len
	);
}

std::vector<PCCERT_CONTEXT> RawCertData::getCertInVector()
{
	auto a  = std::vector<PCCERT_CONTEXT>();
	a.push_back(CertCreateCertificateContext(
		X509_ASN_ENCODING,
		raw_cert,
		raw_cert_len
	));
	return a;
}
