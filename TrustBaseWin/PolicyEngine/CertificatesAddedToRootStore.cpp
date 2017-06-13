#include "stdafx.h"
#include "CertificatesAddedToRootStore.h"


CertificatesAddedToRootStore::CertificatesAddedToRootStore()
{
	//make sure file exists
	std::ifstream infile(STORED_CERT_FILE);

	if (!infile)
	{
		std::ofstream file;
		file.open(STORED_CERT_FILE, std::ios::out);
		file.close();
	}

	loadCertificateFile();
}
bool CertificatesAddedToRootStore::loadCertificateFile()
{
	std::ifstream infile(STORED_CERT_FILE);

	std::string line;
	this->certificates.clear();

	while (std::getline(infile, line))
	{
		std::istringstream iss(line);
		this->certificates.push_back(line);
	}

	return true;
}
bool CertificatesAddedToRootStore::saveAllCertificatesToFile()
{
	std::ofstream file;
	file.open(STORED_CERT_FILE, std::ofstream::trunc);

	for (int i = 0; i < this->certificates.size(); i++)
		file << this->certificates.at(i) << std::endl;

	file.close();
	return true;
}

bool CertificatesAddedToRootStore::addCertificate(std::string thumbPrint)
{
	bool success = false;
	certificates.push_back(thumbPrint);
	success = saveAllCertificatesToFile();
	return success;
}

bool CertificatesAddedToRootStore::removeCertificate(std::string thumbPrint)
{
	bool success = false;
	if (this->certificates.size() > 0) {
		tblog() << "attempting to remove certificate with thumbprint of " << this->certificates.at(0);
		this->certificates.erase(std::remove(this->certificates.begin(), this->certificates.end(), thumbPrint), certificates.end());
		success = saveAllCertificatesToFile();
		if (success == false)
		{
			tblog(LOG_ERROR) << "Failed to remove certificate";
		}
	}
	else
	{
		tblog() << "No certs to remove from storedCerts file";
	}
	return success;
}
