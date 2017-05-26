#pragma once
#include <vector>
#include <Windows.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <string>
#include "THLogger.h"
#define STORED_CERT_FILE "storedCerts.bin"

class CertificatesAddedToRootStore {
public:
	CertificatesAddedToRootStore();
	bool addCertificate(std::string thumbPrint);
	bool removeCertificate(std::string thumbPrint);
	std::vector<std::string> certificates;
private:
	bool saveAllCertificatesToFile();
	bool loadCertificateFile();
};