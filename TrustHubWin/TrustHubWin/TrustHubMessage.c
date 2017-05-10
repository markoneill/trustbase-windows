#include "TrustHubMessage.h"

NTSTATUS ThInitMessageQueue(OUT THMessageQueue* queue) {
	NTSTATUS status = STATUS_SUCCESS;

	KeInitializeSpinLock(&(queue->lock));
	InitializeListHead(&(queue->ListHead));

	return status;
}

NTSTATUS ThMakeMessage(OUT THMessage** msg, IN UINT64 flowHandle, IN UINT64 processID, IN FWP_BYTE_BLOB processPath) {

	// Allocate space
	(*msg) = (THMessage*)ExAllocatePoolWithTag(NonPagedPool, sizeof(THMessage), TH_POOL_TAG);
	RtlZeroMemory((*msg), sizeof(THMessage));

	// Fill it
	(*msg)->flowHandle = flowHandle;
	(*msg)->processID = processID;
	(*msg)->bytesWritten = 0;
	(*msg)->processPathSize = processPath.size;
	(*msg)->processPath = (UINT8*)ExAllocatePoolWithTag(NonPagedPool, sizeof(UINT8) * processPath.size, TH_POOL_TAG);
	RtlCopyMemory((*msg)->processPath, processPath.data, processPath.size);

	return STATUS_SUCCESS;
}



NTSTATUS ThAddMessage(IN THMessageQueue* queue, IN THMessage* msg) {
	KIRQL irql;

	// add it to our thing
	KeAcquireSpinLock(&(queue->lock), &irql);
	InsertTailList(&(queue->ListHead), &(msg->ListEntry));
	KeReleaseSpinLock(&(queue->lock), irql);
	return STATUS_SUCCESS;
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

	ThSizeMessage(message, len);
	return status;
}

VOID ThSizeMessage(IN THMessage* message, OUT size_t* len) {
	*len = 0;
	*len += sizeof(UINT64); // total size
	*len += sizeof message->flowHandle;
	*len += sizeof message->processID;
	*len += sizeof message->processPathSize;
	*len += message->processPathSize;
	*len += sizeof message->clientHelloSize;
	*len += message->clientHelloSize;
	*len += sizeof message->serverHelloSize;
	*len += message->serverHelloSize;
	*len += sizeof message->dataSize;
	*len += message->dataSize;
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

// doesn't pop the message, so it must be removed afterward
NTSTATUS ThGetMessage(IN THMessageQueue* queue, OUT THMessage** message) {
	NTSTATUS status = STATUS_SUCCESS;
	LIST_ENTRY* entry;
	KIRQL irql;
	KeAcquireSpinLock(&(queue->lock), &irql);
	if (IsListEmpty(&(queue->ListHead))) {
		KeReleaseSpinLock(&(queue->lock), irql);
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Could not find a message\r\n");
		return STATUS_NOT_FOUND;
	}
	entry = queue->ListHead.Flink;
	*message = (THMessage*)CONTAINING_RECORD(entry, THMessage, ListEntry);
	KeReleaseSpinLock(&(queue->lock), irql);
	return status;
}

// pop and free
NTSTATUS ThRemMessage(IN THMessageQueue* queue) {
	THMessage* message;
	NTSTATUS status = STATUS_SUCCESS;
	KIRQL irql;

	KeAcquireSpinLock(&(queue->lock), &irql);
	PLIST_ENTRY mentry = RemoveHeadList(&(queue->ListHead));
	KeReleaseSpinLock(&(queue->lock), irql);

	if (mentry == NULL) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Could not pop message\r\n");
		return STATUS_NOT_FOUND;
	}

	message = (THMessage*)CONTAINING_RECORD(mentry, THMessage, ListEntry);

	ThFreeMessage(message);
	return status;
}

NTSTATUS ThFreeMessage(IN THMessage* message) {
	if (message) {
		if (message->serverHello) {
			ExFreePoolWithTag(message->serverHello, TH_POOL_TAG);
		}
		if (message->clientHello) {
			ExFreePoolWithTag(message->clientHello, TH_POOL_TAG);
		}
		if (message->data) {
			ExFreePoolWithTag(message->data, TH_POOL_TAG);
		}
		if (message->processPath) {
			ExFreePoolWithTag(message->processPath, TH_POOL_TAG);
		}
		ExFreePoolWithTag(message, TH_POOL_TAG);
	}
	return STATUS_SUCCESS;
}

NTSTATUS ThCopyMessage(IN UINT8* buffer, size_t bufsize, THMessage* message, size_t* bytesWritten) {
	NTSTATUS status = STATUS_SUCCESS;
	size_t len, offset;
	UINT8* cursor;
	UINT8* bufend;
	UINT32 loc_clienthellosize;
	UINT32 loc_clienthello;
	UINT32 loc_serverhellosize;
	UINT32 loc_serverhello;
	UINT32 loc_datasize;
	UINT32 loc_data;

	ThSizeMessage(message, &len);

	DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Copying message:\r\n");
	
	cursor = buffer;
	bufend = buffer + bufsize;

	switch (message->bytesWritten) {
	case THMESSAGE_OFFSET_LEN:
		if (cursor + sizeof(UINT64) > bufend) break;
		*(UINT64 UNALIGNED *)cursor = (UINT64)(len);
		cursor += sizeof(UINT64);
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Len:%x\r\n", len);

	case THMESSAGE_OFFSET_FLOWHANDLE:
		if (cursor + sizeof(UINT64) > bufend) break;
		*(UINT64 UNALIGNED *)cursor = message->flowHandle;
		cursor += sizeof(UINT64);
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "flowHandle:%x\r\n", message->flowHandle);

	case THMESSAGE_OFFSET_PROCESSID:
		if (cursor + sizeof(UINT64) > bufend) break;
		*(UINT64 UNALIGNED *)cursor = message->processID;
		cursor += sizeof(UINT64);
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "PID:%x\r\n", message->processID);

	case THMESSAGE_OFFSET_PROCESSPATHSIZE:
		if (cursor + sizeof(UINT32) > bufend) break;
		*(UINT32 UNALIGNED *)cursor = message->processPathSize;
		cursor += sizeof(UINT32);
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "PathSize:%x\r\n", message->processPathSize);

	case THMESSAGE_OFFSET_PROCESSPATH:
		if (cursor + message->processPathSize > bufend) {
			// here we could copy over a portion, but our min buffer is bigger than MAX_PATH
			break;
		}
		RtlCopyMemory(cursor, message->processPath, message->processPathSize);
		cursor += message->processPathSize;
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Path:%S\r\n", message->processPath);

	default:
		loc_clienthellosize = THMESSAGE_OFFSET_PROCESSPATH + message->processPathSize;
		loc_clienthello = loc_clienthellosize + sizeof(UINT32);
		loc_serverhellosize = loc_clienthello + message->clientHelloSize;
		loc_serverhello = loc_serverhellosize + sizeof(UINT32);
		loc_datasize = loc_serverhello + message->serverHelloSize;
		loc_data = loc_datasize + sizeof(UINT32);

		// write client hello size
		if (message->bytesWritten + (cursor - buffer) == loc_clienthellosize) {
			if (cursor + sizeof(UINT32) > bufend) break;
			*(UINT32 UNALIGNED *)cursor = message->clientHelloSize;
			cursor += sizeof(UINT32);
			DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Client Hello Size:%x\r\n", message->clientHelloSize);
		}

		// write client hello
		if (message->bytesWritten + (cursor - buffer) >= loc_clienthello && message->bytesWritten + (cursor - buffer) < loc_serverhellosize) {
			// byte offset in client hello
			offset = (message->bytesWritten + (cursor - buffer)) - (loc_clienthello);
			// amount left to write
			len = message->clientHelloSize - offset;
			// check if we can write it all
			if (len > (bufsize - (cursor - buffer))) {
				// write what we can
				len = (bufsize - (cursor - buffer));
				RtlCopyMemory(cursor, message->clientHello + offset, len);
				cursor += len;
				break;
			}
			// write it all
			RtlCopyMemory(cursor, message->clientHello + offset, len);
			cursor += len;
		}

		// write server hello size
		if (message->bytesWritten + (cursor - buffer) == loc_serverhellosize) {
			if (cursor + sizeof(UINT32) > bufend) break;
			*(UINT32 UNALIGNED *)cursor = message->serverHelloSize;
			cursor += sizeof(UINT32);
			DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Server Hello Size:%x\r\n", message->serverHelloSize);
		}

		// write server hello
		if (message->bytesWritten + (cursor - buffer) >= loc_serverhello && message->bytesWritten + (cursor - buffer) < loc_datasize) {
			// byte offset in client hello
			offset = (message->bytesWritten + (cursor - buffer)) - (loc_serverhello);
			// amount left to write
			len = message->serverHelloSize - offset;
			// check if we can write it all
			if (len > (bufsize - (cursor - buffer))) {
				// write what we can
				len = (bufsize - (cursor - buffer));
				RtlCopyMemory(cursor, message->serverHello + offset, len);
				cursor += len;
				break;
			}
			// write it all
			RtlCopyMemory(cursor, message->serverHello + offset, len);
			cursor += len;
		}

		// write data size
		if (message->bytesWritten + (cursor - buffer) == loc_datasize) {
			if (cursor + sizeof(UINT32) > bufend) break;
			*(UINT32 UNALIGNED *)cursor = message->dataSize;
			cursor += sizeof(UINT32);
			DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Data Size:%x\r\n", message->dataSize);
		}

		// write data
		if (message->bytesWritten + (cursor - buffer) >= loc_data) {
			// byte offset in client hello
			offset = (message->bytesWritten + (cursor - buffer)) - (loc_data);
			// amount left to write
			len = message->dataSize - offset;
			// check if we can write it all
			if (len > (bufsize - (cursor - buffer))) {
				// write what we can
				len = (bufsize - (cursor - buffer));
				RtlCopyMemory(cursor, message->data + offset, len);
				cursor += len;
				break;
			}
			// write it all
			RtlCopyMemory(cursor, message->data + offset, len);
			cursor += len;
			DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Finished sending Certificate\r\n");
		}



		/*
		// find where we are
		if (message->bytesWritten + (cursor - buffer) == message->processPathSize + THMESSAGE_OFFSET_PROCESSPATH) {
			if (cursor + sizeof(UINT32) > bufend) break;
			*(UINT32 UNALIGNED *)cursor = message->dataSize;
			cursor += sizeof(UINT32);
			DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "DataSize:%x\r\n", message->dataSize);
		}

		// offset in the buffer is the amount we have written - the size of bytes before data
		// (bytes_written_in_prev_reads + bytes_written_this_read) - (offset_to_path + size_of_process_path + size_of_data_size_field)
		offset = (message->bytesWritten + (cursor - buffer)) - (THMESSAGE_OFFSET_PROCESSPATH + message->processPathSize + sizeof(UINT32));
		// len is now the amount we have left to written
		len = message->dataSize - offset;
		if (len > (bufsize - (cursor - buffer))) {
			len = (bufsize - (cursor - buffer));
		}
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "offset inside the certificate: cursor %d, previously written %d, offset %d\r\n", cursor - buffer, message->bytesWritten, offset);
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Data: byte %d to %d\r\n", offset, offset + len);
		RtlCopyMemory(cursor, message->data + offset, len);
		cursor += len;*/
	}
	
	// set the amount written of the entire message
	message->bytesWritten += (cursor - buffer);
	// set the amount written this call
	*bytesWritten = cursor - buffer;
	DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Bytes written this call %d, total %d", cursor - buffer, message->bytesWritten);
	return status;
}

NTSTATUS ThFinishedMessage(IN THMessage* message) {
	NTSTATUS status = STATUS_SUCCESS;
	size_t len;
	ThSizeMessage(message, &len);
	
	if (message->bytesWritten < len) {
		return STATUS_PARTIAL_COPY;
	}
	return status;
}

VOID ThPrintMessage(IN THMessage * message) {
	DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Message:\r\n\tFlow %x\r\n\tID %x\r\n\tPath %S\r\n", message->flowHandle, message->processID, message->processPath);
	if (message->clientHello) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "\tClient Hello:\r\n\t%02x %02x %02x ... %02x %02x\r\n", message->clientHello[0], message->clientHello[1], message->clientHello[2], message->clientHello[message->clientHelloSize-2], message->clientHello[message->clientHelloSize-1]);
	}
	if (message->serverHello) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "\tServer Hello:\r\n\t%02x %02x %02x ... %02x %02x\r\n", message->serverHello[0], message->serverHello[1], message->serverHello[2], message->serverHello[message->serverHelloSize - 2], message->serverHello[message->serverHelloSize - 1]);
	}
	if (message->data) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "\tCert:\r\n\t%02x %02x %02x ... %02x %02x\r\n", message->data[0], message->data[1], message->data[2], message->data[message->dataSize - 2], message->data[message->dataSize - 1]);
	}

	return;
}

NTSTATUS ThInitResponseTable(IN THResponseTable* table) {
	NTSTATUS status = STATUS_SUCCESS;
	RtlInitializeGenericTableAvl((PRTL_AVL_TABLE)table, (PRTL_AVL_COMPARE_ROUTINE)ThResponseCompare, (PRTL_AVL_ALLOCATE_ROUTINE)ThResponseAllocate, (PRTL_AVL_FREE_ROUTINE)ThResponseFree, NULL);
	return status;
}

NTSTATUS ThAddResponse(IN THResponseTable* table, IN UINT64 flowHandle, IN UINT16 layerId, IN UINT32 streamFlags) {
	NTSTATUS status = STATUS_SUCCESS;
	THResponse response;
	boolean success;

	response.flowHandle = flowHandle;
	response.layerId = layerId;
	response.streamFlags = streamFlags;
	response.response = WAITING_ON_RESPONSE;

	RtlInsertElementGenericTableAvl(table, (PVOID)&response, (CLONG)sizeof(THResponse), &success);

	if (!success) {
		status = STATUS_UNSUCCESSFUL;
	}
	return status;
}

NTSTATUS ThHandleResponse(IN THResponseTable* table, IN UINT64 flowHandle, IN THResponseType answer) {
	NTSTATUS status = STATUS_SUCCESS;
	THResponse* response;
	THResponse lookup_response;
	lookup_response.flowHandle = flowHandle;
	// find the response, add the answer, and call FwpsStreamContinue0
	response = (THResponse*)RtlLookupElementGenericTableAvl(table, (PVOID)&lookup_response);
	if (response == NULL) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Could not find the response we want to handle\r\n");
		return STATUS_NOT_FOUND;
	}

	response->response = answer;

	status = FwpsStreamContinue(flowHandle, TrustHub_callout_id, response->layerId, response->streamFlags);
	DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Continuing stream for flow handle %x\r\n", flowHandle);
	return status;
}

NTSTATUS ThPopResponse(IN THResponseTable* table, IN UINT64 flowHandle, OUT THResponseType* answer) {
	NTSTATUS status = STATUS_SUCCESS;
	THResponse* response;
	THResponse lookup_response;
	lookup_response.flowHandle = flowHandle;
	response = (THResponse*)RtlLookupElementGenericTableAvl(table, (PVOID)&lookup_response);
	if (response == NULL) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Could not find the response to handle");
		return STATUS_NOT_FOUND;
	}

	*answer = response->response;
	if (response->response == WAITING_ON_RESPONSE) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Error! ThPopResponse was called too soon");
		return STATUS_WAIT_0;
	}

	if (!RtlDeleteElementGenericTableAvl(table, (PVOID)&lookup_response)) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Error! Could not find response in table");
		return STATUS_NOT_FOUND;
	}

	return status;
}

RTL_GENERIC_COMPARE_RESULTS ThResponseCompare(IN THResponseTable* table, IN PVOID firstStruct, IN PVOID secondStruct) {
	UNREFERENCED_PARAMETER(table);
	THResponse* response1;
	THResponse* response2;
	response1 = (THResponse*)firstStruct;
	response2 = (THResponse*)secondStruct;
	if (response1->flowHandle > response2->flowHandle) {
		return GenericGreaterThan;
	} else if (response1->flowHandle < response2->flowHandle) {
		return GenericLessThan;
	}
	return GenericEqual;
}

PVOID ThResponseAllocate(IN THResponseTable* table, IN CLONG byteSize) {
	UNREFERENCED_PARAMETER(table);
	PVOID response;
	response = ExAllocatePoolWithTag(NonPagedPool, byteSize, TH_POOL_TAG);
	return response;
}

VOID ThResponseFree(IN THResponseTable table, IN PVOID buffer) {
	UNREFERENCED_PARAMETER(table);
	ExFreePoolWithTag(buffer, TH_POOL_TAG);
}