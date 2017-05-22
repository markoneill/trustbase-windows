#pragma once


#include "THLogger.h"
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
	PCERT_CHAIN_ENGINE_CONFIG cert_chain_engine_config;
	PCERT_CHAIN_PARA cert_chain_config;
	PCERT_USAGE_MATCH chain_params_requested_issuance_policy;
	PCERT_USAGE_MATCH chain_params_requested_use;
	HCERTCHAINENGINE * authentication_train_handle;
	PCCERT_CHAIN_CONTEXT * cert_chain_context;
};