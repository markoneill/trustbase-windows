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
	encodings = (X509_ASN_ENCODING);//No Idea

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
	char * hostname = SNI_Parser::sni_get_hostname(cert_data->data.client_hello, cert_data->data.client_hello_len);
	wchar_t* wHostname = GetWC(hostname);
	delete hostname;

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
	UnbreakableCrypto_RESPONSE answer = evaluateChain(cert_data->data.cert_context_chain, wHostname);
	//UnbreakableCrypto_RESPONSE answer = evaluate(leaf_cert_context, wHostname);
	delete wHostname;
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
	bool successfulAdd= true;
	bool alreadyExists = false;

	HCERTSTORE root_store = openRootStore();

	LPTSTR certName = getCertName(certificate);
	std::wstring ws(certName);
	thlog() << "Attempting to add the following cert to the root store: " << certName;
	free(certName);

	if (!CertAddCertificateContextToStore(root_store, certificate, CERT_STORE_ADD_NEW, NULL))
	{
		successfulAdd = false;
		alreadyExists = GetLastError() == CRYPT_E_EXISTS;
		if (!alreadyExists)
		{
			thlog(LOG_WARNING) << "Failed adding cert to root store";
		}
	}

	if (successfulAdd)
	{
		CRYPT_HASH_BLOB* sha1_blob = getSHA1CryptHashBlob(certificate->pbCertEncoded, certificate->cbCertEncoded);
		std::string thumbprint ((char*)sha1_blob->pbData, sha1_blob->cbData);
		certsAddedToRootStore.addCertificate(thumbprint);
	}
	
	CertCloseStore(root_store, CERT_CLOSE_STORE_FORCE_FLAG);
	return successfulAdd || alreadyExists;
}

LPTSTR UnbreakableCrypto::getCertName(PCCERT_CONTEXT certificate) {
	DWORD cbSize = 0;

	//get size
	cbSize = CertGetNameString(
		certificate,
		CERT_NAME_SIMPLE_DISPLAY_TYPE,
		CERT_NAME_ISSUER_FLAG,
		NULL,
		NULL,
		cbSize);

	LPTSTR pszName = new TCHAR[cbSize];
	//get name
	cbSize = CertGetNameString(
		certificate,
		CERT_NAME_SIMPLE_DISPLAY_TYPE,
		CERT_NAME_ISSUER_FLAG,
		NULL,
		pszName,
		cbSize);

	return pszName;
}


bool UnbreakableCrypto::removeAllStoredCertsFromRootStore()
{
	bool success = true;
	while (certsAddedToRootStore.certificates.size() > 0)
	{
		if (!removeFromRootStore(certsAddedToRootStore.certificates.at(0))) {
			success = false;
		}
	}

	return success;
}

bool UnbreakableCrypto::removeFromRootStore(std::string thumbprint)
{
	//Search for certificates by SHA1 thumbprint (CertFindCertificateInStore w/ CERT_FIND_HASH does this)
	//then remove any certificates (presumably only 1 cert) with this hash.

	CRYPT_HASH_BLOB* sha1_blob = getSHA1CryptHashBlob(thumbprint);
	bool success = removeFromRootStore(sha1_blob);

	delete sha1_blob->pbData;
	delete sha1_blob;

	return success;
}

bool UnbreakableCrypto::removeFromRootStore(byte* raw_cert, size_t raw_cert_len)
{
	//Search for certificates by SHA1 thumbprint (CertFindCertificateInStore w/ CERT_FIND_HASH does this)
	//then remove any certificates (presumably only 1 cert) with this hash.

	CRYPT_HASH_BLOB* sha1_blob = getSHA1CryptHashBlob(raw_cert, raw_cert_len);
	bool success = removeFromRootStore(sha1_blob);

	delete sha1_blob->pbData;
	delete sha1_blob;

	return success;
}

bool UnbreakableCrypto::removeFromRootStore(CRYPT_HASH_BLOB* sha1_blob)
{
	bool success = true;
	HCERTSTORE root_store = openRootStore();
	PCCERT_CONTEXT pCertContext = NULL;

	while (pCertContext = CertFindCertificateInStore(root_store, encodings, 0, CERT_FIND_HASH, sha1_blob, pCertContext)) {
		if (pCertContext != NULL)
		{
			thlog() << "Attempting to remove " << pCertContext->pCertInfo->Subject.pbData << " cert from root store";

			if (!CertDeleteCertificateFromStore(
				CertDuplicateCertificateContext(pCertContext))
				)
			{
				thlog(LOG_WARNING) << "Failed removing cert from root store";
				success = false;
			}
		}
	}
	
	if (success)
	{
		std::string thumbprint((char*)sha1_blob->pbData, sha1_blob->cbData);
		certsAddedToRootStore.removeCertificate(thumbprint);
	}

	CertCloseStore(root_store, CERT_CLOSE_STORE_FORCE_FLAG);
	return success;
}

CRYPT_HASH_BLOB* UnbreakableCrypto::getSHA1CryptHashBlob(byte* raw_cert, size_t raw_cert_len)
{
	//todo: dont use bad hash like sha1 thumbprint
	byte* pbComputedHash = new byte[20];
	DWORD *pcbComputedHash = new DWORD;
	(*pcbComputedHash) = 20;
	CryptHashCertificate(NULL, 0, 0, raw_cert, raw_cert_len, pbComputedHash, pcbComputedHash);
	CRYPT_HASH_BLOB* blob = new CRYPT_HASH_BLOB;
	blob->pbData = pbComputedHash;
	blob->cbData = *pcbComputedHash;
	delete pcbComputedHash;
	return blob;
}

CRYPT_HASH_BLOB* UnbreakableCrypto::getSHA1CryptHashBlob(std::string thumbprint)
{
	//todo: dont use bad hash like sha1 thumbprint
	byte* pbComputedHash = new byte[thumbprint.size()];
	memcpy(pbComputedHash, thumbprint.c_str(), thumbprint.size());
	CRYPT_HASH_BLOB* blob = new CRYPT_HASH_BLOB;
	blob->pbData = pbComputedHash;
	blob->cbData = thumbprint.size();
	return blob;
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

UnbreakableCrypto_RESPONSE UnbreakableCrypto::evaluateChain(std::vector<PCCERT_CONTEXT>* cert_context_chain, LPWSTR wHostname)
{
	size_t cert_count;
	int i;
	PCCERT_CONTEXT current_cert;
	PCCERT_CONTEXT proof_cert;

	//Get number of certs in chain
	cert_count  = cert_context_chain->size();

	//First check hostname on leaf cert
	if (!checkHostname(cert_context_chain->at(cert_count - 1), wHostname)) {
		return UnbreakableCrypto_REJECT;
	}


	CERT_REVOCATION_STATUS foo = CERT_REVOCATION_STATUS();
	foo.cbSize = sizeof(CERT_REVOCATION_STATUS);
	//Now check all certificates' revocation status
	if (!CertVerifyRevocation(
		X509_ASN_ENCODING,
		CERT_CONTEXT_REVOCATION_TYPE,
		cert_count,
		cert_context_chain->_Myfirst,
		CERT_VERIFY_CACHE_ONLY_BASED_REVOCATION,
		NULL,
		&foo
	)) 
	{
		thlog() << "Revoked certificate was encountered";
		return UnbreakableCrypto_REJECT;
	}

	//Loop through chain from root to leaf to validate the chain
	for (i = cert_count - 1; i >= 0; i--) {
		current_cert = cert_context_chain->at(i);

		if (i == cert_count - 1) {
			if (!CheckAgainstRootStore(current_cert)) {
				return UnbreakableCrypto_REJECT;
			}
			continue;
		}

		proof_cert = cert_context_chain->at(i + 1);
		if (!ValidVouching(current_cert, proof_cert)) {

		}
	}

	return UnbreakableCrypto_ACCEPT;
}

/*Checks whether a PCCERT_CONTEXT has an exact match in the root store*/
bool UnbreakableCrypto::CheckAgainstRootStore(PCCERT_CONTEXT cert)
{
	HCERTSTORE root_store;
	bool answer;
	//Open Root store
	root_store = openRootStore();

	//Try to find a match in root store
	PCCERT_CONTEXT root_cert = CertFindCertificateInStore(
		root_store,
		X509_ASN_ENCODING,
		0,
		CERT_FIND_EXISTING,
		cert,
		NULL
	);

	answer = (root_cert != NULL);
	if (answer) {
		CertFreeCertificateContext(root_cert);
	}

	answer = answer && (CertVerifyTimeValidity(NULL, cert->pCertInfo) == 0);

	CertCloseStore(root_store, CERT_CLOSE_STORE_FORCE_FLAG);
	return answer;
}

/*Checks whether a PCCERT_CONTEXT can be trusted based on the previous PCCERT_CONTEXT in the chain*/
bool UnbreakableCrypto::ValidVouching(PCCERT_CONTEXT claimed_cert, PCCERT_CONTEXT trusted_proof)
{
	CRYPT_BIT_BLOB trusted_public_key_blob;
	CRYPT_BIT_BLOB claimed_signature;
	if (!(CertVerifyTimeValidity(NULL, claimed_cert->pCertInfo) == 0)) {
		return false; //This certificate is not valid at this time.
	}

	DWORD validation_code = CERT_STORE_SIGNATURE_FLAG;
	CertVerifySubjectCertificateContext(claimed_cert, trusted_proof, &validation_code);
	if (validation_code != 0) {
		return false;
	}

	return true;
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
wchar_t * UnbreakableCrypto::GetWC(const char *c)
{
	const size_t cSize = strlen(c) + 1;
	wchar_t* wc = new wchar_t[cSize];
	mbstowcs(wc, c, cSize);

	return wc;
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

