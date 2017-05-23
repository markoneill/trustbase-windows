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
			std::cout << " PASS" << std::endl;
			passed++;
		}
		else {
			std::cout << " FAIL" << std::endl;
		}
	}
	catch (std::exception e) {
		std::cout << "exception in Test 1: " << e.what();
	}

	std::cout << std::endl << "TEST 2: UnbreakableCrypto configure should instantiate necessary members." << std::endl;
	try {
		if (Test2(&warnings))
		{
			std::cout << " PASS" << std::endl;
			passed++;
		}
		else {
			std::cout << " FAIL" << std::endl;
		}
	}
	catch (std::exception e) {
		std::cout << "exception in Test 2: " << e.what();
	}

	std::cout << std::endl << "TEST 3: UnbreakableCrypto::evaluate should warn when running without first configuring." << std::endl;
	try {
		if (Test3(&warnings))
		{
			std::cout << " PASS" << std::endl;
			passed++;
		}
		else {
			std::cout << " FAIL" << std::endl;
		}
	}
	catch (std::exception e) {
		std::cout << "exception in Test 3: " << e.what();
	}

	std::cout << std::endl << "TEST 4: UnbreakableCrypto should accept a valid certificate." << std::endl;
	try {
		if (Test4(&warnings))
		{
			std::cout << " PASS" << std::endl;
			passed++;
		}
		else {
			std::cout << " FAIL" << std::endl;
		}
	}
	catch (std::exception e) {
		std::cout << "exception in Test 4: " << e.what();
	}

	std::cout << std::endl << "TEST 5: UnbreakableCrypto should reject a valid certificate with the wrong hostname" << std::endl;
	try {
		if (Test5(&warnings))
		{
			std::cout << " PASS" << std::endl;
			passed++;
		}
		else {
			std::cout << " FAIL" << std::endl;
		}
	}
	catch (std::exception e) {
		std::cout << "exception in Test 5: " << e.what();
	}

	std::cout << std::endl << "TEST 6: UnbreakableCrypto should reject an invalid certificate" << std::endl;
	try {
		if (Test6(&warnings))
		{
			std::cout << " PASS" << std::endl;
			passed++;
		}
		else {
			std::cout << " FAIL" << std::endl;
		}
	}
	catch (std::exception e) {
		std::cout << "exception in Test 6: " << e.what();
	}

	std::cout << std::endl << "TEST 7: UnbreakableCrypto should reject a mal-formed certificate" << std::endl;
	try {
		if (Test7(&warnings))
		{
			std::cout << " PASS" << std::endl;
			passed++;
		}
		else {
			std::cout << " FAIL" << std::endl;
		}
	}
	catch (std::exception e) {
		std::cout << "exception in Test 7: " << e.what();
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
	Query * valid = BuildValidQuery();
	Query * invalid = BuildValidQuery();
	Query * bad_hostname = BuildBadHostnameQuery();
	if (UBC.evaluate(valid) != UnbreakableCrypto_ERROR) {
		std::cout << "Unconfigured UnbreakableCrypto failed to return UnbreakableCrypto_ERROR on a valid certificate" << std::endl;
		pass = false;
		(*warning_count)++;
	}
	if (UBC.evaluate(invalid) != UnbreakableCrypto_ERROR) {
		std::cout << "Unconfigured UnbreakableCrypto failed to return UnbreakableCrypto_ERROR on an invalid certificate" << std::endl;
		pass = false;
		(*warning_count)++;
	}
	if (UBC.evaluate(bad_hostname) != UnbreakableCrypto_ERROR) {
		std::cout << "Unconfigured UnbreakableCrypto failed to return UnbreakableCrypto_ERROR on a valid certificate with a bad hostname" << std::endl;
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
	if (UBC.evaluate(valid) != UnbreakableCrypto_ACCEPT) {
		std::cout << "A valid certificate was rejected by UnbreakableCrypto!" << std::endl;
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

	if (UBC.evaluate(bad_hostname) != UnbreakableCrypto_REJECT) {
		std::cout << "A valid certificate with a bad hostname was not rejected by UnbreakableCrypto!" << std::endl;
		pass = false;
		(*warning_count)++;
	}

	if (UBC.evaluate(null_in_hostname) != UnbreakableCrypto_REJECT) {
		std::cout << "A valid certificate with a null character in the hostname was not rejected by UnbreakableCrypto!" << std::endl;
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
	if (UBC.evaluate(invalid) != UnbreakableCrypto_REJECT) {
		std::cout << "An invalid certificate was not rejected by UnbreakableCrypto!" << std::endl;
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
	if (UBC.evaluate(malformed) != UnbreakableCrypto_REJECT) {
		std::cout << "A malformed certificate was not rejected by UnbreakableCrypto!" << std::endl;
		pass = false;
		(*warning_count)++;
	}
	delete malformed;
	return pass;
}




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//Utilities for the tests:

Query * TestUnbreakableCrypto::BuildValidQuery() {
	return QF->fetch("https://badssl.com");
}
Query * TestUnbreakableCrypto::BuildInValidQuery() {
	return NULL;
}
Query * TestUnbreakableCrypto::BuildBadHostnameQuery() {
	return NULL;
}
Query * TestUnbreakableCrypto::BuildNullInHostnameQuery() {
	return NULL;
}
Query * TestUnbreakableCrypto::BuildMalformedQuery() {
	return NULL;
}