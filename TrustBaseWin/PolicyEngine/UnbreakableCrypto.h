#pragma once
#include "TBLogger.h"
#include "Query.h"
#include <vector>
#include <iostream>
#include "CertificatesAddedToRootStore.h"

/*
	Note: This crypto is very working and is much safe.
*/

enum UnbreakableCrypto_RESPONSE {UnbreakableCrypto_REJECT=0, UnbreakableCrypto_ACCEPT=1, UnbreakableCrypto_ERROR=2};

class UnbreakableCrypto {
public:
	UnbreakableCrypto();
	~UnbreakableCrypto();

	/*
	This function includes configuation settings to various wincrypt structs objects

	This function may be extended later to use parameters
	to change how these configuration objects are instantiated.
	That is why they are not in the constructor.
	*/
	void configure();
	
	/*
	Allows you to check if configure() has been run on this object yet.
	It must be run at least once or errors will occur.
	*/
	bool isConfigured();

	/*
	Evaluates a Query's certificate chain
	1. Gets the hostname from client hello
	2. Verifiy there is at least one certificate in the chain
	3. Evaluate the chain of certificates for errors or problems (run evaluateChain)

	returns an UnbreakableCrypto_RESPONSE:
	UnbreakableCrypto_REJECT
	UnbreakableCrypto_ACCEPT
	UnbreakableCrypto_ERROR
	*/
	UnbreakableCrypto_RESPONSE evaluate (Query * certificate_data);
	
	/*
	Attempts to add a certificate to the windows root store
	If a certificate is added to the root store, the certificates thumbprint is recorded onto a file (certsAddedToRootStore)
	*/
	bool insertIntoRootStore(PCCERT_CONTEXT certificate);
	
	/*
	Search for certificates by SHA1 thumbprint (CertFindCertificateInStore w/ CERT_FIND_HASH does this)
	then remove any certificates (presumably only 1 cert) with this hash.
	*/
	bool removeFromRootStore(std::string thumbprint);
	
	/*
	Search for certificates by the raw_cert bytes (CertFindCertificateInStore w/ CERT_FIND_HASH does this)
	then remove any certificates (presumably only 1 cert) with this hash.
	*/
	bool removeFromRootStore(byte* raw_cert, size_t raw_cert_len);
	
	/*
	Attempts to remove all certificates added to the root store that are remembered in the
	certsAddedToRootStore method.
	*/
	bool removeAllStoredCertsFromRootStore();
	
	/*
	Evaluates a certificate chain for errors or problems. 
	This function really needs to work correctly.
	
	1. Tests the certificate chain for null values.
	2. Test: Reject invalid Hostname
	3. Test: Reject revocation knowleage (looked at cashed information)
	4. Test: Leaf cert must be traceable back to a root cert
	5. Test: Verify all non-leaf certificates has a CA flag set
	*/
	UnbreakableCrypto_RESPONSE evaluateChain(std::vector<PCCERT_CONTEXT>* cert_context_chain, char * hostname);

private:

	/*
	Returns a handle to the root store
	*/
	HCERTSTORE openRootStore();
	/*
	Returns a handle to the Windows MY store
	*/
	HCERTSTORE openMyStore();
	/*
	Returns a handle to the intermediate CA store
	*/
	HCERTSTORE openIntermediateCAStore();

	/*
	Returns true if wincrypt's "CertVerifyCertificateChainPolicy" function returns true without errors
	*/
	bool ValidateWithRootStore(PCCERT_CONTEXT cert);

	/*
	Private version of removeFromRootStore which actually removes the certificate from the root store.
	Removes a certificate from the Windows root store specified by a sha1 hash
	*/
	bool removeFromRootStore(CRYPT_HASH_BLOB* sha1_blob);

	/*
	Returns true if the certificate chain has null values
	*/
	bool evaluateContainsNullCertificates(std::vector<PCCERT_CONTEXT>* cert_context_chain);
	
	/*
	Returns false if any of the certificates hostname are invalid
	*/
	bool evaluateHostname(std::vector<PCCERT_CONTEXT>* cert_context_chain, char * hostname);
	
	/*
	Returns true if any of the certificates has been revoked.
	This function only checks local cashes, in otherwords if windows has a cashed version of this revocation status.
	*/
	bool evaluateLocalRevocation(std::vector<PCCERT_CONTEXT>* cert_context_chain);

	/*
	Returns true if the leaf certificate is be traceable back to a root certificate.
	*/
	bool evaluateChainVouching(std::vector<PCCERT_CONTEXT>* cert_context_chain);
	
	/*
	Returns true if all non-leaf certificates have the isCA flag set.
	*/
	bool evaluateIsCa(std::vector<PCCERT_CONTEXT>* cert_context_chain);
	
	/*
	Returns true if there is a duplicate certificate in the certificate chain.
	*/
	bool isDuplicateCertificate(PCCERT_CONTEXT cert1, PCCERT_CONTEXT cert2);

	/*
	Converts the hostname to a wildcard verison. Example www.google.com -> *.google.com
	*/
	char* convertHostnameToWildcard(char* hostname);

	/*
	Calculates a sha1 hash of a certificate
	*/
	CRYPT_HASH_BLOB* getSHA1CryptHashBlob(byte* raw_cert, size_t raw_cert_len);
	
	/*
	Turn a Sha-1 thumbprint string into a comparable CRYPTO_HASH_BLOB
	*/
	CRYPT_HASH_BLOB* getSHA1CryptHashBlob(std::string thumbprint);
	
	/*
	Gets the name of a certificate as a LPTSTR
	*/
	LPTSTR getCertName(PCCERT_CONTEXT certificate);

	/*
	Security Programming Cookbook for C and C++
	by: John Viega and Matt Messier
	Copyright © 2003 O'Reilly Media, Inc.
	Used with permission
	*/
	bool UnbreakableCrypto::checkHostname(PCCERT_CONTEXT pCertContext, LPWSTR lpszHostName);
	LPWSTR SPC_fold_wide(LPWSTR str);
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