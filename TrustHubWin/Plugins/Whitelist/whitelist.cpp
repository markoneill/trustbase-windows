#include <stdio.h>
#include <stdint.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/pem.h>
#include <string.h>
#include <openssl/evp.h>
#include "dirent.h"
//#include <libgen.h>
#include "trusthub_plugin.h"

#include <openssl/pem.h>
extern "C"
{
#include "openssl/applink.c"
}

//substitute for fnmatch
//#include <fnmatch.h>
#include "Shlwapi.h"
#pragma comment(lib,"shlwapi.lib");

#define MAX_LENGTH	1024

int(*plog)(thlog_level_t level, const char* format, ...);
const char * plugin_path;

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

static unsigned int get_cert_fingerprint(X509* cert, EVP_MD* digest, unsigned char* fingerprint, unsigned int* fingerprint_len);
static int compare_fingerprint(unsigned char *fp1, int fp1len, unsigned char *fp2, int fp2len);
static STACK_OF(X509)* get_whitelist();

static int pem_append(char* filename, STACK_OF(X509)* chain);

__declspec(dllexport) int __cdecl initialize(init_data_t* idata) {
	plugin_path = idata->plugin_path;
	plog = idata->log;
	plog(LOG_DEBUG, "Whitelist initilized");
	return 0;
}

__declspec(dllexport) int __cdecl query(query_data_t* data) {
	X509* cert;
	STACK_OF(X509)* whitelist;
	EVP_MD* digest;
	unsigned char fingerprint[EVP_MAX_MD_SIZE];
	unsigned int fingerprint_len;
	int i;
	unsigned char white_fingerprint[EVP_MAX_MD_SIZE];
	unsigned int white_fingerprint_len;
	plog(LOG_DEBUG, "Whitelist: query function ran");
	plog(LOG_DEBUG, "Whitelist querying");
	/* Only check the leaf certificate */
	cert = sk_X509_value(data->chain, 0);
	//print_certificate(cert);

	/* Get the fingerprint for the leaf cert */

	//OpenSSL_add_all_algorithms(); // Need this?

	digest = (EVP_MD*)EVP_sha1();
	fingerprint_len = sizeof(fingerprint);
	if (!get_cert_fingerprint(cert, digest, fingerprint, &fingerprint_len)) {
		return PLUGIN_RESPONSE_ERROR;
	}

	plog(LOG_DEBUG, "Got fingerprint");

	/* Compare fingerprint to the whitelist */
	whitelist = get_whitelist();
	if (!whitelist) {
		return PLUGIN_RESPONSE_ERROR;
	}

	// Right now this is going over every whitelisted cert, and taking a hash of them, then comparing
	// TODO: For quicker results, switch to storing only hashes of the whitelisted certificates
	plog(LOG_DEBUG, "running through whitelist");
	for (i = 0; i < sk_X509_num(whitelist); i++) {
		cert = sk_X509_value(whitelist, i);
		white_fingerprint_len = sizeof(white_fingerprint);
		if (!get_cert_fingerprint(cert, digest, white_fingerprint, &white_fingerprint_len)) {
			// Couldn't get a fingerprint
			sk_X509_pop_free(whitelist, X509_free);
			return PLUGIN_RESPONSE_ERROR;
		}
		if (compare_fingerprint(fingerprint, fingerprint_len, white_fingerprint, white_fingerprint_len)) {
			// We found a good certificate
			sk_X509_pop_free(whitelist, X509_free);
			return PLUGIN_RESPONSE_VALID;
		}
	}

	sk_X509_pop_free(whitelist, X509_free);
	return PLUGIN_RESPONSE_INVALID;
}

// Plugins can also have an optional exported "finalize" function that takes no arg
__declspec(dllexport) int __cdecl finalize() {
	return PLUGIN_FINALIZE_OK;
}
static unsigned int get_cert_fingerprint(X509* cert, EVP_MD* digest, unsigned char* fingerprint, unsigned int* fingerprint_len) {
	if (*fingerprint_len < EVP_MD_size(digest)) {
		return 0;
	}
	if (!X509_digest(cert, digest, fingerprint, fingerprint_len)) {
		return 0;
	}
	return *fingerprint_len;
}

static int compare_fingerprint(unsigned char *fp1, int fp1len, unsigned char *fp2, int fp2len) {
	return ((fp1len == fp2len) && !memcmp(fp1, fp2, fp1len));
}

 static STACK_OF(X509)* get_whitelist() {
	STACK_OF(X509)* whitelist;
	DIR *dir;
	struct dirent *ent;
	char* whitelist_dir;
	char* temp_path;
	char* filename;

	whitelist_dir = (char*)malloc(strlen(plugin_path) + 11);
	if (whitelist_dir == NULL) {
		return NULL;
	}

	//make copy of plugin_path because dirname changes whats passed in
	//temp_path = (char*)malloc(strlen(plugin_path) + 1);
	//if (temp_path == NULL) {
	//	return NULL;
	//}

	strncpy(whitelist_dir, plugin_path, strlen(plugin_path) + 1);
	
	//sub for dirname
	char *p = strrchr(whitelist_dir, '/');
	if (p) { p[0] = 0; }
	//whitelist_dir = dirname(temp_path);
	//end sub

	strcat(whitelist_dir, "/whitelist");
	plog(LOG_DEBUG, "Getting whitelist at %s", whitelist_dir);
	whitelist = sk_X509_new_null();

	if ((dir = opendir(whitelist_dir)) != NULL) {
		while ((ent = readdir(dir)) != NULL) {
			//sub for fnmatch
			if (PathMatchSpecA(ent->d_name, "*.pem")) {
			//if (!fnmatch("*.pem", ent->d_name, 0)) {
			//end sub for fnmatch
				filename = (char*)malloc(sizeof(whitelist_dir) + sizeof(ent->d_name));
				sprintf(filename, "%s\\%s", whitelist_dir, ent->d_name);
				if (!pem_append(filename, whitelist)) {
					// Could not add the file
				}
				free(filename);
			}
		}
		closedir(dir);
	}
	free(whitelist_dir);
	return whitelist;
}

static int pem_append(char* filename, STACK_OF(X509)* chain) {
	FILE *fp = NULL;
	X509* cert = NULL;

	fp = fopen(filename, "rb");
	if (fp == NULL) {
		return 0;
	}

	while (1 == 1) {
		cert = PEM_read_X509(fp, NULL, NULL, NULL);
		if (cert == NULL) {
			break;
		}
		sk_X509_push(chain, cert);
	}
	fclose(fp);

	return 1;
}
