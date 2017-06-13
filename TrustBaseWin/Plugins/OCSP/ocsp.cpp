#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "trustbase_plugin.h"
#include "TBLogger_Level.h"

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
int(*tb_callback)(int plugin_id, int query_id, int plugin_response);
int plugin_id;

HCERTSTORE openRootStore();
// Plugins must have an exported "query" function that takes a query_data_t* argument
// Plugins must include the "trustbase_plugin.h" header
// In visual studio, add the path to the Policy engine code under:
//   Configuration Properties->C/C++->General->Additional Include Directories
__declspec(dllexport) int __stdcall query(query_data_t* data) {
	plog(LOG_DEBUG, "OCSP: query function ran");

	int status = PLUGIN_RESPONSE_VALID;

	HCERTSTORE rootStore = openRootStore();
	CERT_REVOCATION_STATUS revocation_status = CERT_REVOCATION_STATUS();
	revocation_status.cbSize = sizeof(CERT_REVOCATION_STATUS);
	for (int i = 0; i < data->cert_context_chain->size() - 1; i++) {

		CERT_REVOCATION_PARA revocation_para;
		revocation_para.cbSize = sizeof(CERT_REVOCATION_PARA);
		revocation_para.cCertStore = 0;
		revocation_para.hCrlStore = rootStore;
		revocation_para.pftTimeToUse = NULL;
		revocation_para.pIssuerCert = data->cert_context_chain->at(i + 1);
		revocation_para.rgCertStore = NULL;

		if (!CertVerifyRevocation(
			X509_ASN_ENCODING,
			CERT_CONTEXT_REVOCATION_TYPE,
			1,
			(PVOID*) &(data->cert_context_chain->at(i)),
			CERT_VERIFY_REV_SERVER_OCSP_FLAG,
			&revocation_para,
			&revocation_status
		))
		{
			plog(LOG_DEBUG, "GetLastError: %d", GetLastError());
			const char* reason_text;

			switch (revocation_status.dwError) {
			case CRYPT_E_NO_REVOCATION_CHECK:
				reason_text = "An installed or registered revocation function was not able to do a revocation check on the context.";
				break;
			case CRYPT_E_NO_REVOCATION_DLL:
				reason_text = "No installed or registered DLL was found that was able to verify revocation.";
				status = PLUGIN_RESPONSE_ABSTAIN;
				break;
			case CRYPT_E_NOT_IN_REVOCATION_DATABASE:
				//no ocsp
				reason_text = "The context to be checked was not found in the revocation server's database.";
				break;
			case CRYPT_E_REVOCATION_OFFLINE:
				reason_text = "It was not possible to connect to the revocation server.";
				status = PLUGIN_RESPONSE_ABSTAIN;
				break;
			case CRYPT_E_REVOKED:
				switch (revocation_status.dwReason) {
				case CRL_REASON_UNSPECIFIED:
					reason_text = "The revoking CA gave no explanation. This is discouraged by RFC 2459";
					break;
				case CRL_REASON_KEY_COMPROMISE:
					reason_text = "The CA says their private key was compromised";
					break;
				case CRL_REASON_CA_COMPROMISE:
					reason_text = "The CA says thier own private key was compromised";
					break;
				case CRL_REASON_AFFILIATION_CHANGED:
					reason_text = "The CA says affiliations changed";
					break;
				case CRL_REASON_SUPERSEDED:
					reason_text = "The CA says a new cert supersedes this one";
					break;
				case CRL_REASON_CESSATION_OF_OPERATION:
					reason_text = "The CA says they have shut down operations and can no longer be relied upon";
					break;
				case CRL_REASON_CERTIFICATE_HOLD:
					reason_text = "The CA says this certificate is on hold";
					break;
				} 
				plog(LOG_DEBUG, "Revoked certificate was encountered. reason: %s", reason_text);
				
				status = PLUGIN_RESPONSE_INVALID;
				break;
			case ERROR_SUCCESS:
				reason_text = "The context was good.";
				break;
			case E_INVALIDARG:
				reason_text = "cbSize in pRevStatus is less than sizeof(CERT_REVOCATION_STATUS).Note that dwError in pRevStatus is not updated for this error.";
				status = PLUGIN_RESPONSE_ABSTAIN;
				break;
			}
		}
	}

	CertCloseStore(rootStore, CERT_CLOSE_STORE_FORCE_FLAG);
	tb_callback(plugin_id, data->id, status);
	return 0; // Doesn't matter, we are going to respond via async stuff
}

/*
Returns a handle to the root store
*/
HCERTSTORE openRootStore()
{
	auto target_store_name = L"Root";
	return CertOpenStore(
		CERT_STORE_PROV_SYSTEM,
		0,
		NULL,
		CERT_SYSTEM_STORE_CURRENT_USER,
		target_store_name
	);
}

// Plugins can also have an optional exported "initialize" function that takes an init_data_t* arg
__declspec(dllexport) int __stdcall initialize(init_data_t* idata) {
	plog = idata->log;
	plugin_id = idata->plugin_id;

	tb_callback = idata->callback;
	return PLUGIN_INITIALIZE_OK;
}

// Plugins can also have an optional exported "finalize" function that takes no arg
__declspec(dllexport) int __stdcall finalize() {
	return PLUGIN_FINALIZE_OK;
}

// An optional DllMain can be added for a more precise initialization and finalization