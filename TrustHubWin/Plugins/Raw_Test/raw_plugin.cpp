#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "trusthub_plugin.h"
#include "THLogger_Level.h"

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
int(*plog)(thlog_level_t level, const char* format, ...);

// Plugins must have an exported "query" function that takes a query_data_t* argument
// Plugins must include the "trusthub_plugin.h" header
// In visual studio, add the path to the Policy engine code under:
//   Configuration Properties->C/C++->General->Additional Include Directories
__declspec(dllexport) int __stdcall query(query_data_t*) {
	plog(LOG_DEBUG, "Raw Plugin: query function ran");
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

// An optional DllMain can be added for a more precise initialization and finalization