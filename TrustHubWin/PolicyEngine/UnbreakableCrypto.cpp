#include "stdafx.h"
#include "UnbreakableCrypto.h"
#include <vector>



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
		CertCloseStore(cert_chain_engine_config->hExclusiveRoot, CERT_CLOSE_STORE_CHECK_FLAG);
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

UnbreakableCrypto_RESPONSE UnbreakableCrypto::evaluate(UINT8 * cert_data, DWORD cert_len, LPWSTR hostname){

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

	UnbreakableCrypto_RESPONSE answer = evaluate(certificate_context, hostname);
	CertFreeCertificateContext(certificate_context); certificate_context = NULL;
	return answer;

}

UnbreakableCrypto_RESPONSE UnbreakableCrypto::evaluate(Query * cert_data) {
	WCHAR * hostname = (WCHAR*)SNI_Parser::sni_get_hostname(cert_data->data.client_hello, cert_data->data.client_hello_len);
	
	if (cert_data->data.cert_context_chain->size() <= 0) {
		thlog() << "No PCCERT_CONTEXT in chain";
		return UnbreakableCrypto_REJECT;
	}
	int left_cert_index = 0;
	int root_cert_index = cert_data->data.cert_context_chain->size() - 1;

	PCCERT_CONTEXT leaf_cert_context = cert_data->data.cert_context_chain->at(left_cert_index);
	//PCCERT_CONTEXT root_raw_cert = cert_data->data.cert_context_chain->at(root_cert_index);

	if (leaf_cert_context == NULL) {
		thlog() << "cert_context at index " << left_cert_index << "is NULL";
		return UnbreakableCrypto_REJECT;
	}
	UnbreakableCrypto_RESPONSE answer = evaluate(leaf_cert_context, hostname);
	
	delete hostname;
	CertFreeCertificateContext(leaf_cert_context); leaf_cert_context = NULL;
	return answer;
}

UnbreakableCrypto_RESPONSE UnbreakableCrypto::evaluate(PCCERT_CONTEXT certificate_context, LPWSTR hostname) {
	if (!isConfigured()) {
		thlog() << "WARNING! Using UnbreakableCrypto without configuring.";
		return UnbreakableCrypto_ERROR;
	}

	//++++++++++++++++++++++++++++++++++++
	//Verify the windows certificate object
	//++++++++++++++++++++++++++++++++++++
	
	if (certificate_context == NULL) {
		thlog() << "Wincrypt could not parse certificate. Error Code " << GetLastError();
		return UnbreakableCrypto_REJECT;
	}
	//++++++++++++++++++++++++++++++++++++
	//Verify the hostname
	//++++++++++++++++++++++++++++++++++++

	if (!checkHostname(certificate_context, hostname)) {
		thlog() << "Invalid Hostname Rejected";
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
		CertFreeCertificateChainEngine(authentication_train_handle); authentication_train_handle = NULL;
		return UnbreakableCrypto_REJECT;
	}

	//++++++++++++++++++++++++++++++++++
	//Validate the certificate Chain
	//++++++++++++++++++++++++++++++++++

	CERT_CHAIN_POLICY_PARA chain_policy;
	chain_policy.cbSize = sizeof(CERT_CHAIN_POLICY_PARA);
	chain_policy.dwFlags = 0;

	CERT_CHAIN_POLICY_STATUS cert_policy_status;
	if (!CertVerifyCertificateChainPolicy(
			CERT_CHAIN_POLICY_BASE,
			cert_chain_context,
			&chain_policy, //Do not ignore any problems
			&cert_policy_status
			)
		)
	{
		thlog() << "Wincrypt could not policy check the certificate chain. Error Code: " << GetLastError();
		CertFreeCertificateChain(cert_chain_context); cert_chain_context = NULL;
		CertFreeCertificateChainEngine(authentication_train_handle); authentication_train_handle = NULL;
		//delete cert_policy_status;
		return UnbreakableCrypto_REJECT;
	}
	if (cert_policy_status.dwError != ERROR_SUCCESS) 
	{
		CertFreeCertificateChain(cert_chain_context); cert_chain_context = NULL;
		CertFreeCertificateChainEngine(authentication_train_handle); authentication_train_handle = NULL;
		//delete cert_policy_status;
		return UnbreakableCrypto_REJECT;
	}

	CertFreeCertificateChain(cert_chain_context); cert_chain_context = NULL;
	CertFreeCertificateChainEngine(authentication_train_handle); authentication_train_handle = NULL;
	//delete cert_policy_status;
	return UnbreakableCrypto_ACCEPT;
}

unsigned int UnbreakableCrypto::ntoh24(const UINT8* data) {
	unsigned int ret = (data[0] << 16) | (data[1] << 8) | data[2];
	return ret;
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


/*
These last 3 hostname validation function are from John Viega and Matt Messier's <b>Secure Programming Cookbook</b>
Published in 2003 by O'Reily and Associates Inc. Edited by Deborah Russell.
*/
bool UnbreakableCrypto::checkHostname(PCCERT_CONTEXT pCertContext, LPWSTR lpszHostName) {
	BOOL bResult = false;
	DWORD cbStructInfo, dwCommonNameLength, i;
	LPSTR szOID;
	LPVOID pvStructInfo;
	LPWSTR lpszCommonName, lpszDNSName, lpszTemp;
	CERT_EXTENSION * pExtension;
	CERT_ALT_NAME_INFO * pNameInfo;

	//Try SUBJECT_ALT_NAME2 first - it supercedes SUBJECT_ALT_NAME
	szOID = szOID_SUBJECT_ALT_NAME2;
	pExtension = CertFindExtension(szOID, pCertContext->pCertInfo->cExtension, pCertContext->pCertInfo->rgExtension);

	if (!pExtension) {
		szOID = szOID_SUBJECT_ALT_NAME;
		pExtension = CertFindExtension(szOID, pCertContext->pCertInfo->cExtension, pCertContext->pCertInfo->rgExtension);
	}

	if (pExtension && CryptDecodeObject(
							X509_ASN_ENCODING, 
							szOID,
							pExtension->Value.pbData, 
							pExtension->Value.cbData, 
							0, 
							0, 
							&cbStructInfo
					)
		) 
	{
		if ((pvStructInfo = LocalAlloc(LMEM_FIXED, cbStructInfo)) != 0) 
		{
			CryptDecodeObject(
				X509_ASN_ENCODING, 
				szOID, pExtension->Value.pbData, 
				pExtension->Value.cbData, 
				0, 
				pvStructInfo, 
				&cbStructInfo
			);
			pNameInfo = (CERT_ALT_NAME_INFO *) pvStructInfo;

			for (i = 0; !bResult && i < pNameInfo->cAltEntry; i++) {
				if (pNameInfo->rgAltEntry[i].dwAltNameChoice == CERT_ALT_NAME_DNS_NAME) 
				{
					if (!(lpszDNSName = SPC_fold_wide(pNameInfo->rgAltEntry[i].pwszDNSName))) 
					{
						break;
					}
					if (CompareStringW(LOCALE_USER_DEFAULT, NORM_IGNORECASE, lpszDNSName, -1, lpszHostName, -1) == CSTR_EQUAL)
					{
						bResult = TRUE;
					}
					LocalFree(lpszDNSName);
				}
			}
			LocalFree(pvStructInfo);
			//LocalFree(lpszHostName);
			return bResult;
		}
	}

	/*No Subject AltName Extension -- check CommonName*/
	dwCommonNameLength = CertGetNameStringW(pCertContext, CERT_NAME_ATTR_TYPE, 0, szOID_COMMON_NAME, 0, 0);

	if (!dwCommonNameLength) {
		LocalFree(lpszHostName);
		return FALSE;
	}

	lpszTemp = (LPWSTR)LocalAlloc(LMEM_FIXED, dwCommonNameLength * sizeof(WCHAR));

	if (lpszTemp) {
		CertGetNameStringW(pCertContext, CERT_NAME_ATTR_TYPE, 0, szOID_COMMON_NAME, lpszTemp, dwCommonNameLength);
		if ((lpszCommonName = SPC_fold_wide(lpszTemp)) != 0) {
			if (CompareStringW(LOCALE_USER_DEFAULT, NORM_IGNORECASE, lpszCommonName, -1, lpszHostName, -1) == CSTR_EQUAL) {
				bResult = TRUE;
			}
			LocalFree(lpszCommonName);
		}
		LocalFree(lpszTemp);
	}
	LocalFree(lpszHostName);
	return bResult;
}

LPWSTR UnbreakableCrypto::SPC_fold_wide(LPWSTR str)
{
	int len;
	LPWSTR wstr;

	if (!(len = FoldStringW(MAP_PRECOMPOSED, str, -1, 0, 0))) return NULL;
	if (!(wstr = (LPWSTR)LocalAlloc(LMEM_FIXED, len * sizeof(WCHAR)))) return NULL;
	if (!FoldStringW(MAP_PRECOMPOSED, str, -1, wstr, len)) {
		LocalFree(wstr);
		return NULL;
	}

	return wstr;
}


//LPWSTR UnbreakableCrypto::SPC_make_wide(LPCTSTR str)
//{
//#ifndef UNICODE
//	int len;
//	LPWSTR wstr;
//
//	if (!(len = MultiByteToWideChar(CP_UTF8, 0, str, -1, 0, 0))) return NULL;
//	if (!(wstr = (LPWSTR)LocalAlloc(LMEM_FIXED, len * sizeof(WCHAR)))) return NULL;
//	if (!MultiByteToWideChar(CP_UTF8, 0, str, -1, 0, 0)) {
//		LocalFree(wstr);
//		return NULL;
//	}
//
//	return wstr;
//#else
//	return SPC_fold_wide(str);
//#endif
//}

