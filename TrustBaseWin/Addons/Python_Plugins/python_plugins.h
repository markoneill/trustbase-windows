#ifndef PYTHON_PLUGINS_H_
#define PYTHON_PLUGINS_H_

#include <stddef.h>
#include "trustbase_plugin.h"

/*
*	Begin using this library with the initialize() function.
*	Dynamically load python scripts as plugins with load_plugins().
*	Query a loaded plugin with query().
*	Finally, unload plugins and free memory with finalize().
*/

#ifdef __cplusplus
extern "C" {  // only need to export C interface if  
			  // used by C++ source code  
#endif  

/**** API ****/

/*
*	Initializes the Python Interpreter and allocates memory for plugin pointers.
*	Must be called before any other functions.
*
*	data: an init_addon_data_t which contains the require 
*			information for initializing the python addon
*
*	returns EXIT_SUCCESS on success and EXIT_FAILURE on failure
*/
__declspec(dllexport) int __stdcall initialize(init_addon_data_t*);

/*
*	Finalizes the Python Interpreter and frees memory from plugin pointers.
*	Do not call any other functions after this function.
*
*	returns EXIT_SUCCESS on success and EXIT_FAILURE on failure
*/
__declspec(dllexport) int __stdcall finalize(void);

/*
*	Loads a specified python module into memory as a plugin, which remains in
*	memory until finalize() is called.
*
*	id: a non-negative integer that will be used as an index in an array
*		containing pointers to plugin functions; also used as an identifier
*		for which plugin to query using the query() function
*
*	file_name: the name of the Python file to be imported into the Python
*				interpreter. The search for this module begins in the
*				plugin_directory parameter of the initialize() function.
*
*	is_async: 0 if not async, 1 if is async
*
*	returns EXIT_SUCCESS on success and EXIT_FAILURE on failure
*/
__declspec(dllexport) int __stdcall load_plugin(int id, char *file_name, int is_async);


/*
*	id: a non-negative integer that is used as an index in an array
*		containing pointers to plugin functions; it is the identifier
*		for which plugin to query as assigned by the load_plugin() function
*
*	data: an query_data_t which contains the required information about the query
*
*	returns result: the plugin's opinion on the certificate
*		PLUGIN_RESPONSE_ERROR  -1
*		PLUGIN_RESPONSE_INVALID 0
*		PLUGIN_RESPONSE_VALID   1
*		PLUGIN_RESPONSE_ABSTAIN 2
*/
__declspec(dllexport) int __stdcall query_plugin(int id, query_data_t* data);

/*
*	id: a non-negative integer that is used as an index in an array
*		containing pointers to plugin functions; it is the identifier
*		for which plugin to query as assigned by the load_plugin() function
*
*	data: an query_data_t which contains the required information about the query
*
*	returns result: the plugin's opinion on the certificate
*		PLUGIN_RESPONSE_ERROR  -1
*		PLUGIN_RESPONSE_INVALID 0
*		PLUGIN_RESPONSE_VALID   1
*		PLUGIN_RESPONSE_ABSTAIN 2
*/
__declspec(dllexport) int __stdcall query_plugin_async(int id, query_data_t* data);
#ifdef __cplusplus
}
#endif  

/**	The callback function for asynchronous python plugins
*
*	result: the plugin's opinion on the certificate
*		PLUGIN_RESPONSE_ERROR  -1
*		PLUGIN_RESPONSE_INVALID 0
*		PLUGIN_RESPONSE_VALID   1
*		PLUGIN_RESPONSE_ABSTAIN 2
*	plugin_id: the plugin's assigned id
*	query_id: the id of the async query
*/
int callback(int result, int plugin_id, int query_id);

#ifdef __cplusplus
extern "C" {  // only need to export C interface if  
			  // used by C++ source code  
#endif  
/**
*	Calls the finalize function as part of the python plugin file	
*	This is the thread safe version.
*
*	id: a non-negative integer that is used as an index in an array
*		containing pointers to plugin functions; it is the identifier
*		for which plugin to query as assigned by the load_plugin() function
*
*/
__declspec(dllexport) int __stdcall finalize_plugin(int id);
#ifdef __cplusplus
}
#endif  

#endif
