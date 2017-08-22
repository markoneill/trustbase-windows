#pragma once
#include <vector>
#include <Windows.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <string>
#include "TBLogger.h"
#define STORED_CERT_FILE "storedCerts.bin"
/*
*	CertificatesAddedToRootStore is a class to record which certificates have been added to the root store to avoid proxying.
*	This class also is used to remove certificates that have been added to the root store when they are not needed.
*/
class CertificatesAddedToRootStore {
public:
	/*
	* This constructor makes sure that the STORED_CERT_FILE exists, and then loads the data from the file into memory
	*/
	CertificatesAddedToRootStore();
	/*
	*  add a certificates thumbprint to the list in memory and then save the list to a file
	*/
	bool addCertificate(std::string thumbPrint);
	/*
	*  remove a certificates thumbprint from the list in memory and then save the list to a file
	*/
	bool removeCertificate(std::string thumbPrint);
	std::vector<std::string> certificates;
private:
	/*
	* Loads the certificates that have been added to the root store from the file into memory
	*/
	bool loadCertificateFile();
	/*
	* Saves the certificates that have been added to the root store from the memory onto a file
	*/
	bool saveAllCertificatesToFile();
};