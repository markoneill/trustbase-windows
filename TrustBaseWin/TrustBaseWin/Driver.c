/*++

Module Name:

    driver.c

Abstract:

    This file contains the driver entry points.

Environment:

    Kernel-mode Driver Framework a

--*/

#include "driver.h"
#include "TrustBaseCommunication.h"

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

//NTSTATUS MyDriverDispatch(
//	PDEVICE_OBJECT DeviceObject,
//	PIRP Irp
//);

CHAR irpNames[][40] = {
	"IRP_MJ_CREATE",
	"IRP_MJ_CREATE_NAMED_PIPE",
	"IRP_MJ_CLOSE",
	"IRP_MJ_READ",
	"IRP_MJ_WRITE",
	"IRP_MJ_QUERY_INFORMATION",
	"IRP_MJ_SET_INFORMATION",
	"IRP_MJ_QUERY_EA",
	"IRP_MJ_SET_EA",
	"IRP_MJ_FLUSH_BUFFERS",
	"IRP_MJ_QUERY_VOLUME_INFORMATION",
	"IRP_MJ_SET_VOLUME_INFORMATION",
	"IRP_MJ_DIRECTORY_CONTROL",
	"IRP_MJ_FILE_SYSTEM_CONTROL",
	"IRP_MJ_DEVICE_CONTROL",			//irpName[14]?
	"IRP_MJ_INTERNAL_DEVICE_CONTROL",
	"IRP_MJ_SHUTDOWN",
	"IRP_MJ_LOCK_CONTROL",
	"IRP_MJ_CLEANUP",
	"IRP_MJ_CREATE_MAILSLOT",
	"IRP_MJ_QUERY_SECURITY",
	"IRP_MJ_SET_SECURITY",
	"IRP_MJ_POWER",
	"IRP_MJ_SYSTEM_CONTROL",
	"IRP_MJ_DEVICE_CHANGE",
	"IRP_MJ_QUERY_QUOTA",
	"IRP_MJ_SET_QUOTA",
	"IRP_MJ_PNP",
	"IRP_MJ_PNP_POWER",
};

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

	//device_init = WdfControlDeviceInitAllocate(driver, &SDDL_DEVOBJ_SYS_ALL_ADM_RWX_WORLD_RW_RES_R); // SDDL_DEVOBJ_ALL_ADM_ALL allows kernel, system, and admin
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
	/*
	status = TbInitWorkItems(device);
	if (!NT_SUCCESS(status)) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Could not create the work items\r\n");
		if (device_init) {
			WdfDeviceInitFree(device_init);
		}
		return status;
	}
	*/

	WdfControlFinishInitializing(device);

	wdm_device = WdfDeviceWdmGetDeviceObject(device);
	myDevice = wdm_device; //Save a global to the device for use in the stream callout (used when we have outstanding irps due to an empty message queue)
	g_wdm_device = device; //Save the a pointer to the device for cleanup

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
	int counter = 0;

	//Call remove context on all of the active flows
	int i;
	int j;
	for (i=0; i < MAX_C; i++) {
		for (j=0; j < MAX_DEP; j++) {
			//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "i = %d, j = %d\r\n", i, j);
			//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "this is what is there %d\r\n", contextArray[i][j]);
			UINT64 val = contextArray[i][j];
			if (val != 0) {
				FwpsFlowRemoveContext(val, FWPS_LAYER_STREAM_V4, TrustBase_callout_stream_id);
				//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Tried to remove %d\r\n", val);
			}
		}	
	}

	//Unregister the Stream callout
	do {
		status2 = FwpsCalloutUnregisterById(TrustBase_callout_stream_id);
		counter++;
	} while (status2 == STATUS_DEVICE_BUSY); //&& counter != 50000);

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
points in the function driver, such as Evt
and tbdriver_evt_unload.

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
	//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Registry Path %ws\r\n", RegistryPath);
	cleanup = 0;

	//Initialize an array to keep track of flow contexts so we may clean up properly afterward
	int i;
	int j;
	for (i=0; i < MAX_C; i++) {
		for (j=0; j < MAX_DEP; j++) {
			contextArray[i][j] = 0;
		}
	}

	//create the driver and device object
	status = TBInitDriverAndDevice(DriverObject, RegistryPath);
    if (!NT_SUCCESS(status)) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Could not init Driver and Device\r\n");
        return status;
    }

	DriverObject->MajorFunction[0x0e] = MyDriverDispatch; //Overwrite the Driver's IRP_MJ_DEVICE_CONTROL pointer in the major function table with a pointer to our function

	DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "DriverEntry Out\r\n");
    return status;
}



NTSTATUS MyDriverDispatch(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	UNREFERENCED_PARAMETER(DeviceObject);
	NTSTATUS status = STATUS_SUCCESS;
	PIO_STACK_LOCATION pios = 0;
	TBMessage* message;
	size_t len = MAX_PATH; 
	pios = IoGetCurrentIrpStackLocation(Irp);
	//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "In My Driver Dispatch\r\n");

	switch (pios->MajorFunction) {
	case IRP_MJ_DEVICE_CONTROL:
		switch (pios->Parameters.DeviceIoControl.IoControlCode) {
		case IOCTL_SEND_CERTIFICATE: {
			//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Got an IOCTL_SEND_CERTIFICATE -- Output buffer length == %d\r\n", pios->Parameters.DeviceIoControl.OutputBufferLength);

			//If the Message queue has a certificate, copy it to the system buffer to send it off
			status = TbGetMessage(&TBOutputQueue, &message);
			if (!NT_SUCCESS(status) || message == NULL) {
				//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Could not retrieve message on read request -- read queue probably empty\r\n");
				outstanding_irp = Irp;
				return STATUS_PENDING;
				//break;
			}

			if (pios->Parameters.DeviceIoControl.OutputBufferLength<sizeof(message)) {
				DbgPrint("System buffer size too small\r\n");
				status = STATUS_BUFFER_OVERFLOW;
				break;
			}
			else {
				// copy what we can into the buffer  -- careful with any off by 1 errors
				status = TbCopyMessage(Irp->AssociatedIrp.SystemBuffer, pios->Parameters.DeviceIoControl.OutputBufferLength, message, &len);
				if (!NT_SUCCESS(status)) {
					DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Could not copy message to output buffer -- is this the problem?\r\n");
					break;
				}
				Irp->IoStatus.Information = len;
				status = STATUS_SUCCESS;
			}
			// remove the message if we have read it all out
			if (NT_SUCCESS(TbFinishedMessage(message))) {
				TbRemMessage(&TBOutputQueue);
			}
			break;
		}
		case IOCTL_GET_DECISION:
		{
			//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Got an IOCTL_GET_DECISION");
			UINT8* cursor;
			UINT64 flowhandle;
			TBResponseType response;
			UINT8* buffer[32];											

			//Get the answer out of the buffer and store it in the GenericAVL table
			//DbgPrint("GOT AN IOCTL_GET_DECISION %s\n", Irp->AssociatedIrp.SystemBuffer);

			RtlCopyMemory((void*)buffer, Irp->AssociatedIrp.SystemBuffer, pios->Parameters.DeviceIoControl.InputBufferLength);
			Irp->IoStatus.Information = sizeof(Irp->AssociatedIrp.SystemBuffer);

			cursor = (UINT8*)buffer;
			flowhandle = ((UINT64*)cursor)[0];
			cursor += sizeof(UINT64);
			response = (TBResponseType)(((UINT8*)cursor)[0]);
			//DbgPrint("Response == %d\r\n", response);
			status = TbHandleResponse(&TBResponses, flowhandle, response);
			break;
		}
		case IOCTL_SHUTDOWN: {
				if (outstanding_irp != NULL){
					IoCompleteRequest(outstanding_irp, 0);
					outstanding_irp = NULL;
				}
				break;
		}

		default:
			status = STATUS_NOT_SUPPORTED;
			DbgPrint("Not sure what I am doing here\r\n");
			break;
		} // end of IoControlCode switch statement
		break;
	default:
		DbgPrint("Not sure what I am doing here either\r\n");
		status = STATUS_NOT_SUPPORTED;
	} // end of MajorFunction switch statement

	//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Completing irp %d\r\n", Irp->Type);
	Irp->IoStatus.Status = status;
	IoCompleteRequest(Irp, 0);
	return status;
}

//Not needed without windows I/O queues
VOID TBCleanUp() {
	WdfIoQueuePurgeSynchronously(TBReadQueue);
	WdfIoQueuePurgeSynchronously(TBWriteQueue);
	return;
}

// Clean up the Driver -- Similiar to DriverUnload(IN PDRIVER_OBJECT driver_obj)
void tbdriver_evt_unload(IN WDFDRIVER Driver) {
	UNREFERENCED_PARAMETER(Driver);
	UNICODE_STRING symlink = { 0 };
	NTSTATUS status;
	DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "--- TrustBaseWin unload event ---\r\n");
	cleanup = 1;

	if (outstanding_irp) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Finishing outstanding Irp\r\n");
		IoCompleteRequest(outstanding_irp, 0);
		outstanding_irp = NULL;
	}
	
	//TBCleanUp();
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

	//Handshake handler has some intel specific code that needs to be changed to work cross platform
}
