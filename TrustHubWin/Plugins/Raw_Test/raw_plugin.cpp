#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "trusthub_plugin.h"
#include "THLogger.h"

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

// Plugins must have an exported "query" function that takes a query_data_t* argument
// Plugins must include the "trusthub_plugin.h" header
// In visual studio, add the path to the Policy engine code under:
//   Configuration Properties->C/C++->General->Additional Include Directories
__declspec(dllexport) int __cdecl query(query_data_t*) {
	thlog() << "\n******Plugin ran******\n\n";
	//printf("\n******Plugin ran******\n\n");
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

// An optional DllMain can be added for a more precise initialization and finalization