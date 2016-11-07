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
#pragma alloc_text (PAGE, TrustHubWinEvtDriverContextCleanup)
#endif

// Initializes required WDFDriver and WDFDevice objects
NTSTATUS THInitDriverAndDevice(_In_ DRIVER_OBJECT * driver_obj, _In_ UNICODE_STRING * registry_path);
NTSTATUS THRegisterCallouts(_In_ DEVICE_OBJECT * wdm_device);
NTSTATUS THAddCallouts();

// Initializes required WDFDriver and WDFDevice objects
NTSTATUS THInitDriverAndDevice(_In_ DRIVER_OBJECT * driver_obj, _In_ UNICODE_STRING * registry_path) {
	NTSTATUS status = STATUS_SUCCESS;

	WDF_DRIVER_CONFIG config = { 0 };
	WDFDRIVER driver;
	WDFDEVICE device;
	PWDFDEVICE_INIT device_init = NULL;
	DEVICE_OBJECT * wdm_device = NULL;

	WDF_DRIVER_CONFIG_INIT(&config, WDF_NO_EVENT_CALLBACK); //WDF_NO_EVENT_CALLBACK = NULL, this means there is no callback when WdfDriverCreate is called.
	config.DriverInitFlags |= WdfDriverInitNonPnpDriver; //WdfDriverInitNonPnpDriver means this driver does not support plug and play. "If this value is set, the driver must not supply an EvtDriverDeviceAdd callback function" in other words use WDF_NO_EVENT_CALLBACK
	config.EvtDriverUnload = TrustHubWinEvtDriverContextCleanup;

	// Create a WDFDRIVER for this driver
	status = WdfDriverCreate(driver_obj, registry_path, WDF_NO_OBJECT_ATTRIBUTES, &config, &driver); //WDF_NO_OBJECT_ATTRIBUTES means "extra, nonpageable, memory space"
	if (!NT_SUCCESS(status)) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Could not create WDF Driver\n");
		return status;
	}

	DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Created WDF Driver\n");

	// Allocate a device
	device_init = WdfControlDeviceInitAllocate(driver, &SDDL_DEVOBJ_KERNEL_ONLY);
	
	DECLARE_CONST_UNICODE_STRING(ntDeviceName, TRUSTHUB_DEVICENAME);
	DECLARE_CONST_UNICODE_STRING(symbolicName, TRUSTHUB_SYMNAME);

	//Configure the WDFDEVICE_INIT with a name to allow for access from user mode
	//this section must be done before the WdfDeviceCreate is called.
	WdfDeviceInitSetDeviceType(device_init, FILE_DEVICE_NETWORK);
	WdfDeviceInitSetCharacteristics(device_init, FILE_DEVICE_SECURE_OPEN, FALSE);//"Most drivers specify only the FILE_DEVICE_SECURE_OPEN characteristic. This ensures that the same security settings are applied to any open request into the device's namespace"

	status = WdfDeviceInitAssignName(device_init, &ntDeviceName);
	if (!NT_SUCCESS(status)) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Could not assign driver name\n");
		if (device_init) {
			WdfDeviceInitFree(device_init);
		}
		return status;
	}

	// Create a WDFDEVICE for this driver
	status = WdfDeviceCreate(&device_init, WDF_NO_OBJECT_ATTRIBUTES, &device);
	if (!NT_SUCCESS(status)) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Could not create a device for the driver\n");
		if (device_init) {
			WdfDeviceInitFree(device_init);
		}
		return status;
	}

	status = WdfDeviceCreateSymbolicLink(device, &symbolicName);
	if (!NT_SUCCESS(status)) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Could not create a device for the driver\n");
		if (device_init) {
			WdfDeviceInitFree(device_init);
		}
		return status;
	}

	// Register callouts
	wdm_device = WdfDeviceWdmGetDeviceObject(device);
	status = THRegisterCallouts(wdm_device);
	if (!NT_SUCCESS(status)) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Could not set callouts\n");
		if (device_init) {
			WdfDeviceInitFree(device_init);
		}
		return status;
	}

	return status;
}

NTSTATUS THRegisterCallouts(_In_ DEVICE_OBJECT * wdm_device) {
	NTSTATUS status = STATUS_SUCCESS;
	FWPS_CALLOUT sCallout = { 0 };

	// Register a new Callout with the Filter Engine using the provided callout functions
	sCallout.calloutKey = TRUSTHUB_STREAM_CALLOUT_V4;
	sCallout.classifyFn = trusthubCalloutClassify;
	sCallout.notifyFn = trusthubCalloutNotify;
	sCallout.flowDeleteFn = trusthubCalloutFlowDelete;
	status = FwpsCalloutRegister((void *)wdm_device, &sCallout, &TrustHub_callout_id);
	if (!NT_SUCCESS(status)) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Could not register callouts\n");
		if (TrustHub_callout_id) {
			FwpsCalloutUnregisterById(TrustHub_callout_id);
			TrustHub_callout_id = 0;
		}
		return status;
	}

	// Add Callouts and Add Filters
	status = THAddCallouts();
	if (!NT_SUCCESS(status)) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Could not add callouts or filters\n");
		if (TrustHub_callout_id) {
			FwpsCalloutUnregisterById(TrustHub_callout_id);
			TrustHub_callout_id = 0;
		}
	}
	return status;
}

NTSTATUS THAddCallouts() {
	NTSTATUS status = STATUS_SUCCESS;
	FWPM_CALLOUT mCallout = { 0 };
	FWPM_DISPLAY_DATA displayData = { 0 };
	HANDLE engineHandle;
	FWPM_SESSION session = { 0 };
	FWPM_SUBLAYER monitorSubLayer;
	FWPM_FILTER filter;
	FWPM_FILTER_CONDITION filterConditions[2];
	
	session.flags = FWPM_SESSION_FLAG_DYNAMIC;

	// Open the engine
	status = FwpmEngineOpen(NULL, RPC_C_AUTHN_WINNT, NULL, &session, &engineHandle);
	if (!NT_SUCCESS(status)) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Could not open engine\n");
		return status;
	}

	// begin transaction
	status = FwpmTransactionBegin(engineHandle, 0);
	if (!NT_SUCCESS(status)) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Could not open Transaction\n");
		FwpmEngineClose(engineHandle);
		return status;
	}

	// Add Callouts
	displayData.name = L"Trusthub Callouts";
	displayData.description = L"Intercepts stream data, looking for TLS, then reviews certificates";
	mCallout.displayData = displayData;
	mCallout.calloutKey = TRUSTHUB_STREAM_CALLOUT_V4;
	mCallout.applicableLayer = FWPM_LAYER_STREAM_V4;
	status = FwpmCalloutAdd(engineHandle, &mCallout, NULL, NULL);
	if (!NT_SUCCESS(status)) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Could not add callouts\n");
		if (engineHandle) {
			FwpmEngineClose(engineHandle);
			engineHandle = NULL;
		}
		return status;
	}

	// Add Sublayer
	RtlZeroMemory(&monitorSubLayer, sizeof(FWPM_SUBLAYER));
	monitorSubLayer.subLayerKey = TRUSTHUB_SUBLAYER;
	monitorSubLayer.displayData.name = L"TrustHub Sub layer";
	monitorSubLayer.displayData.description = L"TrustHub Sample Sub layer";
	monitorSubLayer.flags = 0;
	monitorSubLayer.weight = 0;

	status = FwpmSubLayerAdd(engineHandle, &monitorSubLayer, NULL);
	if (!NT_SUCCESS(status)) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Could not create sublayer\n");
		if (!NT_SUCCESS(FwpmTransactionAbort(engineHandle))) {
			DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Could not abort Transaction?\n");
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
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Could not create sublayer\n");
		if (!NT_SUCCESS(FwpmTransactionAbort(engineHandle))) {
			DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Could not abort Transaction?\n");
		}
		FwpmEngineClose(engineHandle);
		return status;
	}

	// Close the transaction
	status = FwpmTransactionCommit(engineHandle);

	// Close the engine
	FwpmEngineClose(engineHandle);
	engineHandle = NULL;

	return status;
}

NTSTATUS unregister_trusthub_callout() {
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

	DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "DriverEntry In\n");

    // Register a cleanup callback so that we can call WPP_CLEANUP when
    // the framework driver object is deleted during driver unload.

	//create the driver and device object
	status = THInitDriverAndDevice(DriverObject, RegistryPath);
    if (!NT_SUCCESS(status)) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Could not init Driver and Device\n");
        return status;
    }

	DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "DriverEntry Out\n");

    return status;
}

// Clean up the Driver
void TrustHubWinEvtDriverContextCleanup(_In_ WDFOBJECT DriverObject) {
    UNREFERENCED_PARAMETER(DriverObject);
}

VOID DriverUnload(_In_ PDRIVER_OBJECT driver_obj)
{
	NTSTATUS status = STATUS_SUCCESS;
	UNICODE_STRING symlink = { 0 };
	UNREFERENCED_PARAMETER(driver_obj);

	status = unregister_trusthub_callout();
	if (!NT_SUCCESS(status)) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Failed to Unregister Callout");
	}

	RtlInitUnicodeString(&symlink, TRUSTHUB_SYMNAME);
	IoDeleteSymbolicLink(&symlink);

	DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "--- TrustHubInterceptor driver unloaded ---");
	return;
}

VOID empty_evt_unload(_In_ WDFDRIVER Driver)
{
	UNREFERENCED_PARAMETER(Driver);
	return;
}
