/*++

Module Name:

    driver.c

Abstract:

    This file contains the driver entry points.

Environment:

    Kernel-mode Driver Framework a

--*/

#include "driver.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, DriverEntry)
#endif

// Initializes required WDFDriver and WDFDevice objects
NTSTATUS TBInitDriverAndDevice(IN DRIVER_OBJECT * driver_obj, IN UNICODE_STRING * registry_path);
NTSTATUS TBRegisterCallouts(IN DEVICE_OBJECT * wdm_device);
NTSTATUS TBRegisterStreamCallout(IN DEVICE_OBJECT * wdm_device, IN HANDLE engineHandle);
NTSTATUS TBRegisterALECallout(IN DEVICE_OBJECT * wdm_device, IN HANDLE engineHandle);
NTSTATUS unregister_trustbase_callouts();
void tbdriver_evt_unload(IN WDFDRIVER Driver);

// Initializes required WDFDriver and WDFDevice objects
NTSTATUS TBInitDriverAndDevice(IN DRIVER_OBJECT * driver_obj, IN UNICODE_STRING * registry_path) {
	NTSTATUS status = STATUS_SUCCESS;

	WDF_DRIVER_CONFIG config = { 0 };
	WDFDRIVER driver;
	WDFDEVICE device;
	PWDFDEVICE_INIT device_init = NULL;
	PDEVICE_OBJECT wdm_device = NULL;


	WDF_DRIVER_CONFIG_INIT(&config, WDF_NO_EVENT_CALLBACK); //WDF_NO_EVENT_CALLBACK = NULL, this means there is no callback when WdfDriverCreate is called.
	config.DriverInitFlags |= WdfDriverInitNonPnpDriver; //WdfDriverInitNonPnpDriver means this driver does not support plug and play. "If this value is set, the driver must not supply an EvtDriverDeviceAdd callback function" in other words use WDF_NO_EVENT_CALLBACK
	config.EvtDriverUnload = tbdriver_evt_unload;

	// Create a WDFDRIVER for this driver
	status = WdfDriverCreate(driver_obj, registry_path, WDF_NO_OBJECT_ATTRIBUTES, &config, &driver); //WDF_NO_OBJECT_ATTRIBUTES means "extra, nonpageable, memory space"
	if (!NT_SUCCESS(status)) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Could not create WDF Driver\r\n");
		return status;
	}

	DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Created WDF Driver\r\n");

	device_init = WdfControlDeviceInitAllocate(driver, &SDDL_DEVOBJ_SYS_ALL_ADM_ALL); // SDDL_DEVOBJ_ALL_ADM_ALL allows kernel, system, and admin
	if (device_init == NULL) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Could not init WDF Driver, insufficient resources\r\n");
		status = STATUS_INSUFFICIENT_RESOURCES;
		return status;
	}

	// Allocate a device
	DECLARE_CONST_UNICODE_STRING(ntDeviceName, TRUSTBASE_DEVICENAME);
	DECLARE_CONST_UNICODE_STRING(symbolicName, TRUSTBASE_SYMNAME);

	// Configure the WDFDEVICE_INIT with a name to allow for access from user mode
	// this section must be done before the WdfDeviceCreate is called.

	WdfDeviceInitSetDeviceType(device_init, FILE_DEVICE_NETWORK);
	WdfDeviceInitSetCharacteristics(device_init, FILE_DEVICE_SECURE_OPEN, FALSE);//"Most drivers specify only the FILE_DEVICE_SECURE_OPEN characteristic. This ensures that the same security settings are applied to any open request into the device's namespace"

	// set to exclusive, so only one app can talk to the device at a time
	WdfDeviceInitSetExclusive(device_init, TRUE);
	
	// set Io type to buffered, if we need faster for large amounts of data, we should change to 
	WdfDeviceInitSetIoType(device_init, WdfDeviceIoBuffered);

	status = WdfDeviceInitAssignName(device_init, &ntDeviceName);
	if (!NT_SUCCESS(status)) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Could not assign driver name\r\n");
		if (device_init) {
			WdfDeviceInitFree(device_init);
		}
		return status;
	}

	// if we want a context, we need to specify the size with WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE in the atributes

	// Create a WDFDEVICE for this driver
	status = WdfDeviceCreate(&device_init, WDF_NO_OBJECT_ATTRIBUTES, &device);
	if (!NT_SUCCESS(status)) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Could not create a device for the driver\r\n");
		if (device_init) {
			WdfDeviceInitFree(device_init);
		}
		return status;
	}

	status = WdfDeviceCreateSymbolicLink(device, &symbolicName);
	if (!NT_SUCCESS(status)) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Could not create a symbolic link for the driver\r\n");
		if (device_init) {
			WdfDeviceInitFree(device_init);
		}
		return status;
	}
	
	// Set up our IO queues
	status = TbInitQueues(device);
	if (!NT_SUCCESS(status)) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Could not create the queues for our IOCtl messages\r\n");
		if (device_init) {
			WdfDeviceInitFree(device_init);
		}
		return status;
	}

	// Set up any work items
	status = TbInitWorkItems(device);
	if (!NT_SUCCESS(status)) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Could not create the work items\r\n");
		if (device_init) {
			WdfDeviceInitFree(device_init);
		}
		return status;
	}

	WdfControlFinishInitializing(device);

	wdm_device = WdfDeviceWdmGetDeviceObject(device);
	g_wdm_device = device; //use device instead of wdm_device - keep track of the device to clean it up later

	// Register callouts
	status = TBRegisterCallouts(wdm_device);
	if (!NT_SUCCESS(status)) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Could not set callouts\r\n");
		if (device_init) {
			WdfDeviceInitFree(device_init);
		}
		return status;
	}
	
	return status;
}

NTSTATUS TBRegisterCallouts(IN DEVICE_OBJECT * wdm_device) {
	NTSTATUS status = STATUS_SUCCESS;
	FWPM_SESSION session = { 0 };
	HANDLE engineHandle;
	FWPM_SUBLAYER tbSubLayer;


	// Open Engine
	session.flags = FWPM_SESSION_FLAG_DYNAMIC;

	status = FwpmEngineOpen(NULL, RPC_C_AUTHN_WINNT, NULL, &session, &engineHandle);
	g_EngineHandle = engineHandle;
	if (!NT_SUCCESS(status)) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Could not open engine\r\n");
		return status;
	}

	// Start Transaction
	status = FwpmTransactionBegin(engineHandle, 0);
	if (!NT_SUCCESS(status)) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Could not open Transaction\r\n");
		FwpmEngineClose(engineHandle);
		return status;
	}

	// Add SubLayer for our filters
	RtlZeroMemory(&tbSubLayer, sizeof(FWPM_SUBLAYER));
	tbSubLayer.subLayerKey = TRUSTBASE_SUBLAYER;
	tbSubLayer.displayData.name = L"TrustBase Sub layer";
	tbSubLayer.displayData.description = L"TrustBase Sample Sub layer";
	tbSubLayer.flags = 0;
	tbSubLayer.weight = 0;

	status = FwpmSubLayerAdd(engineHandle, &tbSubLayer, NULL);
	if (!NT_SUCCESS(status)) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Could not create sublayer\r\n");
		if (!NT_SUCCESS(FwpmTransactionAbort(engineHandle))) {
			DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Could not abort Transaction?\r\n");
		}
		FwpmEngineClose(engineHandle);
		return status;
	}

	// ALE FlOW_ESTABLISHED layer for tracking app pid and stuff
	status = TBRegisterALECallout(wdm_device, engineHandle);
	if (!NT_SUCCESS(status)) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Could not set ALE callouts\r\n");
		return status;
	}

	// TCP stream layer callouts for data inspection
	
	status = TBRegisterStreamCallout(wdm_device, engineHandle);
	if (!NT_SUCCESS(status)) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Could not set Stream callouts\r\n");
		return status;
	}
	// Outbound transport layer for delaying response, and ip information

	// Commit our transaction
	
	status = FwpmTransactionCommit(engineHandle);
	if (status) {
		__debugbreak();
	}

	return status;
}

NTSTATUS TBRegisterALECallout(IN DEVICE_OBJECT * wdm_device, IN HANDLE engineHandle) {

	NTSTATUS status = STATUS_SUCCESS;
	FWPS_CALLOUT sCallout = { 0 };
	FWPM_CALLOUT mCallout = { 0 };
	FWPM_DISPLAY_DATA displayData = { 0 };
	FWPM_FILTER filter;
	FWPM_FILTER_CONDITION filterConditions[1]; // only need 1 for the ALE stuff

	// Register a new Callout with the Filter Engine using the provided callout functions
	sCallout.calloutKey = TRUSTBASE_ALE_CALLOUT_V4;
	sCallout.classifyFn = trustbaseALECalloutClassify;
	sCallout.notifyFn = trustbaseALECalloutNotify;
	sCallout.flowDeleteFn = trustbaseALECalloutFlowDelete;
	sCallout.flags |=  FWP_CALLOUT_FLAG_ALLOW_OFFLOAD;
	status = FwpsCalloutRegister((void *)wdm_device, &sCallout, &TrustBase_callout_ale_id); 
	if (!NT_SUCCESS(status)) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Could not register ALE callouts\r\n");
		if (TrustBase_callout_ale_id) {
			FwpsCalloutUnregisterById(TrustBase_callout_ale_id);
			TrustBase_callout_ale_id = 0;
		}
		return status;
	}
	DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Registed ALE Callouts\r\n");

	// Add Callouts
	displayData.name = L"Trustbase ALE Callouts";
	displayData.description = L"Gets metadata for our stream callouts";
	mCallout.displayData = displayData;
	mCallout.calloutKey = TRUSTBASE_ALE_CALLOUT_V4;
	mCallout.applicableLayer = FWPM_LAYER_ALE_FLOW_ESTABLISHED_V4;
	status = FwpmCalloutAdd(engineHandle, &mCallout, NULL, &aleCallId);
	if (!NT_SUCCESS(status)) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Could not add ALE callouts\r\n\r\n");
		if (engineHandle) {
			FwpmEngineClose(engineHandle);
			engineHandle = NULL;
		}
		return status;
	}
	DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Added ALE Callouts\r\n");

	// Add Filters
	RtlZeroMemory(&filter, sizeof(FWPM_FILTER));

	filter.displayData.name = L"ALE Layer Filter";
	filter.displayData.description = L"Monitors ALE traffic.";
	filter.layerKey = FWPM_LAYER_ALE_FLOW_ESTABLISHED_V4;
	filter.action.type = FWP_ACTION_CALLOUT_INSPECTION; // only inspect at this layer
	filter.action.calloutKey = TRUSTBASE_ALE_CALLOUT_V4;
	filter.subLayerKey = TRUSTBASE_SUBLAYER;
	filter.weight.type = FWP_EMPTY; // auto-weight.
	filter.numFilterConditions = 1;
	RtlZeroMemory(filterConditions, sizeof(filterConditions));
	filter.filterCondition = filterConditions;
	// only watch TCP traffic
	filterConditions[0].fieldKey = FWPM_CONDITION_IP_PROTOCOL;
	filterConditions[0].matchType = FWP_MATCH_EQUAL;
	filterConditions[0].conditionValue.type = FWP_UINT8;
	filterConditions[0].conditionValue.uint8 = IPPROTO_TCP;

	status = FwpmFilterAdd(engineHandle, &filter, NULL, &alefilterId);
	if (!NT_SUCCESS(status)) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Could not add ALE filter\r\n");
		if (!NT_SUCCESS(FwpmTransactionAbort(engineHandle))) {
			DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Could not abort Transaction?\r\n");
		}
		FwpmEngineClose(engineHandle);
		return status;
	}

	return status;
}

NTSTATUS TBRegisterStreamCallout(IN DEVICE_OBJECT * wdm_device, IN HANDLE engineHandle) {
	NTSTATUS status = STATUS_SUCCESS;
	FWPS_CALLOUT sCallout = { 0 };
	FWPM_CALLOUT mCallout = { 0 };
	FWPM_DISPLAY_DATA displayData = { 0 };
	FWPM_FILTER filter;
	FWPM_FILTER_CONDITION filterConditions[1];


	// Register a new Callout with the Filter Engine using the provided callout functions
	sCallout.calloutKey = TRUSTBASE_STREAM_CALLOUT_V4;
	sCallout.classifyFn = trustbaseCalloutClassify;
	sCallout.notifyFn = trustbaseCalloutNotify;
	sCallout.flowDeleteFn = trustbaseCalloutFlowDelete;
	sCallout.flags |= FWP_CALLOUT_FLAG_ALLOW_MID_STREAM_INSPECTION;
	sCallout.flags |= FWP_CALLOUT_FLAG_CONDITIONAL_ON_FLOW; // only get called if our ALE callout has set a context already
	status = FwpsCalloutRegister((void *)wdm_device, &sCallout, &TrustBase_callout_stream_id);
	if (!NT_SUCCESS(status)) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Could not register callouts\r\n");
		if (TrustBase_callout_stream_id) {
			FwpsCalloutUnregisterById(TrustBase_callout_stream_id);
			TrustBase_callout_stream_id = 0;
		}
		return status;
	}
	DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Registed Callouts\r\n");

	// Add Callouts
	displayData.name = L"Trustbase Callouts";
	displayData.description = L"Intercepts stream data, looking for TLS, then reviews certificates";
	mCallout.displayData = displayData;
	mCallout.calloutKey = TRUSTBASE_STREAM_CALLOUT_V4;
	mCallout.applicableLayer = FWPM_LAYER_STREAM_V4;
	status = FwpmCalloutAdd(engineHandle, &mCallout, NULL, &streamCallId);
	if (!NT_SUCCESS(status)) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Could not add callouts\r\n\r\n");
		if (engineHandle) {
			FwpmEngineClose(engineHandle);
			engineHandle = NULL;
		}
		return status;
	}
	DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Added Callouts\r\n");

	// Add Filters
	RtlZeroMemory(&filter, sizeof(FWPM_FILTER));

	filter.displayData.name = L"Stream Layer Filter";
	filter.displayData.description = L"Monitors TCP traffic.";
	filter.layerKey = FWPM_LAYER_STREAM_V4;
	filter.action.type = FWP_ACTION_CALLOUT_UNKNOWN; // we may block or permit stuff
	filter.action.calloutKey = TRUSTBASE_STREAM_CALLOUT_V4;
	filter.subLayerKey = TRUSTBASE_SUBLAYER;
	filter.weight.type = FWP_EMPTY; // auto-weight.
	filter.numFilterConditions = 0; //if we were to add filters to the filterConditions array, we would want to include the amount here
	RtlZeroMemory(filterConditions, sizeof(filterConditions));
	filter.filterCondition = filterConditions;

	status = FwpmFilterAdd(engineHandle, &filter, NULL, &streamfilterId);
	if (!NT_SUCCESS(status)) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Could not add Stream Filter\r\n");
		if (!NT_SUCCESS(FwpmTransactionAbort(engineHandle))) {
			DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Could not abort Transaction?\r\n");
		}
		FwpmEngineClose(engineHandle);
		return status;
	}

	return status;
}

NTSTATUS unregister_trustbase_callouts() {
	NTSTATUS status1;
	NTSTATUS status2;

	//Call remove context on all of the active flows
	int i;
	for (i = 0; i < MAX_C; i++) {
		if (contextArray[i] != 0) {
			FwpsFlowRemoveContext(contextArray[i], FWPS_LAYER_STREAM_V4, TrustBase_callout_stream_id);
		}
	}

	//Unregister the Stream callout
	do {
		status2 = FwpsCalloutUnregisterById(TrustBase_callout_stream_id);
	} while (status2 == STATUS_DEVICE_BUSY);

	if (status2) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Stream callout could not be unregistered, last error = %d\r\n", status2);
	}
	else {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Stream callout unregistered! \r\n");
	}

	//Unregister the ALE callout
	status1 = FwpsCalloutUnregisterById(TrustBase_callout_ale_id);
	if (status1) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "ALE callout could not be unregistered, last error = %d\r\n", status1);
	}
	else {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "ALE callout unregistered! \r\n");
	}

	return STATUS_SUCCESS;
}

/*++
Routine Description:
DriverEntry initializes the driver and is the first routine called by the  
system after the driver is loaded. DriverEntry specifies the other entry
points in the function driver, such as EvtDevice and tbdriver_evt_unload.

Parameters Description:

DriverObject - represents the instance of the function driver that is loaded
into memory. DriverEntry must initialize members of DriverObject before it
returns to the caller. DriverObject is allocated by the system before the
driver is loaded, and it is released by the system after the system unloads
the function driver from memory.

RegistryPath - represents the driver specific path in the Registry.
The function driver can use the path to store driver related data between
reboots. The path does not store hardware instance specific data.

Return Value:

STATUS_SUCCESS if successful,
STATUS_UNSUCCESSFUL otherwise.
--*/
NTSTATUS DriverEntry(IN PDRIVER_OBJECT  DriverObject, IN PUNICODE_STRING RegistryPath) {

    NTSTATUS status;
	DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "DriverEntry In\r\n");
	DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Registry Path %ws\r\n", RegistryPath);

	//Initialize an array to keep track of flow contexts so we may clean up properly afterward
	int i = 0;
	for (i = 0; i < MAX_C; i++) {
		contextArray[i] = 0;
	}
	
	cleanup = 0; //cleanup false

	//create the driver and device object
	status = TBInitDriverAndDevice(DriverObject, RegistryPath);
    if (!NT_SUCCESS(status)) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Could not init Driver and Device\r\n");
        return status;
    }

	DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "DriverEntry Out\r\n");

    return status;
}


VOID TBCleanUp() {
	ULONG QueueRequests;
	ULONG DriverRequests;
	WDF_IO_QUEUE_STATE state;
	state = WdfIoQueueGetState(TBReadQueue, &QueueRequests, &DriverRequests);
	//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Number of read requests in queue = %d\r\n", QueueRequests);
	//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Number of read requests at the driver = %d\r\n", DriverRequests);
	//state = WdfIoQueueGetState(TBWriteQueue, &QueueRequests, &DriverRequests);
	//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Number of write requests in queue = %d\r\n", QueueRequests);
	//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Number of write requests at the driver = %d\r\n", DriverRequests);
	WdfIoQueuePurgeSynchronously(TBReadQueue);
	WdfIoQueuePurgeSynchronously(TBWriteQueue);
	//state = WdfIoQueueGetState(TBReadQueue, &QueueRequests, &DriverRequests);
	//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Number of read requests in queue = %d\r\n", QueueRequests);
	//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Number of read requests at the driver = %d\r\n", DriverRequests);
	//state = WdfIoQueueGetState(TBWriteQueue, &QueueRequests, &DriverRequests);
	//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Number of write requests in queue = %d\r\n", QueueRequests);
	//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Number of write requests at the driver = %d\r\n", DriverRequests);
	//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "PURGED THE QUEUES!!!!!!!!!!!!!\r\n");
	return;
}

// Clean up the Driver -- Similiar to DriverUnload(IN PDRIVER_OBJECT driver_obj)
void tbdriver_evt_unload(IN WDFDRIVER Driver) {
	UNREFERENCED_PARAMETER(Driver);
	UNICODE_STRING symlink = { 0 };
	NTSTATUS status;
	cleanup = 1;
	DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "--- TrustBaseWin unload event ---\r\n");
	
	TBCleanUp();
	status = unregister_trustbase_callouts();	
	if (status != 0) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "--- Couldn't unregister all callouts! ---\r\n");
	}

	RtlInitUnicodeString(&symlink, TRUSTBASE_SYMNAME);
	status = IoDeleteSymbolicLink(&symlink); //Delete the symbolic link
	if (status != 0) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "--- ERROR!!!!Symbolic link failed to delete sucessfully ---\r\n");
	}

	FwpmEngineClose(g_EngineHandle);	//Close the handle to the Filter Engine
	WdfObjectDelete(g_wdm_device);      //This device is actually a WDFdevice. Device must be deleted before the driver can be stopped
	return;

	//Handshake handler has some intel specific code inside that needs to be changed?
}
