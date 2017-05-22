#include "stdafx.h"
#include "UnbreakableCrypto.h"
#include "THLogger.h"



UnbreakableCrypto::UnbreakableCrypto() {

}


UnbreakableCrypto::~UnbreakableCrypto() {

}


void UnbreakableCrypto::configure() {
	encodings = (X509_ASN_ENCODING | PKCS_7_ASN_ENCODING);//No Idea
}

UnbreakableCrypto_RESPONSE UnbreakableCrypto::evaluate(Query cert_data) {

	PCCERT_CONTEXT certificate_context = CertCreateCertificateContext(
		encodings, 
		cert_data.data.raw_chain,
		cert_data.data.raw_chain_len
	);

	if (certificate_context == NULL) {
		thlog() << "Wincrypt could not parse certificate. Error Code " << GetLastError();
		return UnbreakableCrypto_REJECT;
	}


	PCERT_CHAIN_ENGINE_CONFIG cert_chain_engine_config = new CERT_CHAIN_ENGINE_CONFIG();
	cert_chain_engine_config->cbSize = sizeof(*cert_chain_engine_config);
	cert_chain_engine_config->hRestrictedRoot = NULL;
	cert_chain_engine_config->hRestrictedTrust = NULL;
	cert_chain_engine_config->hRestrictedOther = NULL;
	cert_chain_engine_config->cAdditionalStore = 0;
	cert_chain_engine_config->rghAdditionalStore = NULL;
	cert_chain_engine_config->dwFlags = 0x00000000;
	cert_chain_engine_config->dwUrlRetrievalTimeout = 0;
	cert_chain_engine_config->MaximumCachedCertificates = 0;
	cert_chain_engine_config->CycleDetectionModulus = 0;
	cert_chain_engine_config->hExclusiveRoot = NULL;
	cert_chain_engine_config->hExclusiveTrustedPeople = NULL;
	cert_chain_engine_config->dwExclusiveFlags = 0x00000000;


	CERT_USAGE_MATCH chain_params_requested_use;
	chain_params_requested_use.dwType = USAGE_MATCH_TYPE_AND; //No Idea
	chain_params_requested_use.Usage = NULL; //No Idea

	CERT_USAGE_MATCH chain_params_requested_issuance_policy;
	chain_params_requested_issuance_policy.dwType = USAGE_MATCH_TYPE_AND; //No Idea
	chain_params_requested_issuance_policy.Usage = NULL; //No Idea

	//PCCERT_STRONG_SIGN_PARA chain_params_strong_crypto_param = new CERT_STRONG_SIGN_PARA;
	//chain_params_strong_crypto_param->cbSize = sizeof(*chain_params_strong_crypto_param);
	//chain_params_strong_crypto_param->dwInfoChoice = ;
	//chain_params_strong_crypto_param->pSerializedInfo = ;
	//chain_params_strong_crypto_param->pszOID = ;
	//chain_params_strong_crypto_param->pvInfo = ;

	PCERT_CHAIN_PARA cert_chain_config = new CERT_CHAIN_PARA;
	cert_chain_config->cbSize = sizeof(*cert_chain_config);
	cert_chain_config->RequestedUsage = chain_params_requested_use;
	cert_chain_config->RequestedIssuancePolicy = chain_params_requested_issuance_policy;
	cert_chain_config->dwUrlRetrievalTimeout = 0; //Default Revocation Timeout
	cert_chain_config->fCheckRevocationFreshnessTime = true; //Fetch a new CRL if the current one is older than dwRevocationFreshnessTime
	cert_chain_config->dwRevocationFreshnessTime = 86400; //seconds the CRL set should last - currently one day
	cert_chain_config->pftCacheResync = NULL;// Do not expire cached values
	cert_chain_config->pStrongSignPara = NULL;//Do not specify stong algorithms specifically
	cert_chain_config->dwStrongSignFlags = 0; //Do not ignore short public key length of last certificate


	HCERTCHAINENGINE * authentication_train_handle;
	if (!CertCreateCertificateChainEngine(cert_chain_engine_config, authentication_train_handle)) 
	{
		thlog() << "Wincrypt could not create authentiation train Error Code: " << GetLastError();
		delete cert_chain_engine_config;
		delete cert_chain_config;
		CertFreeCertificateContext(certificate_context);
		return UnbreakableCrypto_REJECT;
	}

	PCCERT_CHAIN_CONTEXT * cert_chain_context;
	if (!CertGetCertificateChain(
			authentication_train_handle,
			certificate_context,
			NULL, //Uses System time by default
			NULL, //Search no extra certificate stores
			cert_chain_config,
			0x0000000, // No Flags
			NULL,
			cert_chain_context)
		) 
	{
		thlog() << "Wincrypt could not create authentiation train Error Code: " << GetLastError();
		delete cert_chain_engine_config;
		delete cert_chain_config;
		CertFreeCertificateContext(certificate_context);
		return UnbreakableCrypto_REJECT;
	}


	CertVerifyCertificateChainPolicy(
		pszPolicyOID,
		pChainContext,
		pPolicyPara,
		pPolicyStatus
	);




	CertFreeCertificateChainEngine(authentication_train_handle);
	delete cert_chain_engine_config;
	delete cert_chain_config;
	return UnbreakableCrypto_ACCEPT;
}