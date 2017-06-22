#include <stdio.h>
#include <stdlib.h>
//#include <libgen.h>
#include <sqlite3.h>
#include <stdint.h>
#include <time.h>
#include <string.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/sha.h>
#include <openssl/asn1.h>
#include "trustbase_plugin.h"
#define PINNING_DATABASE "pinned_certs.db"

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

char* database_path;

int(*plog)(tblog_level_t level, const char* format, ...);

static time_t ASN1_GetTimeT(ASN1_TIME* time);

__declspec(dllexport) int __stdcall initialize(init_data_t* idata) {
	const char* plugin_path;
	int i;

	plog = idata->log;

	// make the path for our plugin
	plugin_path = idata->plugin_path;
	database_path = (char*)malloc(strlen(plugin_path) + strlen(PINNING_DATABASE) + 1);
	strcpy(database_path, plugin_path);
	database_path[strlen(plugin_path)] = '\0';

	// put a \0 in the thing
	for (i = strlen(database_path) - 1; i>0; i--) {
		if (database_path[i] == '/' || database_path[i] == '\\') {
			database_path[i] = '\0';
			break;
		}
	}

	strcat(database_path, "/");
	strcat(database_path, PINNING_DATABASE);

	plog(LOG_DEBUG, "CERT PINNING: Trying to use database at %s", database_path);
	return 0;
}

__declspec(dllexport) int __stdcall query(query_data_t* data) {
	int rval;
	unsigned char* hash;
	unsigned char* stored_hash;
	X509* cert;
	EVP_PKEY* pub_key;
	unsigned char* pkey_buf;
	sqlite3* database;
	sqlite3_stmt* statement;
	time_t ptime;
	time_t exptime;

	rval = PLUGIN_RESPONSE_VALID;

	// Get Certificate Public Key

	plog(LOG_DEBUG, "cert_pinning: query function ran");

	cert = sk_X509_value(data->chain, 0);
	pub_key = X509_get_pubkey(cert);
	pkey_buf = NULL;
	i2d_PUBKEY(pub_key, &pkey_buf);

	// Hash it
	hash = (unsigned char*)malloc(SHA256_DIGEST_LENGTH + 1);
	hash[SHA256_DIGEST_LENGTH] = '\0';
	SHA256(pkey_buf, strlen((char*)pkey_buf), hash);
	OPENSSL_free(pkey_buf);

	// Check the Database
	database = NULL;
	statement = NULL;

	if (database_path == NULL) {
		return PLUGIN_RESPONSE_ERROR;
	}

	if (sqlite3_open_v2(database_path, &database, SQLITE_OPEN_READWRITE, NULL) != SQLITE_OK) {
		return PLUGIN_RESPONSE_ERROR;
	}

	// Build the table if it is not there
	if (sqlite3_prepare_v2(database, "CREATE TABLE IF NOT EXISTS pinned (hostname TEXT PRIMARY KEY, hash TEXT, exptime INTEGER)", -1, &statement, NULL) != SQLITE_OK) {
		plog(LOG_ERROR, "CERT PINNING: Could not create certificate table");
	}
	sqlite3_step(statement);
	sqlite3_finalize(statement);

	// Get the current time
	time(&ptime);
	// See if it is expired
	if (X509_cmp_time(X509_get_notAfter(cert), &ptime) < 0) {
		// This cert is expired, so just say no
		return PLUGIN_RESPONSE_INVALID;
	}
	// Get cert expire time as a time_t
	exptime = ASN1_GetTimeT(X509_get_notAfter(cert));

	/* There should be a table named 'pinned'
	* CREATE TABLE pinned (hostname TEXT PRIMARY KEY, hash TEXT, exptime INTEGER);
	*/


	if (sqlite3_prepare_v2(database, "SELECT hash FROM pinned WHERE hostname=?1 AND exptime > ?2;", -1, &statement, NULL) != SQLITE_OK) {
		rval = PLUGIN_RESPONSE_ERROR;
	}
	else if (sqlite3_bind_text(statement, 1, (char*)data->hostname, -1, SQLITE_STATIC) != SQLITE_OK) {
		rval = PLUGIN_RESPONSE_ERROR;
	}

	//Luke edit:
	//uh, I dont think this previous code made any sense
	//sqlite3_bind_int64(statement, 2, (sqlite_uint64)exptime) != SQLITE_OK) {
	//this would be asking if the cert we are presenting expires at a later time than the current cert pinned.
	//I changed it to the following:
	//(sqlite3_bind_int64(statement, 2, (sqlite_uint64)ptime) != SQLITE_OK)
	//this says: I want the cert that is pinned, if it has not yet expired. Otherwise we ignore it
	else if (sqlite3_bind_int64(statement, 2, (sqlite_uint64)ptime) != SQLITE_OK) {
		rval = PLUGIN_RESPONSE_ERROR;
	}
	else if (sqlite3_step(statement) == SQLITE_ROW) {
		// There was a result, compare the stored hash with the new one
		stored_hash = (unsigned char*)sqlite3_column_blob(statement, 0);
		if (strcmp((char*)hash, (char*)stored_hash) != 0) {
			rval = PLUGIN_RESPONSE_INVALID;
		}
	}
	else {
		// There were no results, do an insert.
		sqlite3_finalize(statement);
		if (sqlite3_prepare_v2(database, "INSERT OR REPLACE INTO pinned VALUES(?1,?2,?3);", -1, &statement, NULL) != SQLITE_OK) {
			rval = PLUGIN_RESPONSE_ERROR;
		}
		else if (sqlite3_bind_text(statement, 1, (char*)data->hostname, -1, SQLITE_STATIC) != SQLITE_OK) {
			rval = PLUGIN_RESPONSE_ERROR;
		}
		else if (sqlite3_bind_text(statement, 2, (char*)hash, -1, SQLITE_STATIC) != SQLITE_OK) {
			rval = PLUGIN_RESPONSE_ERROR;
		}
		else if (sqlite3_bind_int64(statement, 3, (sqlite_uint64)exptime) != SQLITE_OK) {
			rval = PLUGIN_RESPONSE_ERROR;
		}
		else if (sqlite3_step(statement) != SQLITE_DONE) {
			rval = PLUGIN_RESPONSE_ERROR;
		}
	}

	sqlite3_finalize(statement);
	sqlite3_close(database);
	free(hash);
	return rval;
}

__declspec(dllexport) int __stdcall finalize() {
	if (database_path != NULL) {
		free(database_path);
	}
	return 0;
}

static time_t ASN1_GetTimeT(ASN1_TIME* time) {
	struct tm t;
	const char* str = (const char*)time->data;
	size_t i = 0;

	memset(&t, 0, sizeof(t));

	if (time->type == V_ASN1_UTCTIME) {/* two digit year */
		t.tm_year = (str[i++] - '0') * 10;
		t.tm_year += (str[i++] - '0');
		if (t.tm_year < 70) {
			t.tm_year += 100;
		}
	}
	else if (time->type == V_ASN1_GENERALIZEDTIME) {/* four digit year */
		t.tm_year = (str[i++] - '0') * 1000;
		t.tm_year += (str[i++] - '0') * 100;
		t.tm_year += (str[i++] - '0') * 10;
		t.tm_year += (str[i++] - '0');
		t.tm_year -= 1900;
	}
	t.tm_mon = (str[i++] - '0') * 10;
	t.tm_mon += (str[i++] - '0') - 1; // -1 since January is 0 not 1.
	t.tm_mday = (str[i++] - '0') * 10;
	t.tm_mday += (str[i++] - '0');
	t.tm_hour = (str[i++] - '0') * 10;
	t.tm_hour += (str[i++] - '0');
	t.tm_min = (str[i++] - '0') * 10;
	t.tm_min += (str[i++] - '0');
	t.tm_sec = (str[i++] - '0') * 10;
	t.tm_sec += (str[i++] - '0');

	/* Note: we did not adjust the time based on time zone information */
	return mktime(&t);
}
