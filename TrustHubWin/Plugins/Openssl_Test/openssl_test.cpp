#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "trusthub_plugin.h"
#include <openssl/x509.h>

#define MAX_LENGTH	1024

#ifdef __cplusplus
extern "C" {  // only need to export C interface if  
			  // used by C++ source code  
#endif  
	__declspec(dllexport) int __cdecl query(query_data_t*);
	__declspec(dllexport) int __cdecl initialize(init_data_t*);
	__declspec(dllexport) int __cdecl finalize();

#ifdef __cplusplus
}
#endif  
void print_certificate(X509* cert);

void print_certificate(X509* cert) {
	char subj[MAX_LENGTH + 1];
	char issuer[MAX_LENGTH + 1];
	X509_NAME_oneline(X509_get_subject_name(cert), subj, MAX_LENGTH);
	X509_NAME_oneline(X509_get_issuer_name(cert), issuer, MAX_LENGTH);
	printf("subject: %s\n", subj);
	printf("issuer: %s\n", issuer);
}
// Plugins must have an exported "query" function that takes a query_data_t* argument
// Plugins must include the "trusthub_plugin.h" header
// In visual studio, add the path to the Policy engine code under:
//   Configuration Properties->C/C++->General->Additional Include Directories
__declspec(dllexport) int __cdecl query(query_data_t* data) {
	int i;
	X509* cert;
	printf("OpenSSL Test Plugin checking cert for host: %s\n", data->hostname);
	printf("Certificate Data:\n");
	for (i = 0; i < sk_X509_num(data->chain); i++) {
	cert = sk_X509_value(data->chain, i);
	print_certificate(cert);
	}
	return PLUGIN_RESPONSE_VALID;
}

// Plugins can also have an optional exported "initialize" function that takes an init_data_t* arg
__declspec(dllexport) int __cdecl initialize(init_data_t*) {
	return PLUGIN_INITIALIZE_OK;
}

// Plugins can also have an optional exported "finalize" function that takes no arg
__declspec(dllexport) int __cdecl finalize() {
	return PLUGIN_FINALIZE_OK;
}