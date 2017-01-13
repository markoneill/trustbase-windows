/*++

Module Name:

    driver.c

Abstract:

    This file contains the driver entry points.

Environment:

    Kernel-mode Driver Framework

--*/

#include "driver.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, DriverEntry)
#endif

// Initializes required WDFDriver and WDFDevice objects
NTSTATUS THInitDriverAndDevice(_In_ DRIVER_OBJECT * driver_obj, _In_ UNICODE_STRING * registry_path);
NTSTATUS THRegisterCallouts(_In_ DEVICE_OBJECT * wdm_device);
NTSTATUS unregister_trusthub_callouts();
void thdriver_evt_unload(_In_ WDFDRIVER Driver);

// Initializes required WDFDriver and WDFDevice objects
NTSTATUS THInitDriverAndDevice(_In_ DRIVER_OBJECT * driver_obj, _In_ UNICODE_STRING * registry_path) {
	NTSTATUS status = STATUS_SUCCESS;

	WDF_DRIVER_CONFIG config = { 0 };
	WDFDRIVER driver;
	WDFDEVICE device;
	PWDFDEVICE_INIT device_init = NULL;
	PDEVICE_OBJECT wdm_device = NULL;
	WDF_OBJECT_ATTRIBUTES attributes;
	WDF_FILEOBJECT_CONFIG fileConfig;
	WDF_IO_QUEUE_CONFIG ioQueueConfig;
	WDFQUEUE queue;

	WDF_DRIVER_CONFIG_INIT(&config, WDF_NO_EVENT_CALLBACK); //WDF_NO_EVENT_CALLBACK = NULL, this means there is no callback when WdfDriverCreate is called.
	config.DriverInitFlags |= WdfDriverInitNonPnpDriver; //WdfDriverInitNonPnpDriver means this driver does not support plug and play. "If this value is set, the driver must not supply an EvtDriverDeviceAdd callback function" in other words use WDF_NO_EVENT_CALLBACK
	config.EvtDriverUnload = thdriver_evt_unload;

	// Create a WDFDRIVER for this driver
	status = WdfDriverCreate(driver_obj, registry_path, WDF_NO_OBJECT_ATTRIBUTES, &config, &driver); //WDF_NO_OBJECT_ATTRIBUTES means "extra, nonpageable, memory space"
	if (!NT_SUCCESS(status)) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Could not create WDF Driver\r\n");
		return status;
	}

	DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Created WDF Driver\r\n");

	// Allocate a device
	device_init = WdfControlDeviceInitAllocate(driver, &SDDL_DEVOBJ_KERNEL_ONLY);
	if (device_init == NULL) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Could not init WDF Driver, insufficient resources\r\n");
		status = STATUS_INSUFFICIENT_RESOURCES;
		return status;
	}
	
	DECLARE_CONST_UNICODE_STRING(ntDeviceName, TRUSTHUB_DEVICENAME);
	DECLARE_CONST_UNICODE_STRING(symbolicName, TRUSTHUB_SYMNAME);

	//Configure the WDFDEVICE_INIT with a name to allow for access from user mode
	//this section must be done before the WdfDeviceCreate is called.

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

	// Make a config for this object, so we can handler create, close but no cleanup requests
	WDF_FILEOBJECT_CONFIG_INIT(&fileConfig, ThIODeviceFileCreate, ThIOEvtFileClose, WDF_NO_EVENT_CALLBACK);

	WdfDeviceInitSetFileObjectConfig(device_init, &fileConfig, WDF_NO_OBJECT_ATTRIBUTES);

	// if we want a context, we need to specify the size with WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE in the atributes

	// Create a WDFDEVICE for this driver
	status = WdfDeviceCreate(&device_init, &attributes, &device);
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

	// configure the default queue
	WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&ioQueueConfig, WdfIoQueueDispatchSequential);
	ioQueueConfig.EvtIoRead = ThIoRead;
	ioQueueConfig.EvtIoWrite = ThIoWrite;
	ioQueueConfig.EvtIoDeviceControl = ThIoDeviceControl;

	WDF_OBJECT_ATTRIBUTES_INIT(&attributes);

	status = WdfIoQueueCreate(device, &ioQueueConfig, &attributes, &queue);
	if (!NT_SUCCESS(status)) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Could not create a queue for our IOCtl messages\r\n");
		if (device_init) {
			WdfDeviceInitFree(device_init);
		}
		return status;
	}

	WdfControlFinishInitializing(device);

	// Register callouts
	wdm_device = WdfDeviceWdmGetDeviceObject(device);

	// TCP stream layer callouts for data inspection
	status = THRegisterCallouts(wdm_device);
	if (!NT_SUCCESS(status)) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Could not set callouts\r\n");
		if (device_init) {
			WdfDeviceInitFree(device_init);
		}
		return status;
	}
	// Outbound transport layer for delaying response, and ip information
	//TODO

	// ALE FlOW_ESTABLISHED layer for tracking app pid and stuff
	//TODO

	return status;
}

NTSTATUS THRegisterCallouts(_In_ DEVICE_OBJECT * wdm_device) {
	NTSTATUS status = STATUS_SUCCESS;
	FWPS_CALLOUT sCallout = { 0 };
	FWPM_CALLOUT mCallout = { 0 };
	FWPM_DISPLAY_DATA displayData = { 0 };
	HANDLE engineHandle;
	FWPM_SESSION session = { 0 };
	FWPM_SUBLAYER monitorSubLayer;
	FWPM_FILTER filter;
	FWPM_FILTER_CONDITION filterConditions[2];

	// Add Callouts and Add Filters
	session.flags = FWPM_SESSION_FLAG_DYNAMIC;

	// Open the engine
	status = FwpmEngineOpen(NULL, RPC_C_AUTHN_WINNT, NULL, &session, &engineHandle);
	if (!NT_SUCCESS(status)) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Could not open engine\r\n");
		return status;
	}

	// begin transaction
	status = FwpmTransactionBegin(engineHandle, 0);
	if (!NT_SUCCESS(status)) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Could not open Transaction\r\n");
		FwpmEngineClose(engineHandle);
		return status;
	}

	// Register a new Callout with the Filter Engine using the provided callout functions
	sCallout.calloutKey = TRUSTHUB_STREAM_CALLOUT_V4;
	sCallout.classifyFn = trusthubCalloutClassify;
	sCallout.notifyFn = trusthubCalloutNotify;
	sCallout.flowDeleteFn = trusthubCalloutFlowDelete;
	sCallout.flags |= FWP_CALLOUT_FLAG_ALLOW_MID_STREAM_INSPECTION;
	status = FwpsCalloutRegister((void *)wdm_device, &sCallout, &TrustHub_callout_id);
	if (!NT_SUCCESS(status)) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Could not register callouts\r\n");
		if (TrustHub_callout_id) {
			FwpsCalloutUnregisterById(TrustHub_callout_id);
			TrustHub_callout_id = 0;
		}
		return status;
	}
	DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Registed Callouts\r\n");

	// Add Callouts
	displayData.name = L"Trusthub Callouts";
	displayData.description = L"Intercepts stream data, looking for TLS, then reviews certificates";
	mCallout.displayData = displayData;
	mCallout.calloutKey = TRUSTHUB_STREAM_CALLOUT_V4;
	mCallout.applicableLayer = FWPM_LAYER_STREAM_V4;
	status = FwpmCalloutAdd(engineHandle, &mCallout, NULL, NULL);
	if (!NT_SUCCESS(status)) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Could not add callouts\r\n\r\n");
		if (engineHandle) {
			FwpmEngineClose(engineHandle);
			engineHandle = NULL;
		}
		return status;
	}
	DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Added Callouts\r\n");

	// Add Sublayer
	RtlZeroMemory(&monitorSubLayer, sizeof(FWPM_SUBLAYER));
	monitorSubLayer.subLayerKey = TRUSTHUB_SUBLAYER;
	monitorSubLayer.displayData.name = L"TrustHub Sub layer";
	monitorSubLayer.displayData.description = L"TrustHub Sample Sub layer";
	monitorSubLayer.flags = 0;
	monitorSubLayer.weight = 0;

	status = FwpmSubLayerAdd(engineHandle, &monitorSubLayer, NULL);
	if (!NT_SUCCESS(status)) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Could not create sublayer\r\n");
		if (!NT_SUCCESS(FwpmTransactionAbort(engineHandle))) {
			DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Could not abort Transaction?\r\n");
		}
		FwpmEngineClose(engineHandle);
		return status;
	}

	// Add Filters
	RtlZeroMemory(&filter, sizeof(FWPM_FILTER));

	filter.layerKey = FWPM_LAYER_STREAM_V4;
	filter.action.type = FWP_ACTION_CALLOUT_UNKNOWN;
	filter.action.calloutKey = TRUSTHUB_STREAM_CALLOUT_V4;
	filter.subLayerKey = monitorSubLayer.subLayerKey;
	filter.weight.type = FWP_EMPTY; // auto-weight.

	filter.numFilterConditions = 0; //if we were to add filters to the filterConditions array, we would want to include the amount here

	RtlZeroMemory(filterConditions, sizeof(filterConditions));

	filter.filterCondition = filterConditions;

	filter.displayData.name = L"Stream Layer Filter";
	filter.displayData.description = L"Monitors TCP traffic.";

	status = FwpmFilterAdd(engineHandle, &filter, NULL, NULL);
	if (!NT_SUCCESS(status)) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Could not create sublayer\r\n");
		if (!NT_SUCCESS(FwpmTransactionAbort(engineHandle))) {
			DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Could not abort Transaction?\r\n");
		}
		FwpmEngineClose(engineHandle);
		return status;
	}

	// Commit the transaction
	status = FwpmTransactionCommit(engineHandle);

	// Closing the engine removes the filter, turns out
	/*FwpmEngineClose(engineHandle);
	engineHandle = NULL;
	if (!NT_SUCCESS(status)) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Could not add callouts or filters\r\n");
		if (TrustHub_callout_id) {
			FwpsCalloutUnregisterById(TrustHub_callout_id);
			TrustHub_callout_id = 0;
		}
	}*/
	return status;
}

NTSTATUS unregister_trusthub_callouts() {
	return FwpsCalloutUnregisterById(TrustHub_callout_id);
}

/*++
Routine Description:
DriverEntry initializes the driver and is the first routine called by the
system after the driver is loaded. DriverEntry specifies the other entry
points in the function driver, such as EvtDevice and DriverUnload.

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
NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT  DriverObject, _In_ PUNICODE_STRING RegistryPath) {

    NTSTATUS status;

	DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "DriverEntry In\r\n");

    // Register a cleanup callback so that we can call WPP_CLEANUP when
    // the framework driver object is deleted during driver unload.

	//create the driver and device object
	status = THInitDriverAndDevice(DriverObject, RegistryPath);
    if (!NT_SUCCESS(status)) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Could not init Driver and Device\r\n");
        return status;
    }

	DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "DriverEntry Out\r\n");

    return status;
}

// Clean up the Driver
void DriverUnload(_In_ PDRIVER_OBJECT driver_obj) {
	NTSTATUS status = STATUS_SUCCESS;
	UNICODE_STRING symlink = { 0 };
	UNREFERENCED_PARAMETER(driver_obj);

	status = unregister_trusthub_callouts();
	if (!NT_SUCCESS(status)) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Failed to Unregister Callout\r\n");
	}

	RtlInitUnicodeString(&symlink, TRUSTHUB_SYMNAME);
	IoDeleteSymbolicLink(&symlink);

	DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "--- TrustHubWin driver unloaded ---\r\n");
	return;
}

void thdriver_evt_unload(_In_ WDFDRIVER Driver) {
	UNREFERENCED_PARAMETER(Driver);
	DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "--- TrustHubWin unload event ---\r\n");
	// Delete the framework device object
	//TODO
	//WdfObjectDelete(wdfDevice);
	return;
}
