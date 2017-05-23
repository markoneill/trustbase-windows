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
	int Test();

private:
	QueryFetcher * QF = NULL;
	bool Test1(int* warning_count);
	bool Test2(int* warning_count);
	bool Test3(int* warning_count);
	bool Test4(int* warning_count);
	bool Test5(int* warning_count);
	bool Test6(int* warning_count);
	bool Test7(int* warning_count);
	Query * BuildValidQuery();
	Query * BuildInValidQuery();
	Query * BuildBadHostnameQuery();
	Query * BuildNullInHostnameQuery();
	Query * BuildMalformedQuery();
};