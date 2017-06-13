#include "stdafx.h"
#include "UnbreakableCrypto.h"
#include <vector>

UnbreakableCrypto::UnbreakableCrypto() {

}

/*
Free Memory
*/
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


/*
UnbreakableCrypto::configure()

This function may be extended later to use parameters
to change how these configuration objects are instantiated.
That is why they are not in the constructor.
*/
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

	cert_chain_engine_config = new CERT_CHAIN_ENGINE_CONFIG();
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
	cert_chain_engine_config->hExclusiveRoot = openRootStore();
	cert_chain_engine_config->hExclusiveTrustedPeople = NULL;//openMyStore();
	cert_chain_engine_config->dwExclusiveFlags = 0x00000000;
}

/*
Evaluates a Query's certificate chain
*/
UnbreakableCrypto_RESPONSE UnbreakableCrypto::evaluate(Query * cert_data) {
	char * hostname = SNI_Parser::sni_get_hostname(cert_data->data.client_hello, cert_data->data.client_hello_len);
	//LPWSTR lHostname = convert_CStr_to_LPWSTR(hostname);

	if (cert_data->data.cert_context_chain->size() <= 0) {
		tblog() << "No PCCERT_CONTEXT in chain";
		return UnbreakableCrypto_REJECT;
	}
	int left_cert_index = 0;
	int root_cert_index = cert_data->data.cert_context_chain->size() - 1;

	PCCERT_CONTEXT leaf_cert_context = cert_data->data.cert_context_chain->at(left_cert_index);
	//LocalFree(lHostname);

	if (leaf_cert_context == NULL) {
		tblog() << "cert_context at index " << left_cert_index << "is NULL";
		delete hostname;
		return UnbreakableCrypto_REJECT;
	}
	//delete wHostname;

	UnbreakableCrypto_RESPONSE answer = evaluateChain(cert_data->data.cert_context_chain, hostname);
	delete hostname;

	return answer;
}

/*
Attempts to add a certificate to the windows root store
*/
bool UnbreakableCrypto::insertIntoRootStore(PCCERT_CONTEXT certificate)
{
	bool successfulAdd = true;
	bool alreadyExists = false;

	HCERTSTORE root_store = openRootStore();

	LPTSTR certName = getCertName(certificate);
	std::wstring ws(certName);
	tblog() << "Attempting to add the following cert to the root store: " << certName;
	free(certName);

	if (!CertAddCertificateContextToStore(root_store, certificate, CERT_STORE_ADD_NEW, NULL))
	{
		successfulAdd = false;
		alreadyExists = GetLastError() == CRYPT_E_EXISTS;
		if (!alreadyExists)
		{
			tblog(LOG_WARNING) << "Failed adding cert to root store";
		}
	}

	if (successfulAdd)
	{
		CRYPT_HASH_BLOB* sha1_blob = getSHA1CryptHashBlob(certificate->pbCertEncoded, certificate->cbCertEncoded);
		std::string thumbprint((char*)sha1_blob->pbData, sha1_blob->cbData);
		certsAddedToRootStore.addCertificate(thumbprint);
	}

	//UNCOMMENT THIS TO HELP CLEAN UP YOUR ROOT STORE
	//if (alreadyExists)
	//{
	//	CRYPT_HASH_BLOB* sha1_blob = getSHA1CryptHashBlob(certificate->pbCertEncoded, certificate->cbCertEncoded);
	//	std::string thumbprint((char*)sha1_blob->pbData, sha1_blob->cbData);
	//	removeFromRootStore(thumbprint);
	//}

	CertCloseStore(root_store, CERT_CLOSE_STORE_FORCE_FLAG);
	return successfulAdd || alreadyExists;
}

/*
Gets the name of a certificate as a LPTSTR
*/
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

/*
Attempts to remove all certificates added to the root store that are remembered in the
certsAddedToRootStore method.
*/
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

/*
//Search for certificates by SHA1 thumbprint (CertFindCertificateInStore w/ CERT_FIND_HASH does this)
//then remove any certificates (presumably only 1 cert) with this hash.
*/
bool UnbreakableCrypto::removeFromRootStore(std::string thumbprint)
{
	CRYPT_HASH_BLOB* sha1_blob = getSHA1CryptHashBlob(thumbprint);
	bool success = removeFromRootStore(sha1_blob);

	delete sha1_blob->pbData;
	delete sha1_blob;

	return success;
}

/*
//Search for certificates by SHA1 thumbprint (CertFindCertificateInStore w/ CERT_FIND_HASH does this)
//then remove any certificates (presumably only 1 cert) with this hash.
*/
bool UnbreakableCrypto::removeFromRootStore(byte* raw_cert, size_t raw_cert_len)
{


	CRYPT_HASH_BLOB* sha1_blob = getSHA1CryptHashBlob(raw_cert, raw_cert_len);
	bool success = removeFromRootStore(sha1_blob);

	delete sha1_blob->pbData;
	delete sha1_blob;

	return success;
}

/*
Removes a certificate from the Windows root store specified by a sha1 hash
*/
bool UnbreakableCrypto::removeFromRootStore(CRYPT_HASH_BLOB* sha1_blob)
{
	bool success = true;
	HCERTSTORE root_store = openRootStore();
	PCCERT_CONTEXT pCertContext = NULL;

	while (pCertContext = CertFindCertificateInStore(root_store, encodings, 0, CERT_FIND_HASH, sha1_blob, pCertContext)) {
		if (pCertContext != NULL)
		{
			tblog() << "Attempting to remove " << pCertContext->pCertInfo->Subject.pbData << " cert from root store";

			if (!CertDeleteCertificateFromStore(
				CertDuplicateCertificateContext(pCertContext))
				)
			{
				tblog(LOG_WARNING) << "Failed removing cert from root store";
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

/*
Calculates a sha1 hash of a certificate
*/
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

/*
Turn a Sha-1 thumbprint string into a comparable CRYPTO_HASH_BLOB
*/
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

/*
Allows you to check if configure() has been run on this object yet.
It must be run at least once or errors will occur.
*/
bool UnbreakableCrypto::isConfigured()
{
	bool encoding_valid = true;
	if (encodings != X509_ASN_ENCODING  && encodings != (X509_ASN_ENCODING | PKCS_7_ASN_ENCODING)) {
		tblog() << "Unknown Encoding configured in UnbreakableCrypto";
		encoding_valid = false;
	}

	bool configs =
		cert_chain_engine_config != NULL &&
		empty_enhkey != NULL &&
		cert_chain_config != NULL &&
		chain_params_requested_issuance_policy != NULL &&
		chain_params_requested_use != NULL;

	return encoding_valid && configs;
}

/*
Returns a handle to the root store
*/
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

/*
Returns a handle to the Windows MY store
*/
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
Returns a handle to the intermediate CA store
*/
HCERTSTORE UnbreakableCrypto::openIntermediateCAStore()
{
	auto target_store_name = L"CA";
	return CertOpenStore(
		CERT_STORE_PROV_SYSTEM,
		0,
		NULL,
		CERT_SYSTEM_STORE_CURRENT_USER,
		target_store_name
	);
}

/*
Evaluates a certificate chain. This function really needs to work correctly.
*/
UnbreakableCrypto_RESPONSE UnbreakableCrypto::evaluateChain(std::vector<PCCERT_CONTEXT>* cert_context_chain, char * hostname)
{
	size_t cert_count;
	int i;
	PCCERT_CONTEXT current_cert;
	PCCERT_CONTEXT proof_cert;

	//Return Error if not configured
	if (!isConfigured())
	{
		tblog() << "UnbreakableCrypto was run but not configured";
		return UnbreakableCrypto_ERROR;
	}
	//not null
	if (evaluateContainsNullCertificates(cert_context_chain))
	{
		tblog() << "UnbreakableCrypto_REJECT:  Reject Null Certificates";
		return UnbreakableCrypto_REJECT;
	}

	//Start actual tests

	//Reject invalid Hostname
	bool validHostname = evaluateHostname(cert_context_chain, hostname);
	//Reject revocation knowleage (looked at cashed information)
	bool revokedCertificates = evaluateLocalRevocation(cert_context_chain);
	//leaf cert must be traceable back to a root cert 
	bool chainValidates = evaluateChainVouching(cert_context_chain);
	//verify all non-leaf certificates has a CA flag set
	bool validIsCaFlags = evaluateIsCa(cert_context_chain);

	return validHostname && !revokedCertificates && chainValidates && validIsCaFlags ?UnbreakableCrypto_ACCEPT : UnbreakableCrypto_REJECT;
}
bool UnbreakableCrypto::evaluateContainsNullCertificates(std::vector<PCCERT_CONTEXT>* cert_context_chain) {
	size_t cert_count = cert_context_chain->size();
	for (int i = cert_count - 1; i >= 0; i--) {
		PCCERT_CONTEXT current_cert = cert_context_chain->at(i);
		if (current_cert == NULL) {
			tblog() << "UnbreakableCrypto_REJECT: Loop through chain from root to leaf to validate the chain: cert_context_chain->at(" << i << ") = NULL";
			return true;
		}
	}
	return false;
}
bool UnbreakableCrypto::evaluateHostname(std::vector<PCCERT_CONTEXT>* cert_context_chain, char *hostname) {
	//Reject invalid Hostname
	LPWSTR wHostname = new WCHAR[strlen(hostname) + 1];
	MultiByteToWideChar(CP_OEMCP, 0, hostname, -1, wHostname, strlen(hostname) + 1);

	if (!checkHostname(cert_context_chain->at(0), wHostname))
	{
		char* wildCardHostname = convertHostnameToWildcard(hostname);
		LPWSTR w_WildCardHostname = new WCHAR[strlen(wildCardHostname) + 1];
		MultiByteToWideChar(CP_OEMCP, 0, wildCardHostname, -1, w_WildCardHostname, strlen(wildCardHostname) + 1);

		if (!checkHostname(cert_context_chain->at(0), w_WildCardHostname))
		{
			delete[] wHostname;
			delete[] w_WildCardHostname;
			delete[] wildCardHostname;

			tblog() << "UnbreakableCrypto_REJECT:  Reject invalid Hostname";
			return false;
		}
		delete[] w_WildCardHostname;
		delete[] wildCardHostname;
	}
	delete[] wHostname;
	return true;
}
bool UnbreakableCrypto::evaluateLocalRevocation(std::vector<PCCERT_CONTEXT>* cert_context_chain)
{
	HCERTSTORE rootStore = openRootStore();
	CERT_REVOCATION_STATUS revocation_status = CERT_REVOCATION_STATUS();
	revocation_status.cbSize = sizeof(CERT_REVOCATION_STATUS);
	for (int i = 0; i < cert_context_chain->size() - 1; i++) {

		CERT_REVOCATION_PARA revocation_para;
		revocation_para.cbSize = sizeof(CERT_REVOCATION_PARA);
		revocation_para.cCertStore = 0;
		revocation_para.hCrlStore = rootStore;
		revocation_para.pftTimeToUse = NULL;
		revocation_para.pIssuerCert = cert_context_chain->at(i + 1);
		revocation_para.rgCertStore = NULL;

		if (!CertVerifyRevocation(//TODO Is the return type a good boolean?
			X509_ASN_ENCODING,
			CERT_CONTEXT_REVOCATION_TYPE,
			1,
			(PVOID*) &(cert_context_chain->at(i)),
			CERT_VERIFY_CACHE_ONLY_BASED_REVOCATION,
			&revocation_para,
			&revocation_status
		))
		{
			tblog() << GetLastError();
			const char* reason_text;

			switch (revocation_status.dwError) {
			case CRYPT_E_NO_REVOCATION_CHECK:
				reason_text = "An installed or registered revocation function was not able to do a revocation check on the context.";
				break;
			case CRYPT_E_NO_REVOCATION_DLL:
				reason_text = "No installed or registered DLL was found that was able to verify revocation.";
				break;
			case CRYPT_E_NOT_IN_REVOCATION_DATABASE:
				reason_text = "The context to be checked was not found in the revocation server's database.";
				break;
			case CRYPT_E_REVOCATION_OFFLINE:
				reason_text = "It was not possible to connect to the revocation server.";
				break;
			case CRYPT_E_REVOKED:
				switch (revocation_status.dwReason) {
				case CRL_REASON_UNSPECIFIED:
					reason_text = "The revoking CA gave no explanation. This is discouraged by RFC 2459";
					break;
				case CRL_REASON_KEY_COMPROMISE:
					reason_text = "The CA says their private key was compromised";
					break;
				case CRL_REASON_CA_COMPROMISE:
					reason_text = "The CA says thier own private key was compromised";
					break;
				case CRL_REASON_AFFILIATION_CHANGED:
					reason_text = "The CA says affiliations changed";
					break;
				case CRL_REASON_SUPERSEDED:
					reason_text = "The CA says a new cert supersedes this one";
					break;
				case CRL_REASON_CESSATION_OF_OPERATION:
					reason_text = "The CA says they have shut down operations and can no longer be relied upon";
					break;
				case CRL_REASON_CERTIFICATE_HOLD:
					reason_text = "The CA says this certificate is on hold";
					break;
				}
				tblog() << "Revoked certificate was encountered. reason: " << reason_text;
				return true;
				break;
			case ERROR_SUCCESS:
				reason_text = "The context was good.";
				break;
			case E_INVALIDARG:
				reason_text = "cbSize in pRevStatus is less than sizeof(CERT_REVOCATION_STATUS).Note that dwError in pRevStatus is not updated for this error.";
				break;
			}

			tblog() << "Certificate could not be verified as revoked or not. Accepting certificate. reason: " << reason_text;

		}
	}
	CertCloseStore(rootStore, CERT_CLOSE_STORE_FORCE_FLAG);

	return false;

}
bool UnbreakableCrypto::evaluateChainVouching(std::vector<PCCERT_CONTEXT>* cert_context_chain)
{
	size_t cert_count = cert_context_chain->size();
	for (int i = cert_count - 1; i >= 0; i--) {
		//The first cert should be signed by a root certificate
		if (i == 0) {
			PCCERT_CONTEXT current_cert = cert_context_chain->at(i);
			if (!ValidateWithRootStore(current_cert)) {
				tblog() << "UnbreakableCrypto_REJECT: Loop through chain from root to leaf to validate the chain: ValidateWithRootStore failed";
				return false;
			}
		}
	}
	return true;
}
bool UnbreakableCrypto::evaluateIsCa(std::vector<PCCERT_CONTEXT>* cert_context_chain)
{
	size_t cert_count = cert_context_chain->size();
	for (int i = cert_count - 1; i >= 0; i--) {
	//All certs except the leaf should be in the Intermediate CA store
	//check all certs except the leaf
		if (i == 0) {
			continue;
		}
		PCCERT_CONTEXT current_cert = cert_context_chain->at(i);

		bool isCA = false;
		bool foundConstrant = false;
		for (int ext = 0; ext< current_cert->pCertInfo->cExtension; ext++)
		{
			//look for CA extension
			if (!strcmp(current_cert->pCertInfo->rgExtension[ext].pszObjId, szOID_BASIC_CONSTRAINTS))
			{
				//NOTE: I have never seen szOID_BASIC_CONSTRAINTS found. It always has been the szOID_BASIC_CONSTRAINTS2 
				CERT_BASIC_CONSTRAINTS_INFO *basicContraints;
				DWORD size = sizeof(CERT_BASIC_CONSTRAINTS_INFO);

				CryptDecodeObjectEx(X509_ASN_ENCODING,
					szOID_BASIC_CONSTRAINTS,
					current_cert->pCertInfo->rgExtension[ext].Value.pbData,
					current_cert->pCertInfo->rgExtension[ext].Value.cbData,
					CRYPT_DECODE_ALLOC_FLAG,
					NULL,
					&basicContraints,
					&size);

				//bool isCA
				if (basicContraints->SubjectType.cbData > 0)
				{
					foundConstrant = true;
					isCA = basicContraints->SubjectType.pbData[0] & CERT_CA_SUBJECT_FLAG;
					LocalFree(basicContraints);
					break;
				}

				LocalFree(basicContraints);
			}
			if (!strcmp(current_cert->pCertInfo->rgExtension[ext].pszObjId, szOID_BASIC_CONSTRAINTS2))
			{
				CERT_BASIC_CONSTRAINTS2_INFO *basicContraints;
				DWORD size = sizeof(CERT_BASIC_CONSTRAINTS2_INFO);

				CryptDecodeObjectEx(X509_ASN_ENCODING,
					szOID_BASIC_CONSTRAINTS2,
					current_cert->pCertInfo->rgExtension[ext].Value.pbData,
					current_cert->pCertInfo->rgExtension[ext].Value.cbData,
					CRYPT_DECODE_ALLOC_FLAG,
					NULL,
					&basicContraints,
					&size);


				foundConstrant = true;

				//bool fCA
				isCA = basicContraints->fCA;
				LocalFree(basicContraints);
				break;
			}
		}

		if (foundConstrant && !isCA)
		{
			tblog() << "UnbreakableCrypto_REJECT: All certs except the leaf should be in the Intermediate CA store";
			return false;
		}
		if (!foundConstrant)
		{
			tblog() << "UnbreakableCrypto_REJECT: All certs except the leaf should be in the Intermediate CA store: Cert did not state if it was a CA or not";
			return false;
		}
	}
	return true;
}

char* UnbreakableCrypto::convertHostnameToWildcard(char* hostname)
{
	//find first dot
	int index = 0;
	for (index = 0; index < strlen(hostname); index++)
	{
		if (hostname[index] == '.')
		{
			break;
		}
	}

	if (index == strlen(hostname))
	{
		//fail
		//todo: how to fail nicely
		char* wildcardHostname = new char[1];
		wildcardHostname[0] = 0;
		return wildcardHostname;
	}

	int count = strlen(hostname) - index + 1;
	char* wildcardHostname = new char[count + 1];
	wildcardHostname[0] = '*';

	for (int i = 1; i <= count; i++)
	{
		wildcardHostname[i] = hostname[i + index - 1];
	}

	return wildcardHostname;
}

bool UnbreakableCrypto::ValidateWithRootStore(PCCERT_CONTEXT cert) {
	if (!isConfigured()) {
		tblog() << "WARNING! Using UnbreakableCrypto without configuring.";
		return false;
	}

	//++++++++++++++++++++++++++++++++++++
	//Verify the windows certificate object
	//++++++++++++++++++++++++++++++++++++

	if (cert == NULL) {
		tblog() << "UnbreakableCrypto got a NULL certificate. That should not happen" << GetLastError();
		return false;
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
		tblog() << "Wincrypt could not create authentiation train Error Code: " << GetLastError();
		return false;
	}

	//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	//Turn certificate object into a certificate chain object
	//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	if (!CertGetCertificateChain(
		authentication_train_handle,
		cert,
		NULL, //Uses System time by default
		NULL, //Search no extra certificate stores
		cert_chain_config,
		0x0000000, // No Flags
		NULL,
		&cert_chain_context
	)
		)
	{
		tblog() << "Wincrypt could not create authentiation train Error Code: " << GetLastError();
		CertFreeCertificateChainEngine(authentication_train_handle); authentication_train_handle = NULL;
		return false;
	}

	//++++++++++++++++++++++++++++++++++
	//Validate the certificate Chain
	//++++++++++++++++++++++++++++++++++

	CERT_CHAIN_POLICY_PARA chain_policy;
	chain_policy.cbSize = sizeof(CERT_CHAIN_POLICY_PARA);
	chain_policy.dwFlags = CERT_CHAIN_POLICY_IGNORE_ALL_REV_UNKNOWN_FLAGS;

	CERT_CHAIN_POLICY_STATUS cert_policy_status;
	if (!CertVerifyCertificateChainPolicy(
		CERT_CHAIN_POLICY_BASE,
		cert_chain_context,
		&chain_policy,
		&cert_policy_status
	)
		)
	{
		tblog() << "Wincrypt could not policy check the certificate chain. Error Code: " << GetLastError();
		CertFreeCertificateChain(cert_chain_context); cert_chain_context = NULL;
		CertFreeCertificateChainEngine(authentication_train_handle); authentication_train_handle = NULL;
		//delete cert_policy_status;
		return false;
	}
	if (cert_policy_status.dwError != ERROR_SUCCESS)
	{
		CertFreeCertificateChain(cert_chain_context); cert_chain_context = NULL;
		CertFreeCertificateChainEngine(authentication_train_handle); authentication_train_handle = NULL;
		//delete cert_policy_status;
		return false;
	}

	CertFreeCertificateChain(cert_chain_context); cert_chain_context = NULL;
	CertFreeCertificateChainEngine(authentication_train_handle); authentication_train_handle = NULL;
	//delete cert_policy_status;
	return true;

}

/*
Security Programming Cookbook for C and C++
by: John Viega and Matt Messier
Copyright © 2003 O'Reilly Media, Inc.
Used with permission

Includes everything until the "END ATTRIBUTED CODE" comment
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
			pNameInfo = (CERT_ALT_NAME_INFO *)pvStructInfo;

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
			//We changed this line: we malloc/free lpszHostName outside of this function
			//LocalFree(lpszHostName);
			return bResult;
		}
	}

	/*No Subject AltName Extension -- check CommonName*/
	dwCommonNameLength = CertGetNameStringW(pCertContext, CERT_NAME_ATTR_TYPE, 0, szOID_COMMON_NAME, 0, 0);

	if (!dwCommonNameLength) {
		//We changed this line: we malloc/free lpszHostName outside of this function
		//LocalFree(lpszHostName);
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

	//We changed this line: we malloc/free lpszHostName outside of this function
	//LocalFree(lpszHostName);
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

//END ATTRIBUTED CODE
