#include "stdafx.h"
#include "UnbreakableCrypto.h"




UnbreakableCrypto::UnbreakableCrypto() {

}


UnbreakableCrypto::~UnbreakableCrypto() {

	if (cert_chain_engine_config != NULL) {
		delete cert_chain_engine_config;
	}
	if (cert_chain_config != NULL) {
		delete cert_chain_config;
	}
	if (chain_params_requested_issuance_policy != NULL) {
		delete chain_params_requested_issuance_policy;
	}
	if (chain_params_requested_use != NULL) {
		delete chain_params_requested_use;
	}
	if (authentication_train_handle != NULL) {
		CertFreeCertificateChainEngine(authentication_train_handle);
	}
	if (cert_chain_context != NULL) {
		CertFreeCertificateChain(cert_chain_context);
	}
}
	


void UnbreakableCrypto::configure() {
	encodings = (X509_ASN_ENCODING | PKCS_7_ASN_ENCODING);//No Idea

	//PCCERT_STRONG_SIGN_PARA chain_params_strong_crypto_param = new CERT_STRONG_SIGN_PARA;
	//chain_params_strong_crypto_param->cbSize = sizeof(*chain_params_strong_crypto_param);
	//chain_params_strong_crypto_param->dwInfoChoice = ;
	//chain_params_strong_crypto_param->pSerializedInfo = ;
	//chain_params_strong_crypto_param->pszOID = ;
	//chain_params_strong_crypto_param->pvInfo = ;

	empty_enhkey = new CERT_ENHKEY_USAGE;
	empty_enhkey->cUsageIdentifier = 0; //EMPTY
	empty_enhkey->rgpszUsageIdentifier = NULL; //NO LIST

	chain_params_requested_use = new CERT_USAGE_MATCH;
	chain_params_requested_use->dwType = USAGE_MATCH_TYPE_AND; //No Idea
	chain_params_requested_use->Usage = *empty_enhkey; //No Idea

	chain_params_requested_issuance_policy = new CERT_USAGE_MATCH;
	chain_params_requested_issuance_policy->dwType = USAGE_MATCH_TYPE_AND; //No Idea
	chain_params_requested_issuance_policy->Usage = *empty_enhkey; //No Idea

	cert_chain_config = new CERT_CHAIN_PARA;
	cert_chain_config->cbSize = sizeof(*cert_chain_config);
	cert_chain_config->RequestedUsage = *chain_params_requested_use;
	cert_chain_config->RequestedIssuancePolicy = *chain_params_requested_issuance_policy;
	cert_chain_config->dwUrlRetrievalTimeout = 0; //Default Revocation Timeout
	cert_chain_config->fCheckRevocationFreshnessTime = true; //Fetch a new CRL if the current one is older than dwRevocationFreshnessTime
	cert_chain_config->dwRevocationFreshnessTime = 86400; //seconds the CRL set should last - currently one day
	cert_chain_config->pftCacheResync = NULL;// Do not expire cached values
	cert_chain_config->pStrongSignPara = NULL;//Do not specify stong algorithms specifically
	cert_chain_config->dwStrongSignFlags = 0; //Do not ignore short public key length of last certificate

	cert_chain_engine_config = new CERT_CHAIN_ENGINE_CONFIG;
	cert_chain_engine_config->cbSize = sizeof(*cert_chain_engine_config);
	cert_chain_engine_config->hRestrictedRoot = NULL; //Use the whole root store
	cert_chain_engine_config->hRestrictedTrust = NULL; //Use the whole trusted store
	cert_chain_engine_config->hRestrictedOther = NULL; //No other restrictions on acceptable cert stores
	cert_chain_engine_config->cAdditionalStore = 0; // Zero extra certificate stores
	cert_chain_engine_config->rghAdditionalStore = NULL; //No extra certificate store
	cert_chain_engine_config->dwFlags = 0x00000000; // No flags
	cert_chain_engine_config->dwUrlRetrievalTimeout = 0;
	cert_chain_engine_config->MaximumCachedCertificates = 0;
	cert_chain_engine_config->CycleDetectionModulus = 0;
	cert_chain_engine_config->hExclusiveRoot = NULL;
	cert_chain_engine_config->hExclusiveTrustedPeople = NULL;
	cert_chain_engine_config->dwExclusiveFlags = 0x00000000;

	cert_chain_engine_config = new CERT_CHAIN_ENGINE_CONFIG();
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
	cert_chain_engine_config->hExclusiveRoot = openRootStore();
	cert_chain_engine_config->hExclusiveTrustedPeople = NULL;//openMyStore();
	cert_chain_engine_config->dwExclusiveFlags = 0x00000000;
}

UnbreakableCrypto_RESPONSE UnbreakableCrypto::evaluate(UINT8 * cert_data, DWORD cert_len, char * hostname){

	if (!isConfigured()) {
		thlog() << "WARNING! Using UnbreakableCrypto without configuring.";
		return UnbreakableCrypto_ERROR;
	}

	//++++++++++++++++++++++++++++++++++++
	//Create a windows certificate object
	//++++++++++++++++++++++++++++++++++++
	PCCERT_CONTEXT certificate_context = CertCreateCertificateContext(
		encodings, 
		cert_data,
		cert_len // Possible error when raw chain is longer than INT_MAX
	);
	if (certificate_context == NULL) {
		thlog() << "Wincrypt could not parse certificate. Error Code " << GetLastError();
		return UnbreakableCrypto_REJECT;
	}

	//++++++++++++++++++++++++++++++++++++++++++++
	//Create a Windows Certificate Chain Engine
	//++++++++++++++++++++++++++++++++++++++++++++
	if (!CertCreateCertificateChainEngine(
				cert_chain_engine_config, 
				&authentication_train_handle
			)
		) 
	{
		thlog() << "Wincrypt could not create authentiation train Error Code: " << GetLastError();
		CertFreeCertificateContext(certificate_context);
		return UnbreakableCrypto_REJECT;
	}

	//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	//Turn certificate object into a certificate chain object
	//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	if (!CertGetCertificateChain(
				authentication_train_handle,
				certificate_context,
				NULL, //Uses System time by default
				NULL, //Search no extra certificate stores
				cert_chain_config,
				0x0000000, // No Flags
				NULL,
				&cert_chain_context
			)
		) 
	{
		thlog() << "Wincrypt could not create authentiation train Error Code: " << GetLastError();
		CertFreeCertificateContext(certificate_context); certificate_context = NULL;
		CertFreeCertificateChainEngine(authentication_train_handle); authentication_train_handle = NULL;
		return UnbreakableCrypto_REJECT;
	}

	//++++++++++++++++++++++++++++++++++
	//Validate the certificate Chain
	//++++++++++++++++++++++++++++++++++

	PCERT_CHAIN_POLICY_STATUS cert_policy_status = new CERT_CHAIN_POLICY_STATUS;
	if (!CertVerifyCertificateChainPolicy(
			CERT_CHAIN_POLICY_NT_AUTH,
			cert_chain_context,
			0, //Do not ignore any problems
			cert_policy_status
		)
	) 
	{
		thlog() << "Wincrypt could not policy check the certificate chain. Error Code: " << GetLastError();
		CertFreeCertificateChain(cert_chain_context);
		CertFreeCertificateContext(certificate_context); certificate_context = NULL;
		CertFreeCertificateChainEngine(authentication_train_handle); authentication_train_handle = NULL;
		delete cert_policy_status;
		return UnbreakableCrypto_REJECT;
	}
	if (cert_policy_status->dwError != ERROR_SUCCESS) 
	{
		CertFreeCertificateChain(cert_chain_context);
		CertFreeCertificateContext(certificate_context); certificate_context = NULL;
		CertFreeCertificateChainEngine(authentication_train_handle); authentication_train_handle = NULL;
		delete cert_policy_status;
		return UnbreakableCrypto_REJECT;
	}

	CertFreeCertificateChain(cert_chain_context);
	CertFreeCertificateContext(certificate_context); certificate_context = NULL;
	CertFreeCertificateChainEngine(authentication_train_handle); authentication_train_handle = NULL;
	delete cert_policy_status;
	return UnbreakableCrypto_ACCEPT;
}


UnbreakableCrypto_RESPONSE UnbreakableCrypto::evaluate(Query * cert_data) {
	char * hostname = SNI_Parser::sni_get_hostname(cert_data->data.client_hello, cert_data->data.client_hello_len);
	UnbreakableCrypto_RESPONSE answer = evaluate(cert_data->data.raw_chain, cert_data->data.client_hello_len, hostname);
	delete hostname;
	return answer;
}

/*
HCERTSTORE WINAPI CertOpenStore(
_In_       LPCSTR            lpszStoreProvider,
_In_       DWORD             dwMsgAndCertEncodingType,
_In_       HCRYPTPROV_LEGACY hCryptProv,
_In_       DWORD             dwFlags,
_In_ const void              *pvPara
);
*/

bool UnbreakableCrypto::insertIntoRootStore(PCCERT_CONTEXT certificate)
{
	HCERTSTORE root_store = openRootStore();

	if (!CertAddCertificateContextToStore(root_store, certificate, CERT_STORE_ADD_NEW, NULL))
	{
		CertCloseStore(root_store, CERT_CLOSE_STORE_FORCE_FLAG);
		return GetLastError() == CRYPT_E_EXISTS;
	}

	CertCloseStore(root_store, CERT_CLOSE_STORE_FORCE_FLAG);
	return true;
}

bool UnbreakableCrypto::removeFromRootStore(Query * query)
{
	HCERTSTORE root_store = openRootStore();
	PCCERT_CONTEXT cert_to_destroy = CertCreateCertificateContext(encodings, query->data.raw_chain, query->data.raw_chain_len);
	


	if (!CertDeleteCertificateFromStore(cert_to_destroy))
	{
		CertCloseStore(root_store, CERT_CLOSE_STORE_FORCE_FLAG);
		return false;
	}


	CertCloseStore(root_store, CERT_CLOSE_STORE_FORCE_FLAG);
	return false;
}

bool UnbreakableCrypto::isConfigured()
{
	bool encoding_valid = true;
	if (encodings != X509_ASN_ENCODING  && encodings != (X509_ASN_ENCODING | PKCS_7_ASN_ENCODING)) {
		thlog() << "Unknown Encoding configured in UnbreakableCrypto";
		encoding_valid = false;
	}
	
	bool configs =
		cert_chain_engine_config != NULL &&
		empty_enhkey != NULL &&
		cert_chain_config != NULL &&
		chain_params_requested_issuance_policy != NULL &&
		chain_params_requested_use != NULL;

	//bool handles =
	//	authentication_train_handle != NULL &&
	//	(*authentication_train_handle) != NULL &&
	//	cert_chain_context != NULL &&
	//	(*cert_chain_context) != NULL;


	return encoding_valid && configs;
}

HCERTSTORE UnbreakableCrypto::openRootStore()
{
	auto target_store_name = L"Root";
	return CertOpenStore(
		CERT_STORE_PROV_SYSTEM,
		0,
		NULL,
		CERT_SYSTEM_STORE_CURRENT_USER,
		target_store_name
	);
}

HCERTSTORE UnbreakableCrypto::openMyStore()
{
	auto target_store_name = L"MY";
	return CertOpenStore(
		CERT_STORE_PROV_SYSTEM,
		0,
		NULL,
		CERT_SYSTEM_STORE_CURRENT_USER,
		target_store_name
	);
}


