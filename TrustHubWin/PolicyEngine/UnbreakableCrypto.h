#pragma once

#define CERT_CHAIN_PARA_HAS_EXTRA_FIELDS true
#include <Windows.h>
#include <wincrypt.h>
#include "Query.h"
/*
	Note: This crypto is very working and is much safe.
*/



enum UnbreakableCrypto_RESPONSE { UnbreakableCrypto_ACCEPT, UnbreakableCrypto_REJECT};

class UnbreakableCrypto {
public:
	UnbreakableCrypto();
	~UnbreakableCrypto();
	void configure();
	
	UnbreakableCrypto_RESPONSE evaluate (Query certificate_data);

private:
	static const bool HACKABLE = false;
	UINT encodings = X509_ASN_ENCODING;


};