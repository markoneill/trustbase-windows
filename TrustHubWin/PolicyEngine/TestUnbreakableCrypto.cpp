#include "stdafx.h"
#include "TestUnbreakableCrypto.h"



TestUnbreakableCrypto::TestUnbreakableCrypto() {
	QF = new QueryFetcher();
}

TestUnbreakableCrypto::~TestUnbreakableCrypto() {
	delete QF;
}

int TestUnbreakableCrypto::Test() {
	int passed = 0;
	int warnings = 0;
	const int total = 8;

	std::cout << std::endl << "TEST 1: UnbreakableCrypto constructor should not fail." << std::endl;
	try {
		if (Test1(&warnings)) 
		{
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
		if (Test2(&warnings))
		{
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
		if (Test3(&warnings))
		{
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
		if (Test4(&warnings))
		{
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
		if (Test5(&warnings))
		{
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
		if (Test6(&warnings))
		{
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
		if (Test7(&warnings))
		{
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


	std::cout << std::endl << "TEST 8: UnbreakableCrypto should be able to add a cert to the root store" << std::endl;
	std::cout << " **SKIPPED**" << std::endl;
	/*UNCOMMENT AT YOUR OWN RISK. THIS WILL TRY TO ALTER YOUR WINDOWS ROOT CERTIFICATE STORE :)
	try {
		if (Test8(&warnings))
		{
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
	*/

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
	Query * valid = BuildValidQuery();
	Query * invalid = BuildValidQuery();
	Query * bad_hostname = BuildBadHostnameQuery();
	if (valid == NULL || invalid == NULL || bad_hostname == NULL) { throw std::exception("Cert Reading Failed!"); }
	if (UBC.evaluate(valid->data.raw_chain, valid->data.raw_chain_len, "badssl.com") != UnbreakableCrypto_ERROR) {
		std::cout << "+Unconfigured UnbreakableCrypto failed to return UnbreakableCrypto_ERROR on a valid certificate" << std::endl;
		pass = false;
		(*warning_count)++;
	}
	if (UBC.evaluate(invalid->data.raw_chain, invalid->data.raw_chain_len, "badssl.com") != UnbreakableCrypto_ERROR) {
		std::cout << "+Unconfigured UnbreakableCrypto failed to return UnbreakableCrypto_ERROR on an invalid certificate" << std::endl;
		pass = false;
		(*warning_count)++;
	}
	if (UBC.evaluate(bad_hostname->data.raw_chain, bad_hostname->data.raw_chain_len, "badssl.com") != UnbreakableCrypto_ERROR) {
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
	Query * valid = BuildValidQuery();

	if (valid == NULL) { throw std::exception("Cert Reading Failed!"); }
	if (UBC.evaluate(valid->data.raw_chain, valid->data.raw_chain_len, "badssl.com") != UnbreakableCrypto_ACCEPT) {
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
	Query * bad_hostname = BuildBadHostnameQuery();
	Query * null_in_hostname = BuildNullInHostnameQuery();

	if (null_in_hostname == NULL || bad_hostname == NULL) { throw std::exception("Cert Reading Failed!"); }
	if (UBC.evaluate(bad_hostname->data.raw_chain, bad_hostname->data.raw_chain_len, "badssl.com") != UnbreakableCrypto_REJECT) {
		std::cout << "+A valid certificate with a bad hostname was not rejected by UnbreakableCrypto!" << std::endl;
		pass = false;
		(*warning_count)++;
	}

	if (UBC.evaluate(null_in_hostname->data.raw_chain, null_in_hostname->data.raw_chain_len, "badssl.com") != UnbreakableCrypto_REJECT) {
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
	Query * invalid = BuildInValidQuery();
	if (invalid == NULL) { throw std::exception("Cert Reading Failed!"); }
	if (UBC.evaluate(invalid->data.raw_chain, invalid->data.raw_chain_len, "badssl.com") != UnbreakableCrypto_REJECT) {
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
	Query * malformed = BuildMalformedQuery();
	if (malformed == NULL) { throw std::exception("Cert Reading Failed!"); }
	if (UBC.evaluate(malformed->data.raw_chain, malformed->data.raw_chain_len, "badssl.com") != UnbreakableCrypto_REJECT) {
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

	Query * invalid = BuildInValidQuery();

	if (invalid == NULL) { throw std::exception("Cert Reading Failed!"); }
	if (UBC.evaluate(invalid->data.raw_chain, invalid->data.raw_chain_len, "badssl.com") != UnbreakableCrypto_REJECT) {
		std::cout << "+An invalid certificate was not rejected by UnbreakableCrypto!" << std::endl;
		pass = false;
		(*warning_count)++;
	}

	PCCERT_CONTEXT bad_cert = CertCreateCertificateContext(
		(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING),
		invalid->data.raw_chain, 
		invalid->data.raw_chain_len
	);
	if (bad_cert == NULL) {
		std::cout << "+Certificate Failed to get parsed by WinCrypt. Error code: " << GetLastError() << "." << std::endl;
		return false;
	}

	UBC.insertIntoRootStore(bad_cert);

	if (UBC.evaluate(invalid->data.raw_chain, invalid->data.raw_chain_len, "badssl.com") != UnbreakableCrypto_ACCEPT) {
		std::cout << "+An invalid certificate was not accepted by UnbreakableCrypto after being added to the root store." << std::endl;
		pass = false;
		(*warning_count)++;
	}

	UBC.removeFromRootStore(invalid);

	if (UBC.evaluate(invalid->data.raw_chain, invalid->data.raw_chain_len, "badssl.com") != UnbreakableCrypto_REJECT) {
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

Query * TestUnbreakableCrypto::BuildValidQuery() {
	return QF->fetch(VALID_CERT_FILENAME);
}
Query * TestUnbreakableCrypto::BuildInValidQuery() {
	return QF->fetch(INVALID_CERT_FILENAME);
}
Query * TestUnbreakableCrypto::BuildBadHostnameQuery() {
	return QF->fetch(BAD_HOST_CERT_FILENAME);
}
Query * TestUnbreakableCrypto::BuildNullInHostnameQuery() {
	return QF->fetch(NULL_HOST_CERT_FILENAME);
}
Query * TestUnbreakableCrypto::BuildMalformedQuery() {
	return QF->fetch(MALFORMED_CERT_FILENAME);
}