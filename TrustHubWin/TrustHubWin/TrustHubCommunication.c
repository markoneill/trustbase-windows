#include "TrustHubCommunication.h"

NTSTATUS ThInitQueues(IN WDFDEVICE device) {
	WDF_IO_QUEUE_CONFIG readIoQueueConfig;
	WDF_IO_QUEUE_CONFIG writeIoQueueConfig;
	NTSTATUS status = STATUS_SUCCESS;


	// configure the queues with callbacks
	WDF_IO_QUEUE_CONFIG_INIT(&readIoQueueConfig, WdfIoQueueDispatchSequential);
	readIoQueueConfig.EvtIoRead = ThIoRead; // Our callback for writing out to userspace

	WDF_IO_QUEUE_CONFIG_INIT(&writeIoQueueConfig, WdfIoQueueDispatchSequential);
	writeIoQueueConfig.EvtIoWrite = ThIoWrite; // Our callback for getting a response from userspace

											   // create the queues
	status = WdfIoQueueCreate(device, &readIoQueueConfig, WDF_NO_OBJECT_ATTRIBUTES, &THReadQueue);
	if (!NT_SUCCESS(status)) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Could not create a read queue\r\n");
		return status;
	}
	status = WdfIoQueueCreate(device, &writeIoQueueConfig, WDF_NO_OBJECT_ATTRIBUTES, &THWriteQueue);
	if (!NT_SUCCESS(status)) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Could not create a write queue\r\n");
		return status;
	}

	// Stop the queues, so we only get the callback if we are ready
	WdfIoQueueStop(THReadQueue, NULL, NULL);
	WdfIoQueueStop(THWriteQueue, NULL, NULL);

	// Configure the dispatching
	status = WdfDeviceConfigureRequestDispatching(device, THReadQueue, WdfRequestTypeRead);
	if (!NT_SUCCESS(status)) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Could configure the device to use our read queue\r\n");
		return status;
	}
	status = WdfDeviceConfigureRequestDispatching(device, THWriteQueue, WdfRequestTypeWrite);
	if (!NT_SUCCESS(status)) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Could configure the device to use our write queue\r\n");
		return status;
	}

	// init message queues

	return status;
}

NTSTATUS ThInitWorkItems(IN WDFDEVICE device) {
	WDF_OBJECT_ATTRIBUTES  workitemAttributes;
	WDF_WORKITEM_CONFIG  workitemConfig;
	NTSTATUS status = STATUS_SUCCESS;

	WDF_OBJECT_ATTRIBUTES_INIT(&workitemAttributes);
	// if we want to pass parameters, we do a WDF_OBJECT_ATTRIBUTES_SET_CONTEXT_TYPE with a custom context
	WDF_WORKITEM_CONFIG_INIT(&workitemConfig, THReadyRead);
	// we have to set a parent
	workitemAttributes.ParentObject = device;
	status = WdfWorkItemCreate(&workitemConfig, &workitemAttributes, &THReadyReadItem);
	if (!NT_SUCCESS(status)) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Could not create a work item\r\n");
		if (status == STATUS_INVALID_PARAMETER) {
			DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "due to an invalid parameter\r\n");
		} else if (status == STATUS_INVALID_DEVICE_REQUEST) {
			DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "due to an invalid parent object\r\n");
		} else if (status == STATUS_INSUFFICIENT_RESOURCES) {
			DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "due to an insufficent resources\r\n");
		} else if (status == STATUS_WDF_INCOMPATIBLE_EXECUTION_LEVEL) {
			DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "due to an insufficent resources\r\n");
		} else if (status == STATUS_WDF_PARENT_NOT_SPECIFIED) {
			DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "due to no parent\r\n");
		} else if (status == STATUS_WDF_EXECUTION_LEVEL_INVALID) {
			DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "due to an invalid level\r\n");
		} else {
			DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "for some reason, with status %d\r\n", status);
		}
		return status;
	}


	return status;
}

// Called when the framework receives IRP_MJ_READ requests
VOID ThIoRead(IN WDFQUEUE Queue, IN WDFREQUEST Request, IN size_t Length) {
	UNREFERENCED_PARAMETER(Queue);
	UNREFERENCED_PARAMETER(Request);
	UNREFERENCED_PARAMETER(Length);
	PAGED_CODE();
	size_t bufsize;
	NTSTATUS status = STATUS_SUCCESS;
	void* buffer;
	size_t len;

	len = 16;

	DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "File Read Called\r\n");

	status = WdfRequestRetrieveOutputBuffer(Request, len, &buffer, &bufsize);
	if (!NT_SUCCESS(status)) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Could not retrieve output buffer on read request\r\n");
	}

	DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "a buff: %x\r\n", *((PUINT64)buffer));

	RtlCopyMemory(buffer, "Run The Jewels\n", len);
	DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "b buff: %x\r\n", *((PUINT64)buffer));

	// Decrement the number to be read, and stop if we have responded to all current to be reads
	WdfIoQueueStop(THReadQueue, NULL, NULL);
	WdfRequestCompleteWithInformation(Request, status, len);
}

// Called when the framework receives IRP_MJ_WRITE
VOID ThIoWrite(IN WDFQUEUE Queue, IN WDFREQUEST Request, IN size_t Length) {
	UNREFERENCED_PARAMETER(Queue);
	UNREFERENCED_PARAMETER(Request);
	UNREFERENCED_PARAMETER(Length);
	PAGED_CODE();
	NTSTATUS status = STATUS_SUCCESS;

	DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "File Write Called\r\n");
	WdfRequestComplete(Request, status);
}

VOID THReadyRead(IN WDFWORKITEM WorkItem) {
	UNREFERENCED_PARAMETER(WorkItem);
	DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "WorkItem Called\r\n");
	WdfIoQueueStart(THReadQueue);
	// Increment the number we have ready to be read

	// We reuse this workitem, so we don't have to delete it
}
