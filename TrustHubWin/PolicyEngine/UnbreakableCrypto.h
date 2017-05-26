#pragma once


#include "THLogger.h"
#include "Query.h"
#include <vector>
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
	UnbreakableCrypto_RESPONSE evaluate(PCCERT_CONTEXT cert_context, LPWSTR hostname);
	UnbreakableCrypto_RESPONSE evaluate (UINT8 * cert_data, DWORD cert_len, LPWSTR hostname);

	/*
	These 3 hostname validation functions come from John Viega and Matt Messier's <b>Secure Programming Cookbook</b>
	Published in 2003 by O'Reily and Associates Inc. Edited by Deborah Russell.
	*/
	bool UnbreakableCrypto::checkHostname(PCCERT_CONTEXT pCertContext, LPWSTR lpszHostName);
	//LPWSTR SPC_make_wide(LPCTSTR str);
	LPWSTR SPC_fold_wide(LPWSTR str);

	bool insertIntoRootStore(PCCERT_CONTEXT certificate);
	bool removeFromRootStore(Query * certificate_data);

	bool isConfigured();

private:
	HCERTSTORE openRootStore();
	HCERTSTORE openMyStore();
	unsigned int ntoh24(const UINT8* data);

	static const bool HACKABLE = false;
	UINT encodings = X509_ASN_ENCODING;
	CERT_ENHKEY_USAGE * empty_enhkey = NULL;
	PCERT_CHAIN_ENGINE_CONFIG cert_chain_engine_config = NULL;
	PCERT_CHAIN_PARA cert_chain_config = NULL;
	PCERT_USAGE_MATCH chain_params_requested_issuance_policy = NULL;
	PCERT_USAGE_MATCH chain_params_requested_use = NULL;
	HCERTCHAINENGINE authentication_train_handle = NULL;
	PCCERT_CHAIN_CONTEXT cert_chain_context = NULL;
};