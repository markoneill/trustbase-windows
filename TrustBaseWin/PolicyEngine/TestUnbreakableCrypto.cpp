#include "stdafx.h"
#include "TestUnbreakableCrypto.h"

TestUnbreakableCrypto::TestUnbreakableCrypto() {
	QF = new QueryFetcher();
}

TestUnbreakableCrypto::~TestUnbreakableCrypto() {
	delete QF;
}

/*
run test cases
*/
int TestUnbreakableCrypto::Test() {
	int passed = 0;
	int warnings = 0;
	const int total = 8;

	std::cout << std::endl << "TEST 1: UnbreakableCrypto constructor should not fail." << std::endl;
	try {
		if (Test1(&warnings)) {
			std::cout << " **PASS**" << std::endl;
			passed++;
		}
		else {
			std::cout << " **FAIL**" << std::endl;
		}
	}
	catch (std::exception e) {
		std::cout << "-! exception in Test 1: " << e.what() << std::endl;
	}

	std::cout << std::endl << "TEST 2: UnbreakableCrypto configure should instantiate necessary members." << std::endl;
	try {
		if (Test2(&warnings)){
			std::cout << " **PASS**" << std::endl;
			passed++;
		}
		else {
			std::cout << " **FAIL**" << std::endl;
		}
	}
	catch (std::exception e) {
		std::cout << "-! exception in Test 2: " << e.what() << std::endl;
	}

	std::cout << std::endl << "TEST 3: UnbreakableCrypto::evaluate should warn when running without first configuring." << std::endl;
	try {
		if (Test3(&warnings)){
			std::cout << " **PASS**" << std::endl;
			passed++;
		}
		else {
			std::cout << " **FAIL**" << std::endl;
		}
	}
	catch (std::exception e) {
		std::cout << "-! exception in Test 3: " << e.what() << std::endl;
	}

	std::cout << std::endl << "TEST 4: UnbreakableCrypto should accept a valid certificate." << std::endl;
	try {
		if (Test4(&warnings)){
			std::cout << " **PASS**" << std::endl;
			passed++;
		}
		else {
			std::cout << " **FAIL**" << std::endl;
		}
	}
	catch (std::exception e) {
		std::cout << "-! exception in Test 4: " << e.what() << std::endl;
	}

	std::cout << std::endl << "TEST 5: UnbreakableCrypto should reject a valid certificate with the wrong hostname" << std::endl;
	try {
		if (Test5(&warnings)){
			std::cout << " **PASS**" << std::endl;
			passed++;
		}
		else {
			std::cout << " **FAIL**" << std::endl;
		}
	}
	catch (std::exception e) {
		std::cout << "-! exception in Test 5: " << e.what() << std::endl;
	}

	std::cout << std::endl << "TEST 6: UnbreakableCrypto should reject an invalid certificate" << std::endl;
	try {
		if (Test6(&warnings)){
			std::cout << " **PASS**" << std::endl;
			passed++;
		}
		else {
			std::cout << " **FAIL**" << std::endl;
		}
	}
	catch (std::exception e) {
		std::cout << "-! exception in Test 6: " << e.what() << std::endl;
	}

	std::cout << std::endl << "TEST 7: UnbreakableCrypto should reject a mal-formed certificate" << std::endl;
	try {
		if (Test7(&warnings)){
			std::cout << " **PASS**" << std::endl;
			passed++;
		}
		else {
			std::cout << " **FAIL**" << std::endl;
		}
	}
	catch (std::exception e) {
		std::cout << "-! exception in Test 7: " << e.what() << std::endl;
	}


	std::cout << std::endl << "TEST 8: UnbreakableCrypto should be able to add a cert to the root store (and then remove it)" << std::endl;
	//std::cout << " **SKIPPED**" << std::endl;
	//UNCOMMENT AT YOUR OWN RISK. THIS WILL TRY TO ALTER YOUR WINDOWS ROOT CERTIFICATE STORE :)
	try {
		if (Test8(&warnings)){
			std::cout << " **PASS**" << std::endl;
			passed++;
		}
		else {
			std::cout << " **FAIL**" << std::endl;
		}
	}
	catch (std::exception e) {
		std::cout << "-! exception in Test 8: " << e.what() << std::endl;
	}
	

	//Print Summary:
	std::cout << std::endl << "TEST SUMMARY: " << passed << "/" << total << " tests passed. " << warnings << " warnings." << std::endl << std::endl;
	std::system("PAUSE");
	return 0;
}

//UnbreakableCrypto constructor should not fail
bool TestUnbreakableCrypto::Test1(int * warning_count) {
	UnbreakableCrypto UBC = UnbreakableCrypto();
	UnbreakableCrypto * pUBC = new UnbreakableCrypto();
	delete pUBC;
	return true;
}

//UnbreakableCrypto configure should instantiate necessary members
bool TestUnbreakableCrypto::Test2(int* warning_count) {
	UnbreakableCrypto UBC = UnbreakableCrypto();
	UBC.configure();
	return UBC.isConfigured();
}

//UnbreakableCrypto::evaluate should warn when running without first configuring.
bool TestUnbreakableCrypto::Test3(int* warning_count) {
	bool pass = true;
	UnbreakableCrypto UBC = UnbreakableCrypto();
	RawCertData * valid = BuildValidQuery();
	auto valid_data = valid->getCertInVector();
	RawCertData * invalid = BuildInValidQuery();
	auto invalid_data = invalid->getCertInVector();
	RawCertData * bad_hostname = BuildBadHostnameQuery();
	auto bad_hostnam_data = bad_hostname->getCertInVector();

	if (valid == NULL || invalid == NULL || bad_hostname == NULL) { throw std::exception("Cert Reading Failed!"); }
	if (UBC.evaluateChain(&valid_data, "badssl.com") != UnbreakableCrypto_ERROR) {
		std::cout << "+Unconfigured UnbreakableCrypto failed to return UnbreakableCrypto_ERROR on a valid certificate" << std::endl;
		pass = false;
		(*warning_count)++;
	}
	if (UBC.evaluateChain(&invalid_data, "badssl.com") != UnbreakableCrypto_ERROR) {
		std::cout << "+Unconfigured UnbreakableCrypto failed to return UnbreakableCrypto_ERROR on an invalid certificate" << std::endl;
		pass = false;
		(*warning_count)++;
	}
	if (UBC.evaluateChain(&bad_hostnam_data, "badssl.com") != UnbreakableCrypto_ERROR) {
		std::cout << "+Unconfigured UnbreakableCrypto failed to return UnbreakableCrypto_ERROR on a valid certificate with a bad hostname" << std::endl;
		pass = false;
		(*warning_count)++;
	}

	UBC.configure();
	
	if (!UBC.isConfigured()) {
		std::cout << "+ALERT! UnbreakableCrypto Cannot tell when it is configured!" << std::endl;
		pass = false;
		(*warning_count)++;
	}

	delete valid;
	delete invalid;
	delete bad_hostname;
	return pass;
}

//UnbreakableCrypto should accept a valid certificate.
bool TestUnbreakableCrypto::Test4(int* warning_count) {
	bool pass = true;
	UnbreakableCrypto UBC = UnbreakableCrypto();
	UBC.configure();
	RawCertData * valid = BuildValidQuery();
	std::vector<PCCERT_CONTEXT> valid_vector = valid->getCertInVector();

	if (valid->raw_cert == NULL) { throw std::exception("Cert Reading Failed!"); }
	if (UBC.evaluateChain(&valid_vector, "badssl.com") != UnbreakableCrypto_ACCEPT) {
		std::cout << "+A valid certificate was rejected by UnbreakableCrypto!" << std::endl;
		pass = false;
		(*warning_count)++;
	}
	delete valid;
	return pass;
}

//UnbreakableCrypto should reject a valid certificate with the wrong hostname.
bool TestUnbreakableCrypto::Test5(int* warning_count) {
	bool pass = true;
	UnbreakableCrypto UBC = UnbreakableCrypto();
	UBC.configure();
	RawCertData * bad_hostname = BuildBadHostnameQuery();
	std::vector<PCCERT_CONTEXT> bad_host_data = bad_hostname->getCertInVector();
	RawCertData * null_in_hostname = BuildNullInHostnameQuery();
	std::vector<PCCERT_CONTEXT> null_in_hostname_data = null_in_hostname->getCertInVector();

	if (null_in_hostname == NULL || bad_hostname == NULL) { throw std::exception("Cert Reading Failed!"); }
	if (UBC.evaluateChain(&bad_host_data, "wrong.host.badssl.com") != UnbreakableCrypto_REJECT) {
		std::cout << "+A valid certificate with a bad hostname was not rejected by UnbreakableCrypto!" << std::endl;
		pass = false;
		(*warning_count)++;
	}

	if (UBC.evaluateChain(&null_in_hostname_data, "badssl.com") != UnbreakableCrypto_REJECT) {
		std::cout << "+A valid certificate with a null character in the hostname was not rejected by UnbreakableCrypto!" << std::endl;
		pass = false;
		(*warning_count)++;
	}

	delete bad_hostname;
	delete null_in_hostname;
	return pass;
}

//UnbreakableCrypto should reject an invalid certificate
bool TestUnbreakableCrypto::Test6(int* warning_count) {
	bool pass = true;
	UnbreakableCrypto UBC = UnbreakableCrypto();
	UBC.configure();
	RawCertData * invalid = BuildInValidQuery();
	std::vector<PCCERT_CONTEXT> invalid_data = invalid->getCertInVector();

	if (invalid == NULL) { throw std::exception("Cert Reading Failed!"); }
	if (UBC.evaluateChain(&invalid_data, "badssl.com") != UnbreakableCrypto_REJECT) {
		std::cout << "+An invalid certificate was not rejected by UnbreakableCrypto!" << std::endl;
		pass = false;
		(*warning_count)++;
	}
	delete invalid;
	return pass;
}

//UnbreakableCrypto should reject a mal-formed certificate
bool TestUnbreakableCrypto::Test7(int* warning_count) {
	bool pass = true;
	UnbreakableCrypto UBC = UnbreakableCrypto();
	UBC.configure();
	RawCertData * malformed = BuildMalformedQuery();
	std::vector<PCCERT_CONTEXT> malformed_data = malformed->getCertInVector();

	if (malformed == NULL) { throw std::exception("Cert Reading Failed!"); }
	if (UBC.evaluateChain(&malformed_data, "badssl.com") != UnbreakableCrypto_REJECT) {
		std::cout << "+A malformed certificate was not rejected by UnbreakableCrypto!" << std::endl;
		pass = false;
		(*warning_count)++;
	}
	delete malformed;
	return pass;
}

//UnbreakableCrypto should be able to add a certificate to the root store
//and remove it again.
bool TestUnbreakableCrypto::Test8(int * warning_count)
{
	bool pass = true;
	UnbreakableCrypto UBC = UnbreakableCrypto();
	UBC.configure();

	RawCertData * invalid = BuildSelfSignedQuery();
	std::vector<PCCERT_CONTEXT> invalid_data = invalid->getCertInVector();

	if (invalid == NULL) { throw std::exception("Cert Reading Failed!"); }
	if (UBC.evaluateChain(&invalid_data, "badssl.com") != UnbreakableCrypto_REJECT) {
		std::cout << "+An invalid certificate was not rejected by UnbreakableCrypto!" << std::endl;
		pass = false;
		(*warning_count)++;
	}

	PCCERT_CONTEXT bad_cert = CertCreateCertificateContext(
		(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING),
		invalid->raw_cert,
		invalid->raw_cert_len
	);
	if (bad_cert == NULL) {
		std::cout << "+Certificate Failed to get parsed by WinCrypt. Error code: " << GetLastError() << "." << std::endl;
		return false;
	}

	UBC.insertIntoRootStore(bad_cert);

	UnbreakableCrypto UBC2 = UnbreakableCrypto();
	UBC2.configure();

	if (UBC2.evaluateChain(&invalid_data, "badssl.com") != UnbreakableCrypto_ACCEPT) {
		std::cout << "+An invalid certificate was not accepted by UnbreakableCrypto after being added to the root store." << std::endl;
		pass = false;
		(*warning_count)++;
	}

	UBC2.removeFromRootStore(invalid->raw_cert, invalid->raw_cert_len);

	UnbreakableCrypto UBC3 = UnbreakableCrypto();
	UBC3.configure();

	if (UBC3.evaluateChain(&invalid_data, "badssl.com") != UnbreakableCrypto_REJECT) {
		std::cout << "+An invalid certificate was not rejected by UnbreakableCrypto after being removed from the root store!" << std::endl;
		pass = false;
		(*warning_count)++;
	}

	CertFreeCertificateContext(bad_cert);
	return pass;
}

//reserved
bool TestUnbreakableCrypto::Test9(int * warning_count)
{
	return false;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//Utilities for the tests:

RawCertData * TestUnbreakableCrypto::BuildValidQuery() {
	return QF->fetch(VALID_CERT_FILENAME);
}
RawCertData * TestUnbreakableCrypto::BuildInValidQuery() {
	return QF->fetch(INVALID_CERT_FILENAME);
}
RawCertData * TestUnbreakableCrypto::BuildBadHostnameQuery() {
	return QF->fetch(BAD_HOST_CERT_FILENAME);
}
RawCertData * TestUnbreakableCrypto::BuildNullInHostnameQuery() {
	return QF->fetch(NULL_HOST_CERT_FILENAME);
}
RawCertData * TestUnbreakableCrypto::BuildMalformedQuery() {
	return QF->fetch(MALFORMED_CERT_FILENAME);
}
RawCertData * TestUnbreakableCrypto::BuildSelfSignedQuery() {
	return QF->fetch(SELF_SIGNED_CERT_FILENAME);
}