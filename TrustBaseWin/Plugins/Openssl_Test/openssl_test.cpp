#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "trustbase_plugin.h"
#include <openssl/x509.h>

#define MAX_LENGTH	1024

#ifdef __cplusplus
extern "C" {  // only need to export C interface if  
			  // used by C++ source code  
#endif  
	__declspec(dllexport) int __stdcall query(query_data_t*);
	__declspec(dllexport) int __stdcall initialize(init_data_t*);
	__declspec(dllexport) int __stdcall finalize();

#ifdef __cplusplus
}
#endif  
int(*plog)(tblog_level_t level, const char* format, ...);

void print_certificate(X509* cert);

void print_certificate(X509* cert) {
	char subj[MAX_LENGTH + 1];
	char issuer[MAX_LENGTH + 1];
	X509_NAME_oneline(X509_get_subject_name(cert), subj, MAX_LENGTH);
	X509_NAME_oneline(X509_get_issuer_name(cert), issuer, MAX_LENGTH);
	plog(LOG_DEBUG, "subject: %s\n", subj);
	plog(LOG_DEBUG, "issuer: %s\n", issuer);
}
// Plugins must have an exported "query" function that takes a query_data_t* argument
// Plugins must include the "trustbase_plugin.h" header
// In visual studio, add the path to the Policy engine code under:
//   Configuration Properties->C/C++->General->Additional Include Directories
__declspec(dllexport) int __stdcall query(query_data_t* data) {
	int i;
	X509* cert;
	EVP_PKEY* pub_key;
	unsigned char* pkey_buf;
	plog(LOG_DEBUG, "OpenSSL Test Plugin checking cert for host: %s\n", data->hostname);
	plog(LOG_DEBUG, "Certificate Data:\n");

	int count = sk_X509_num(data->chain);
	for (i = 0; i < count; i++) {
	cert = sk_X509_value(data->chain, i);
	print_certificate(cert);
	}
	return PLUGIN_RESPONSE_VALID;
}

// Plugins can also have an optional exported "initialize" function that takes an init_data_t* arg
__declspec(dllexport) int __stdcall initialize(init_data_t* idata) {
	plog = idata->log;

	return PLUGIN_INITIALIZE_OK;
}

// Plugins can also have an optional exported "finalize" function that takes no arg
__declspec(dllexport) int __stdcall finalize() {
	return PLUGIN_FINALIZE_OK;
}