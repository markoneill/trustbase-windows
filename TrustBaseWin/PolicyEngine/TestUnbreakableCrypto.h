 #pragma once
#include <string>
#include <stdio.h>
#include <exception>
#include "Query.h"
#include "QueryFetcher.h"
#include "UnbreakableCrypto.h"
class TestUnbreakableCrypto {
public:
	TestUnbreakableCrypto();
	~TestUnbreakableCrypto();
	/*
	run test cases
	*/
	int Test();
private:
	QueryFetcher * QF = NULL;

	//UnbreakableCrypto constructor should not fail
	bool Test1(int* warning_count);
	
	//UnbreakableCrypto configure should instantiate necessary members
	bool Test2(int* warning_count);
	
	//UnbreakableCrypto::evaluate should warn when running without first configuring.
	bool Test3(int* warning_count);
	
	//UnbreakableCrypto should accept a valid certificate.
	bool Test4(int* warning_count);

	//UnbreakableCrypto should reject a valid certificate with the wrong hostname.
	bool Test5(int* warning_count);

	//UnbreakableCrypto should reject an invalid certificate
	bool Test6(int* warning_count);
	
	//UnbreakableCrypto should reject a mal-formed certificate
	bool Test7(int* warning_count);
	
	//UnbreakableCrypto should be able to add a certificate to the root store
	//and remove it again.
	bool Test8(int* warning_count);

	//reserved
	bool Test9(int* warning_count);

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//Utilities for the tests:
	RawCertData * BuildValidQuery();
	RawCertData * BuildInValidQuery();
	RawCertData * BuildBadHostnameQuery();
	RawCertData * BuildNullInHostnameQuery();
	RawCertData * BuildMalformedQuery();
	RawCertData * BuildSelfSignedQuery();
};