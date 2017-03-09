#include "TrustHubMessage.h"

NTSTATUS ThInitMessageQueue(OUT THMessageQueue* queue) {
	NTSTATUS status = STATUS_SUCCESS;

	KeInitializeSpinLock(&(queue->lock));
	InitializeListHead(&(queue->ListHead));

	return status;
}

NTSTATUS ThAddMessage(IN THMessageQueue* queue, IN THMessageType type, IN UINT64 processID, IN FWP_BYTE_BLOB processPath, IN UINT32 dataSize, IN UINT8* data) {
	NTSTATUS status = STATUS_SUCCESS;
	THMessage* message;
	KIRQL irql;

	// Allocate space
	message = (THMessage*)ExAllocatePoolWithTag(NonPagedPool, sizeof(THMessage), TH_POOL_TAG);
	RtlZeroMemory(message, sizeof(THMessage));

	// Fill it
	message->type = type;
	message->processID = processID;

	message->processPathSize = processPath.size;
	message->processPath = (UINT8*)ExAllocatePoolWithTag(NonPagedPool, sizeof(UINT8) * processPath.size, TH_POOL_TAG);
	RtlCopyMemory(message->processPath, processPath.data, processPath.size);

	message->dataSize = dataSize;
	//message->data = (UINT8*)ExAllocatePoolWithTag(NonPagedPool, sizeof(UINT8) * dataSize, TH_POOL_TAG);
	//RtlCopyMemory(message->data, data, dataSize);
	// our data will be preallocated by the caller
	message->data = data;

	// add it to our thing
	//ExInterlockedInsertTailList(&(queue.ListHead), &(message->ListEntry), &(queue.lock));

	KeAcquireSpinLock(&(queue->lock), &irql);
	InsertTailList(&(queue->ListHead), &(message->ListEntry));
	KeReleaseSpinLock(&(queue->lock), irql);
	return status;
}

NTSTATUS ThSizeNextMessage(IN THMessageQueue* queue, OUT size_t* len) {
	NTSTATUS status = STATUS_SUCCESS;
	THMessage* message;
	LIST_ENTRY* entry;

	if (IsListEmpty(&(queue->ListHead))) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Could not find another message\r\n");
		return STATUS_NOT_FOUND;
	}
	entry = queue->ListHead.Flink;
	message = (THMessage*)CONTAINING_RECORD(entry, THMessage, ListEntry);

	status = ThSizeMessage(message, len);
	return status;
}

NTSTATUS ThSizeMessage(IN THMessage* message, OUT size_t* len) {
	NTSTATUS status = STATUS_SUCCESS;
	*len = sizeof message->type;
	*len += sizeof message->processID;
	*len += sizeof message->processPathSize;
	*len += message->processPathSize;
	*len += sizeof message->dataSize;
	*len += message->dataSize;
	return status;
}

// messages popped must later be freed with ThFreeMessage
NTSTATUS ThPopMessage(IN THMessageQueue* queue, OUT THMessage** message) {
	NTSTATUS status = STATUS_SUCCESS;
	KIRQL irql;
	//PLIST_ENTRY mentry = ExInterlockedRemoveHeadList(&(queue.ListHead), &(queue.lock));
	KeAcquireSpinLock(&(queue->lock), &irql);
	PLIST_ENTRY mentry = RemoveHeadList(&(queue->ListHead));
	KeReleaseSpinLock(&(queue->lock), irql);
	if (mentry == NULL) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Could not pop message\r\n");
		return STATUS_NOT_FOUND;
	}

	*message = (THMessage*)CONTAINING_RECORD(mentry, THMessage, ListEntry);
	return status;
}

NTSTATUS ThFreeMessage(IN THMessage* message) {
	if (message) {
		ExFreePoolWithTag(message->data, TH_POOL_TAG);
		ExFreePoolWithTag(message->processPath, TH_POOL_TAG);
	}
	ExFreePoolWithTag(message, TH_POOL_TAG);
	return STATUS_SUCCESS;
}

NTSTATUS ThCopyMessage(IN UINT8* buffer, size_t bufsize, THMessage* message) {
	NTSTATUS status = STATUS_SUCCESS;
	size_t len;
	UINT8* cursor = buffer;
	status = ThSizeMessage(message, &len);
	if (!NT_SUCCESS(status)) {
		return status;
	}

	if (len > bufsize) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Too small of a buffer to copy message buf : %x, message : %x\r\n", bufsize, len);
		return STATUS_BUFFER_TOO_SMALL;
	}

	DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Copying message:\r\n");
	DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Type:%x\r\n", message->type);
	DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "PID:%x\r\n", message->processID);
	DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "PathSize:%x\r\n", message->processPathSize);
	DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Path:%S\r\n", message->processPath);
	DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "DataSize:%x ...\r\n", message->dataSize);

	*(THMessageType UNALIGNED *)cursor = message->type;
	cursor += sizeof(THMessageType);

	*(UINT64 UNALIGNED *)cursor = message->processID;
	cursor += sizeof(UINT64);

	*(UINT32 UNALIGNED *)cursor = message->processPathSize;
	cursor += sizeof(UINT32);

	RtlCopyMemory(cursor, message->processPath, message->processPathSize);
	cursor += message->processPathSize;

	*(UINT32 UNALIGNED *)cursor = message->dataSize;
	cursor += sizeof(UINT32);

	RtlCopyMemory(cursor, message->data, message->dataSize);
	return status;
}


	