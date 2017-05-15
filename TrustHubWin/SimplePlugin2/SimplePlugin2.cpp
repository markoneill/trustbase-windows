// SimplePlugin2.cpp : Defines the exported functions for the DLL application
//					 : Uses an asynchronous response

#include "stdafx.h"
#include <thread>
#include "THLogger.h"
#include "trusthub_plugin.h"

// globals
// WARNING : Multiple instances of the same plugin will actually share globals
void sleep_then_respond(int query_id);
int plugin_id;
int (*th_callback)(int plugin_id, int query_id, int plugin_response);

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

// Plugins must have an exported "query" function that takes a query_data_t* argument
// Plugins must include the "trusthub_plugin.h" header
// In visual studio, add the path to the Policy engine code under:
//   Configuration Properties->C/C++->General->Additional Include Directories
__declspec(dllexport) int __stdcall query(query_data_t* qdata) {
	std::thread(sleep_then_respond, qdata->id).detach();
	return 0; // Doesn't matter, we are going to respond via async stuff
}

// Plugins can also have an optional exported "initialize" function that takes an init_data_t* arg
__declspec(dllexport) int __stdcall initialize(init_data_t* idata) {
	// save the thing
	th_callback = idata->callback;
	plugin_id = idata->plugin_id;
	return PLUGIN_INITIALIZE_OK;
}

// Plugins can also have an optional exported "finalize" function that takes no arg
__declspec(dllexport) int __stdcall finalize() {
	return PLUGIN_FINALIZE_OK;
}

// An optional DllMain can be added for a more precise initialization and finalization


void sleep_then_respond(int query_id) {
	Sleep(500);
	th_callback(plugin_id, query_id, PLUGIN_RESPONSE_VALID);
}