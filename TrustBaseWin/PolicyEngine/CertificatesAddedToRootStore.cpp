#include "stdafx.h"
#include "CertificatesAddedToRootStore.h"

/*
*	CertificatesAddedToRootStore is a class to record which certificates have been added to the root store to avoid proxying.
*	This class also is used to remove certificates that have been added to the root store when they are not needed.
*/

/*
* This constructor makes sure that the STORED_CERT_FILE exists, and then loads the data from the file into memory
*/
CertificatesAddedToRootStore::CertificatesAddedToRootStore(){
	std::ifstream infile(STORED_CERT_FILE);
	
	//make sure file exists
	if (!infile){
		std::ofstream file;
		file.open(STORED_CERT_FILE, std::ios::out);
		file.close();
	}

	loadCertificateFile();
}
/*
* Loads the certificates that have been added to the root store from the file into memory
*/
bool CertificatesAddedToRootStore::loadCertificateFile(){
	std::ifstream infile(STORED_CERT_FILE);

	std::string line;
	this->certificates.clear();

	while (std::getline(infile, line)){
		std::istringstream iss(line);
		this->certificates.push_back(line);
	}

	return true;
}
/*
* Saves the certificates that have been added to the root store from the memory onto a file
*/
bool CertificatesAddedToRootStore::saveAllCertificatesToFile(){
	std::ofstream file;
	file.open(STORED_CERT_FILE, std::ofstream::trunc);

	for (int i = 0; i < this->certificates.size(); i++)
		file << this->certificates.at(i) << std::endl;

	file.close();
	return true;
}
/*
*  add a certificates thumbprint to the list in memory and then save the list to a file
*/
bool CertificatesAddedToRootStore::addCertificate(std::string thumbPrint){
	bool success = false;
	certificates.push_back(thumbPrint);
	success = saveAllCertificatesToFile();
	return success;
}
/*
*  remove a certificates thumbprint from the list in memory and then save the list to a file
*/
bool CertificatesAddedToRootStore::removeCertificate(std::string thumbPrint){
	bool success = false;
	if (this->certificates.size() > 0) {
		tblog() << "attempting to remove certificate with thumbprint of " << this->certificates.at(0);
		this->certificates.erase(std::remove(this->certificates.begin(), this->certificates.end(), thumbPrint), certificates.end());
		success = saveAllCertificatesToFile();
		if (success == false){
			tblog(LOG_ERROR) << "Failed to remove certificate";
		}
	}
	else{
		tblog() << "No certs to remove from storedCerts file";
	}
	return success;
}
