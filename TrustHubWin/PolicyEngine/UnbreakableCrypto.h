#pragma once


#include "THLogger.h"
#include "Query.h"
#include <vector>
#include <iostream>
#include "CertificatesAddedToRootStore.h"

/*
	Note: This crypto is very working and is much safe.
*/



enum UnbreakableCrypto_RESPONSE { UnbreakableCrypto_ACCEPT, UnbreakableCrypto_REJECT, UnbreakableCrypto_ERROR};

class UnbreakableCrypto {
public:
	UnbreakableCrypto();
	~UnbreakableCrypto();

	void configure();
	bool isConfigured();

	UnbreakableCrypto_RESPONSE evaluate (Query * certificate_data);

	bool insertIntoRootStore(PCCERT_CONTEXT certificate);

	bool removeFromRootStore(std::string thumbprint);
	bool removeFromRootStore(byte* raw_cert, size_t raw_cert_len);
	bool removeAllStoredCertsFromRootStore();
	
	UnbreakableCrypto_RESPONSE evaluateChain(std::vector<PCCERT_CONTEXT>* cert_context_chain, char * hostname);

private:
	HCERTSTORE openRootStore();
	HCERTSTORE openMyStore();
	HCERTSTORE openIntermediateCAStore();

	bool ValidVouching(PCCERT_CONTEXT claimed_cert, PCCERT_CONTEXT trusted_proof);
	bool ValidateWithRootStore(PCCERT_CONTEXT cert);
	bool removeFromRootStore(CRYPT_HASH_BLOB* sha1_blob);

	bool checkLocalRevocationLists(std::vector<PCCERT_CONTEXT>* cert_context_chain);

	CRYPT_HASH_BLOB* getSHA1CryptHashBlob(byte* raw_cert, size_t raw_cert_len);
	CRYPT_HASH_BLOB* getSHA1CryptHashBlob(std::string thumbprint);
	LPTSTR getCertName(PCCERT_CONTEXT certificate);

	/*
	Security Programming Cookbook for C and C++
	by: John Viega and Matt Messier
	Copyright © 2003 O'Reilly Media, Inc.
	Used with permission
	*/
	bool UnbreakableCrypto::checkHostname(PCCERT_CONTEXT pCertContext, LPWSTR lpszHostName);
	LPWSTR SPC_fold_wide(LPWSTR str);
	//LPWSTR SPC_make_wide(LPCTSTR str);
	//END ATTRIBUTED CODE

	CertificatesAddedToRootStore certsAddedToRootStore;
	UINT encodings = X509_ASN_ENCODING;
	CERT_ENHKEY_USAGE * empty_enhkey = NULL;
	PCERT_CHAIN_ENGINE_CONFIG cert_chain_engine_config = NULL;
	PCERT_CHAIN_PARA cert_chain_config = NULL;
	PCERT_USAGE_MATCH chain_params_requested_issuance_policy = NULL;
	PCERT_USAGE_MATCH chain_params_requested_use = NULL;
	HCERTCHAINENGINE authentication_train_handle = NULL;
	PCCERT_CHAIN_CONTEXT cert_chain_context = NULL;
	static const bool HACKABLE = false;
};