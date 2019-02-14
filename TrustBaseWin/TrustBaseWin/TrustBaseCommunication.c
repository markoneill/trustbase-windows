#include "TrustBaseCommunication.h"

NTSTATUS TbInitQueues(IN WDFDEVICE device) {
	UNREFERENCED_PARAMETER(device);
	NTSTATUS status = STATUS_SUCCESS;
/*
	WDF_IO_QUEUE_CONFIG readIoQueueConfig;
	WDF_IO_QUEUE_CONFIG writeIoQueueConfig;


	// configure the queues with callbacks
	WDF_IO_QUEUE_CONFIG_INIT(&readIoQueueConfig, WdfIoQueueDispatchSequential);   //Potentially Faster?
	//WDF_IO_QUEUE_CONFIG_INIT(&readIoQueueConfig, WdfIoQueueDispatchParallel);
	readIoQueueConfig.EvtIoRead = TbIoRead; // Our callback for writing out to userspace

	WDF_IO_QUEUE_CONFIG_INIT(&writeIoQueueConfig, WdfIoQueueDispatchSequential);
	//WDF_IO_QUEUE_CONFIG_INIT(&writeIoQueueConfig, WdfIoQueueDispatchParallel);
	writeIoQueueConfig.EvtIoWrite = TbIoWrite; // Our callback for getting a response from userspace

	// create the queues
	status = WdfIoQueueCreate(device, &writeIoQueueConfig, WDF_NO_OBJECT_ATTRIBUTES, &TBWriteQueue);
	if (!NT_SUCCESS(status)) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Could not create a write queue\r\n");
		return status;
	}
	status = WdfIoQueueCreate(device, &readIoQueueConfig, WDF_NO_OBJECT_ATTRIBUTES, &TBReadQueue);
	if (!NT_SUCCESS(status)) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Could not create a read queue\r\n");
		return status;
	}

	// Stop the read queue, so we only get the callback if we are ready
	WdfIoQueueStop(TBReadQueue, NULL, NULL);
	WdfIoQueueStop(TBWriteQueue, NULL, NULL);


	// Configure the dispatching
	status = WdfDeviceConfigureRequestDispatching(device, TBReadQueue, WdfRequestTypeRead);  //Tell this queue to recieve IRP_READ
	if (!NT_SUCCESS(status)) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Couldn't configure the device to use our read queue\r\n");
		return status;
	}
	status = WdfDeviceConfigureRequestDispatching(device, TBWriteQueue, WdfRequestTypeWrite); //Tell this queue to recieve IRP_WRITE
	if (!NT_SUCCESS(status)) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Couldn't configure the device to use our write queue\r\n");
		return status;
	}
*/
	// initialize message queues
	status = TbInitMessageQueue(&TBOutputQueue);
	if (!NT_SUCCESS(status)) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Could not init the output message queue\r\n");
		return status;
	}
	status = TbInitResponseTable(&TBResponses);

	return status;
}
 
/*
NTSTATUS TbInitWorkItems(IN WDFDEVICE device) {
	WDF_OBJECT_ATTRIBUTES  workitemAttributes;
	WDF_WORKITEM_CONFIG  workitemConfig;
	NTSTATUS status = STATUS_SUCCESS;

	WDF_OBJECT_ATTRIBUTES_INIT(&workitemAttributes);
	// if we want to pass parameters, we do a WDF_OBJECT_ATTRIBUTES_SET_CONTEXT_TYPE with a custom context
	WDF_WORKITEM_CONFIG_INIT(&workitemConfig, TBReadyRead);
	// we have to set a parent
	workitemAttributes.ParentObject = device;
	status = WdfWorkItemCreate(&workitemConfig, &workitemAttributes, &TBReadyReadItem);
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
*/

/*
// Called when the framework receives IRP_MJ_READ requests
VOID TbIoRead(IN WDFQUEUE Queue, IN WDFREQUEST Request, IN size_t Length) {
	UNREFERENCED_PARAMETER(Queue);
	UNREFERENCED_PARAMETER(Length);
	PAGED_CODE();

	//UNREFERENCED_PARAMETER(Request);

	
	size_t bufsize;
	NTSTATUS status = STATUS_SUCCESS;
	TBMessage* message;
	void* buffer;
	size_t len = MAX_PATH; // they should at least fit the biggest process path, but should actually be much bigger
	DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Entered IoRead Time=%llu\r\n", getTime());
	
	//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Print read Queue\r\n");
	//queueStats(TBReadQueue);


	// get the message
	status = TbGetMessage(&TBOutputQueue, &message);
	if (!NT_SUCCESS(status) || message == NULL) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Could not retrieve message on read request\r\n");
		WdfRequestCompleteWithInformation(Request, STATUS_UNSUCCESSFUL, 0);
		return;
	}

	// get the buffer
	status = WdfRequestRetrieveOutputBuffer(Request, len, &buffer, &bufsize);
	if (!NT_SUCCESS(status)) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Could not retrieve output buffer on read request\r\n");
		WdfRequestCompleteWithInformation(Request, STATUS_CANCELLED, 0); // set len so they know the size we want?
		// the buffer wasn't big enough probably
		return;
	}

	// copy what we can into the buffer
	status = TbCopyMessage(buffer, bufsize, message, &len);
	if (!NT_SUCCESS(status)) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Could not copy message to output buffer\r\n");
		WdfRequestCompleteWithInformation(Request, STATUS_UNSUCCESSFUL, 0);
		return;
	}

	// remove the message if we have read it all out
	if (NT_SUCCESS(TbFinishedMessage(message))) {
		//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Finished with this message\r\n");
		TbRemMessage(&TBOutputQueue);
		// stop if we have responded to all current to be reads
		if (IsListEmpty(&((&TBOutputQueue)->ListHead))) {             //Need protection?
			//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Finished with outward message queue\r\n");
			WdfIoQueueStop(TBReadQueue, NULL, NULL);
		}
	}
	WdfRequestCompleteWithInformation(Request, status, len);
	//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Exited IoRead Time=%llu\r\n", getTime());
	return;
}


// Called when the framework receives IRP_MJ_WRITE
VOID TbIoWrite(IN WDFQUEUE Queue, IN WDFREQUEST Request, IN size_t Length) {

	UNREFERENCED_PARAMETER(Queue);
	UNREFERENCED_PARAMETER(Length);
	PAGED_CODE();

	NTSTATUS status = STATUS_SUCCESS;

	size_t bufsize;
	void* buffer;
	UINT8* cursor;
	UINT64 flowhandle;
	TBResponseType response;
	DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Entered IoWrite Time=%llu\r\n", getTime());
	//queueStats(TBWriteQueue);


	status = WdfRequestRetrieveInputBuffer(Request, TBRESPONSE_MESSAGE_SIZE, &buffer, &bufsize);
	if (!NT_SUCCESS(status)) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Didn't get a whole response\r\n");
		WdfRequestComplete(Request, STATUS_UNSUCCESSFUL);
		return;
	}

	cursor = (UINT8*)buffer;

	flowhandle = ((UINT64*)cursor)[0];
	cursor += sizeof(UINT64);

	response = (TBResponseType)(((UINT8*)cursor)[0]);

	//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Got response %x for handle %d\r\n", response, flowhandle);

	TbHandleResponse(&TBResponses, flowhandle, response);

	WdfRequestComplete(Request, status);
	//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Exited IoWrite Time=%llu\r\n", getTime());
	return;
	
}


VOID TBReadyRead(IN WDFWORKITEM WorkItem) {
	UNREFERENCED_PARAMETER(WorkItem);
	WdfIoQueueStart(TBReadQueue);
	// We reuse this workitem, so we don't have to delete it
}
*/

ULONG64 getTime() {
	LARGE_INTEGER frequency;
	LARGE_INTEGER start;
	start = KeQueryPerformanceCounter(&frequency);
	return (ULONG64)(start.QuadPart) / frequency.QuadPart;
}

VOID queueStats(WDFQUEUE queue) {
	ULONG QueueRequests;
	ULONG DriverRequests;
	WDF_IO_QUEUE_STATE state;
	state = WdfIoQueueGetState(queue, &QueueRequests, &DriverRequests);
	DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Number of requests in queue = %d\r\n", QueueRequests);
	DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Number of requests at the driver = %d\r\n", DriverRequests);
	if (WDF_IO_QUEUE_IDLE(state)) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Queue is idle\r\n");
	}
	if (WDF_IO_QUEUE_STOPPED(state)) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Queue is stopped\r\n");
	}
}
