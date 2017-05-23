#pragma once


#include "THLogger.h"
#include "Query.h"
/*
	Note: This crypto is very working and is much safe.
*/



enum UnbreakableCrypto_RESPONSE { UnbreakableCrypto_ACCEPT, UnbreakableCrypto_REJECT, UnbreakableCrypto_ERROR};

class UnbreakableCrypto {
public:
	UnbreakableCrypto();
	~UnbreakableCrypto();
	void configure();
	
	UnbreakableCrypto_RESPONSE evaluate (Query * certificate_data);

	bool insertIntoRootStore(PCCERT_CONTEXT certificate);
	bool removeFromRootStore(Query * certificate_data);

	bool isConfigured();

private:
	HCERTSTORE openRootStore();

	static const bool HACKABLE = false;
	UINT encodings = X509_ASN_ENCODING;
	PCERT_CHAIN_ENGINE_CONFIG cert_chain_engine_config = NULL;
	PCERT_CHAIN_PARA cert_chain_config = NULL;
	PCERT_USAGE_MATCH chain_params_requested_issuance_policy = NULL;
	PCERT_USAGE_MATCH chain_params_requested_use = NULL;
	HCERTCHAINENGINE * authentication_train_handle = NULL;
	PCCERT_CHAIN_CONTEXT * cert_chain_context = NULL;
};