#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libconfig.h++>
//#include <libgen.h>
#include "trustbase_plugin.h"

#define CONFIG_FILE "/plugin-config/cipher_suite.cfg"
#define PLUGIN_INIT_ERROR -1

#define CLIENT_HELLO_SIZE 3
#define CLIENT_HELLO_TLS_VERISON 2
#define CLIENT_HELLO_RANDOM_BYTES 32
#define CLIENT_HELLO_SESSION_LENGTH 1

#define CLIENT_HELLO_SESSION_LEN_OFF	CLIENT_HELLO_SIZE+CLIENT_HELLO_TLS_VERISON+CLIENT_HELLO_RANDOM_BYTES

//Luke Edit: Assuming 0 length session: This value was set to 0x27 (39). For windows, at least, it should be 0x26 (38)
//dont use CLIENT_HELLO_CIPHER_LEN_OFF as it assumes 0 length session
//#define CLIENT_HELLO_CIPHER_LEN_OFF	CLIENT_HELLO_SIZE+CLIENT_HELLO_TLS_VERISON+CLIENT_HELLO_RANDOM_BYTES+CLIENT_HELLO_SESSION_LENGTH
#define CLIENT_HELLO_CIPHER_LEN_SIZE	2
#define CLIENT_HELLO_COMPRESS_LEN_SIZE	1
#define CLIENT_HELLO_EXT_LEN_SIZE	2

#define SERVER_HELLO_SIZE			3
#define SERVER_HELLO_TLS_VERISON	2
#define SERVER_HELLO_RANDOM_BYTES 32
#define SERVER_HELLO_SESSION_LENGTH 1

#define SERVER_HELLO_SESSION_LEN_OFF	SERVER_HELLO_SIZE + SERVER_HELLO_TLS_VERISON +SERVER_HELLO_RANDOM_BYTES

//Luke Edit: Assuming 0 length session: This value was set to 0x47. For windows, at least, it should be 0x27 (39)
//dont use SERVER_HELLO_CIPHER_OFF as it assumes 0 length session

//#define SERVER_HELLO_CIPHER_OFF		SERVER_HELLO_SIZE + SERVER_HELLO_TLS_VERISON +SERVER_HELLO_RANDOM_BYTES+ SERVER_HELLO_SESSION_LENGTH
#define SERVER_HELLO_CIPHER_SIZE	2
#define SERVER_HELLO_COMPRESSION_SIZE	1

//Luke Edit: This value is not always true
//#define EXT_OFF				0x4c

#define All_EXT_SIZE			2

#define EXT_SIZE				2
#define EXT_LEN_SIZE			2

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

static void hexdump(char* data, size_t len);
static bool loadconfig(const char* config_path);
static unsigned int get_int_from_net(char* buf, int len);
static int verify_server_hello(char* server_hello, size_t server_hello_len);
static int verify_client_hello(char* client_hello, size_t client_hello_len);
static int processExtList(char* inbuf, int offset, int buf_len, int* req_exts, size_t req_exts_len, int* rej_exts, size_t rej_exts_len);

// global settings structure
typedef struct cipher_settings_t {
	bool isApproved;
	int* cipherList;
	size_t cipherListSize;
	int* requiredServerExtList;
	size_t requiredServerExtListSize;
	int* rejectedServerExtList;
	size_t rejectedServerExtListSize;
	int* requiredClientExtList;
	size_t requiredClientExtListSize;
	int* rejectedClientExtList;
	size_t rejectedClientExtListSize;
} cipher_settings_t;

cipher_settings_t cipher_settings;

__declspec(dllexport) int __stdcall initialize(init_data_t* idata) {
	const char* plugin_path;
	char* config_path;
	int i;

	plog = idata->log;

	// make the path for our plugin
	plugin_path = idata->plugin_path;
	config_path = (char*)malloc(strlen(plugin_path) + strlen(CONFIG_FILE) + 1);
	strcpy(config_path, plugin_path);
	config_path[strlen(plugin_path)] = '\0';

	// put a \0 in the thing
	for (i = strlen(config_path) - 1; i>0; i--) {
		if (config_path[i] == '/' || config_path[i] == '\\') {
			config_path[i] = '\0';
			break;
		}
	}

	strcat(config_path, CONFIG_FILE);

	// load the settings
	loadconfig(config_path);

	free(config_path);
	return 0;
}

__declspec(dllexport) int __stdcall query(query_data_t* data) {
	int rval;
	plog(LOG_DEBUG, "cipher_suite: query function ran");
	if (cipher_settings.isApproved == PLUGIN_INIT_ERROR) {
		return PLUGIN_RESPONSE_ERROR;
	}
	rval = PLUGIN_RESPONSE_VALID;

	if (data->client_hello == NULL) {
		plog(LOG_DEBUG, "Bad data");
		return PLUGIN_RESPONSE_ERROR;
	}
	hexdump(data->client_hello, data->client_hello_len);
	if (data->server_hello == NULL) {
		plog(LOG_DEBUG, "Bad data");
		return PLUGIN_RESPONSE_ERROR;
	}
	//hexdump(data->server_hello, data->server_hello_len);

	rval = verify_client_hello(data->client_hello, data->client_hello_len);
	if (rval != PLUGIN_RESPONSE_VALID) {
		return rval;
	}

	rval = verify_server_hello(data->server_hello, data->server_hello_len);

	return rval;
}

__declspec(dllexport) int __stdcall finalize() {
	// free settings
	free(cipher_settings.cipherList);
	free(cipher_settings.requiredServerExtList);
	free(cipher_settings.rejectedServerExtList);
	free(cipher_settings.requiredClientExtList);
	free(cipher_settings.rejectedClientExtList);
	return 0;
}

int verify_client_hello(char* client_hello, size_t client_hello_len) {
	unsigned int offset;
	unsigned int size;
	unsigned int sessionSize;

	offset = CLIENT_HELLO_SESSION_LEN_OFF;
	if (offset + CLIENT_HELLO_SESSION_LEN_OFF > client_hello_len) {
		plog(LOG_ERROR, "Cipher Suite Plugin: Got a truncated Client Hello.");
		return PLUGIN_RESPONSE_ERROR;
	}
	sessionSize = get_int_from_net(&(client_hello[offset]), CLIENT_HELLO_SESSION_LENGTH);

	offset = offset + sessionSize + 1;
	if (offset + CLIENT_HELLO_CIPHER_LEN_SIZE > client_hello_len) {
		plog(LOG_ERROR, "Cipher Suite Plugin: Got a truncated Client Hello.");
		return PLUGIN_RESPONSE_ERROR;
	}
	size = get_int_from_net(&(client_hello[offset]), CLIENT_HELLO_CIPHER_LEN_SIZE);
	offset += CLIENT_HELLO_CIPHER_LEN_SIZE;
	offset += size;

	if (offset + CLIENT_HELLO_COMPRESS_LEN_SIZE > client_hello_len) {
		plog(LOG_ERROR, "Cipher Suite Plugin: Recieved a truncated Client Hello");
		return PLUGIN_RESPONSE_ERROR;
	}
	size = get_int_from_net(&(client_hello[offset]), CLIENT_HELLO_COMPRESS_LEN_SIZE);
	offset += CLIENT_HELLO_COMPRESS_LEN_SIZE;
	offset += size;

	offset += CLIENT_HELLO_EXT_LEN_SIZE;

	return processExtList(client_hello, offset, client_hello_len, cipher_settings.requiredClientExtList, cipher_settings.requiredClientExtListSize, cipher_settings.rejectedClientExtList, cipher_settings.rejectedClientExtListSize);
}

int verify_server_hello(char* server_hello, size_t server_hello_len) {
	unsigned int offset;
	unsigned int cipher;
	int i;
	char found;
	unsigned int sessionSize;

	if (SERVER_HELLO_SESSION_LEN_OFF > server_hello_len) {
		plog(LOG_ERROR, "Cipher Suite Plugin: Got a truncated Client Hello.");
		return PLUGIN_RESPONSE_ERROR;
	}
	offset = SERVER_HELLO_SESSION_LEN_OFF;

	sessionSize = get_int_from_net(&(server_hello[offset]), SERVER_HELLO_SESSION_LENGTH);
	offset = offset + sessionSize + 1;

	// extract the choosen cipher
	if (offset + SERVER_HELLO_CIPHER_SIZE > server_hello_len) {
		plog(LOG_ERROR, "Cipher Suite Plugin: Got truncated Server Hello");
		return PLUGIN_RESPONSE_ERROR;
	}
	cipher = get_int_from_net(&(server_hello[offset]), SERVER_HELLO_CIPHER_SIZE);
	//plog(LOG_DEBUG, "We see a cipher of %x", cipher);
	// check the cipher
	found = 0;
	for (i = 0; i<cipher_settings.cipherListSize; i++) {
		if (cipher == cipher_settings.cipherList[i]) {
			if (cipher_settings.isApproved) {
				// we found an approved cipher
				found = 1;
				break;
			}
			else {
				// we found a bad cipher
				plog(LOG_INFO, "Cipher Suite Plugin: Found a rejected cipher suite");
				return PLUGIN_RESPONSE_INVALID;
			}
		}
	}
	if (cipher_settings.isApproved && !found) {
		plog(LOG_INFO, "Cipher Suite Plugin: Didn't find an accepted cipher suite");
		return PLUGIN_RESPONSE_INVALID;
	}

	offset = offset + SERVER_HELLO_CIPHER_SIZE + SERVER_HELLO_COMPRESSION_SIZE + All_EXT_SIZE;
	return processExtList(server_hello, offset, server_hello_len, cipher_settings.requiredServerExtList, cipher_settings.requiredServerExtListSize, cipher_settings.rejectedServerExtList, cipher_settings.rejectedServerExtListSize);
}

int processExtList(char* inbuf, int offset, int buf_len, int* req_exts, size_t req_exts_len, int* rej_exts, size_t rej_exts_len) {
	int num_req_found;
	int i;
	unsigned int ext;
	unsigned int ext_len;

	num_req_found = 0;



	while (offset + EXT_SIZE + EXT_LEN_SIZE <= buf_len) {
		// get the ext id
		ext = get_int_from_net(&(inbuf[offset]), EXT_SIZE);
		//plog(LOG_DEBUG, "We see an ext of %04x", ext);
		for (i = 0; i<req_exts_len; i++) {
			if (ext == req_exts[i]) {
				num_req_found++;
				// TODO doesn't account for duplicate extentions
				break;
			}
		}
		for (i = 0; i<rej_exts_len; i++) {
			if (ext == rej_exts[i]) {
				plog(LOG_INFO, "Cipher Suite Plugin: Found a rejected extention");
				//return PLUGIN_RESPONSE_INVALID;
			}
		}

		// check the ext
		// get the size
		offset += EXT_SIZE;
		ext_len = get_int_from_net(&(inbuf[offset]), EXT_LEN_SIZE);

		// offset to the next one
		offset += EXT_LEN_SIZE;
		offset += ext_len;
	}
	if (num_req_found < req_exts_len) {
		plog(LOG_INFO, "Cipher Suite Plugin: Did not find a required extention");
		return PLUGIN_RESPONSE_INVALID;
	}
	return PLUGIN_RESPONSE_VALID;
}

unsigned int get_int_from_net(char* inbuf, int len) {
	int i;
	unsigned int response = 0;
	unsigned char* buf;
	buf = (unsigned char*)inbuf;

	for (i = 0; i<len; i++) {
		response += ((unsigned)buf[i]) << (8 * (len - (i + 1)));

	}
	return response;
}

bool loadconfig(const char* config_path) {
	libconfig::Config cfg;
	plog(LOG_DEBUG, "Loading configuration at %s", config_path);
	try {
		cfg.readFile(config_path);
	}
	catch (const libconfig::FileIOException &fioex) {
		plog(LOG_ERROR, "Cipher Suite Plugin could not open config file at %s", config_path);
		cipher_settings.isApproved = PLUGIN_INIT_ERROR;
		return false;
	}
	catch (const libconfig::ParseException &pex) {
		plog(LOG_ERROR, "Parse error at %s : %d - %s", pex.getFile(), pex.getLine(), pex.getError());
		cipher_settings.isApproved = PLUGIN_INIT_ERROR;
		return false;
	}
	try {
		bool status = cfg.lookupValue("approved_ciphers", cipher_settings.isApproved);
		if (!status)
		{
			plog(LOG_ERROR, "Broken config file for cipher suite plugin.");
			cipher_settings.isApproved = PLUGIN_INIT_ERROR;
			return false;
		}
		int count;
		//cipher_list
		const libconfig::Setting& ciphers_list = cfg.lookup("ciphers_list");
		count = ciphers_list.getLength();
		cipher_settings.cipherListSize = count;
		cipher_settings.cipherList = new int[count];
		if (!ciphers_list.isList() && !ciphers_list.isGroup() && !ciphers_list.isArray())
		{
			plog(LOG_ERROR, "Broken config file for cipher suite plugin, no 'ciphers_list'");
			cipher_settings.isApproved = PLUGIN_INIT_ERROR;
			return false;
		}

		for (int i = 0; i<count; i++) {
			cipher_settings.cipherList[i] = ciphers_list[i];
		}

		//required_server_extentions
		const libconfig::Setting& required_server_extentions = cfg.lookup("required_server_extentions");
		count = required_server_extentions.getLength();
		cipher_settings.requiredServerExtListSize = count;
		cipher_settings.requiredServerExtList = new int[count];

		if (!required_server_extentions.isList() && !required_server_extentions.isGroup() && !required_server_extentions.isArray())
		{
			plog(LOG_ERROR, "required_server_extentions is not a list, group, or array");
			cipher_settings.isApproved = PLUGIN_INIT_ERROR;
			return false;
		}
		for (int i = 0; i<count; i++) {
			cipher_settings.requiredServerExtList[i] = required_server_extentions[i];
		}

		//rejected_server_extentions
		const libconfig::Setting& rejected_server_extentions = cfg.lookup("rejected_server_extentions");
		count = rejected_server_extentions.getLength();
		cipher_settings.rejectedServerExtListSize = count;
		cipher_settings.rejectedServerExtList = new int[count];

		if (!rejected_server_extentions.isList() && !rejected_server_extentions.isGroup() && !rejected_server_extentions.isArray())
		{
			plog(LOG_ERROR, "rejected_server_extentions is not a list, group, or array");
			cipher_settings.isApproved = PLUGIN_INIT_ERROR;
			return false;
		}
		for (int i = 0; i<count; i++) {
			cipher_settings.rejectedServerExtList[i] = rejected_server_extentions[i];
		}

		//required_client_extentions
		const libconfig::Setting& required_client_extentions = cfg.lookup("required_client_extentions");
		count = required_client_extentions.getLength();
		cipher_settings.requiredClientExtListSize = count;
		cipher_settings.requiredClientExtList = new int[count];

		if (!required_client_extentions.isList() && !required_client_extentions.isGroup() && !required_client_extentions.isArray())
		{
			plog(LOG_ERROR, "required_client_extentions is not a list, group, or array");
			cipher_settings.isApproved = PLUGIN_INIT_ERROR;
			return false;
		}
		for (int i = 0; i<count; i++) {
			cipher_settings.requiredClientExtList[i] = required_client_extentions[i];
		}

		//rejected_client_extentions
		const libconfig::Setting& rejected_client_extentions = cfg.lookup("rejected_client_extentions");
		count = rejected_client_extentions.getLength();
		cipher_settings.rejectedClientExtListSize = count;
		cipher_settings.rejectedClientExtList = new int[count];

		if (!rejected_client_extentions.isList() && !rejected_client_extentions.isGroup() && !rejected_client_extentions.isArray())
		{
			plog(LOG_ERROR, "rejected_client_extentions is not a list, group, or array");
			cipher_settings.isApproved = PLUGIN_INIT_ERROR;
			return false;
		}
		for (int i = 0; i<count; i++) {
			cipher_settings.rejectedClientExtList[i] = rejected_client_extentions[i];
		}
	}
	catch (const libconfig::SettingNotFoundException &nfex) {
		plog(LOG_ERROR, "Could not find setting %s", nfex.getPath());
		cipher_settings.isApproved = PLUGIN_INIT_ERROR;
		return false;
	}
	catch (...) {
		plog(LOG_ERROR, "Incorrectly formed config file %s", config_path);
		cipher_settings.isApproved = PLUGIN_INIT_ERROR;
		return false;
	}

	return true;
}

void hexdump(char* data, size_t len) {
	char* formatted;
	char* formatter;
	int i;

	formatted = (char*)malloc((len * 3) * sizeof(char));

	for (i = 0; i<len; i++) {
		if (!((i + 1) % 8)) {
			formatter = "\n";
		}
		else if (!((i + 1) % 4)) {
			formatter = "\t";
		}
		else {
			formatter = " ";
		}
		snprintf(formatted + (i * 3), 27, "%02x%s", (unsigned char)data[i], formatter);
	}
	plog(LOG_DEBUG, "\n%s\n", formatted);
}
