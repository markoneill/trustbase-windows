#include "TrustBaseMessage.h"

NTSTATUS TbInitMessageQueue(OUT TBMessageQueue* queue) {
	NTSTATUS status = STATUS_SUCCESS;

	KeInitializeSpinLock(&(queue->lock));
	InitializeListHead(&(queue->ListHead));

	return status;
}

NTSTATUS TbMakeMessage(OUT TBMessage** msg, IN UINT64 flowHandle, IN UINT64 processID, IN FWP_BYTE_BLOB processPath) {

	//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Entered Make Message - Time=%llu\r\n", getTime());
	// Allocate space
	(*msg) = (TBMessage*)ExAllocatePoolWithTag(NonPagedPool, sizeof(TBMessage), TB_POOL_TAG);
	RtlZeroMemory((*msg), sizeof(TBMessage));

	// Fill it
	(*msg)->flowHandle = flowHandle;
	(*msg)->processID = processID;
	(*msg)->bytesWritten = 0;
	(*msg)->processPathSize = processPath.size;
	(*msg)->processPath = (UINT8*)ExAllocatePoolWithTag(NonPagedPool, sizeof(UINT8) * processPath.size, TB_POOL_TAG);
	RtlCopyMemory((*msg)->processPath, processPath.data, processPath.size);
	//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Exited Make Message - Time=%llu\r\n", getTime());

	return STATUS_SUCCESS;
}



NTSTATUS TbAddMessage(IN TBMessageQueue* queue, IN TBMessage* msg) {
	KIRQL irql;

	//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Entered Add Message - Time=%llu\r\n", getTime());
	DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Before adding message, list is %sempty\r\n", ((IsListEmpty(&(queue->ListHead))) ? "" : "not "));

	// add it to our thing
	KeAcquireSpinLock(&(queue->lock), &irql);
	InsertTailList(&(queue->ListHead), &(msg->ListEntry));
	KeReleaseSpinLock(&(queue->lock), irql);
	//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Exited Add Message - Time=%llu\r\n", getTime());
	return STATUS_SUCCESS;
}

NTSTATUS TbSizeNextMessage(IN TBMessageQueue* queue, OUT size_t* len) {
	NTSTATUS status = STATUS_SUCCESS;
	TBMessage* message;
	LIST_ENTRY* entry;

	//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Entered SizeNextMessage Message\r\n");
	if (IsListEmpty(&(queue->ListHead))) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Could not find another message\r\n");
		//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Exited SizeNextMessage Message\r\n");
		return STATUS_NOT_FOUND;
	}
	entry = queue->ListHead.Flink;
	message = (TBMessage*)CONTAINING_RECORD(entry, TBMessage, ListEntry);

	TbSizeMessage(message, len);
	//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Exited SizeNextMessage Message\r\n");
	return status;
}

VOID TbSizeMessage(IN TBMessage* message, OUT size_t* len) {

	//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Entered SizeMessage Message\r\n");
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
	//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Exited SizeMessage Message\r\n");

}

// messages popped must later be freed with TbFreeMessage								 -- NEVER USED --
NTSTATUS TbPopMessage(IN TBMessageQueue* queue, OUT TBMessage** message) {
	NTSTATUS status = STATUS_SUCCESS;
	KIRQL irql;
	//PLIST_ENTRY mentry = ExInterlockedRemoveHeadList(&(queue.ListHead), &(queue.lock));
	//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Entered PopMessage\r\n");

	KeAcquireSpinLock(&(queue->lock), &irql);
	PLIST_ENTRY mentry = RemoveHeadList(&(queue->ListHead));
	KeReleaseSpinLock(&(queue->lock), irql);
	if (mentry == NULL) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Could not pop message\r\n");
		//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Exited PopMessage\r\n");
		return STATUS_NOT_FOUND;
	}

	*message = (TBMessage*)CONTAINING_RECORD(mentry, TBMessage, ListEntry);
	//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Exited PopMessage\r\n");
	return status;
}

// doesn't pop the message, so it must be removed afterward
NTSTATUS TbGetMessage(IN TBMessageQueue* queue, OUT TBMessage** message) {
	NTSTATUS status = STATUS_SUCCESS;
	LIST_ENTRY* entry;
	KIRQL irql;
	//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Entered GetMessage - Time=%llu\r\n", getTime());
	KeAcquireSpinLock(&(queue->lock), &irql);
	if (IsListEmpty(&(queue->ListHead))) {
		KeReleaseSpinLock(&(queue->lock), irql);
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Could not find a message, list is empty\r\n");
		//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Exited PopMessage - Time=%llu\r\n", getTime());
		return STATUS_NOT_FOUND;
	}
	entry = queue->ListHead.Flink;
	*message = (TBMessage*)CONTAINING_RECORD(entry, TBMessage, ListEntry);
	KeReleaseSpinLock(&(queue->lock), irql);
	//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Exited PopMessage - Time=%llu\r\n", getTime());
	return status;
}

// pop and free
NTSTATUS TbRemMessage(IN TBMessageQueue* queue) {
	TBMessage* message;
	NTSTATUS status = STATUS_SUCCESS;
	KIRQL irql;

	//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Entered RemMessage - Time=%llu\r\n", getTime());

	if (IsListEmpty(&(queue->ListHead))) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Tried to remove from empty list, could not remove message\r\n");
		return STATUS_UNSUCCESSFUL;
	}

	KeAcquireSpinLock(&(queue->lock), &irql);
	PLIST_ENTRY mentry = RemoveHeadList(&(queue->ListHead));
	KeReleaseSpinLock(&(queue->lock), irql);

	DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "After removing message, list is %sempty\r\n", ((IsListEmpty(&(queue->ListHead))) ? "" : "not "));

	if (mentry == NULL) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Could not pop message\r\n");
		//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Exited RemMessage - Time=%llu\r\n", getTime());
		return STATUS_NOT_FOUND;
	}

	message = (TBMessage*)CONTAINING_RECORD(mentry, TBMessage, ListEntry);

	TbFreeMessage(message);
	//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Exited RemMessage - Time=%llu\r\n", getTime());
	return status;
}

NTSTATUS TbFreeMessage(IN TBMessage* message) {
	//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Entered FreeMessage - Time=%llu\r\n", getTime());
	if (message) {
		if (message->serverHello) {
			ExFreePoolWithTag(message->serverHello, TB_POOL_TAG);
		}
		if (message->clientHello) {
			ExFreePoolWithTag(message->clientHello, TB_POOL_TAG);
		}
		if (message->data) {
			ExFreePoolWithTag(message->data, TB_POOL_TAG);
		}
		if (message->processPath) {
			ExFreePoolWithTag(message->processPath, TB_POOL_TAG);
		}
		ExFreePoolWithTag(message, TB_POOL_TAG);
	}
	//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Exited FreeMessage - Time=%llu\r\n", getTime());
	return STATUS_SUCCESS;
}

NTSTATUS TbCopyMessage(IN UINT8* buffer, size_t bufsize, TBMessage* message, size_t* bytesWritten) {
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

	//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Entered CopyMessage - Time=%llu\r\n", getTime());
	TbSizeMessage(message, &len);

	//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Copying message:\r\n");
	
	cursor = buffer;
	bufend = buffer + bufsize;

	switch (message->bytesWritten) {
	case TBMESSAGE_OFFSET_LEN:
		if (cursor + sizeof(UINT64) > bufend) break;
		*(UINT64 UNALIGNED *)cursor = (UINT64)(len);
		cursor += sizeof(UINT64);
		//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Len:%x\r\n", len);

	case TBMESSAGE_OFFSET_FLOWHANDLE:
		if (cursor + sizeof(UINT64) > bufend) break;
		*(UINT64 UNALIGNED *)cursor = message->flowHandle;
		cursor += sizeof(UINT64);
		//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "flowHandle:%x\r\n", message->flowHandle);

	case TBMESSAGE_OFFSET_PROCESSID:
		if (cursor + sizeof(UINT64) > bufend) break;
		*(UINT64 UNALIGNED *)cursor = message->processID;
		cursor += sizeof(UINT64);
		//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "PID:%x\r\n", message->processID);

	case TBMESSAGE_OFFSET_PROCESSPATHSIZE:
		if (cursor + sizeof(UINT32) > bufend) break;
		*(UINT32 UNALIGNED *)cursor = message->processPathSize;
		cursor += sizeof(UINT32);
		//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "PathSize:%x\r\n", message->processPathSize);

	case TBMESSAGE_OFFSET_PROCESSPATH:
		if (cursor + message->processPathSize > bufend) {
			// here we could copy over a portion, but our min buffer is bigger than MAX_PATH
			break;
		}
		RtlCopyMemory(cursor, message->processPath, message->processPathSize);
		cursor += message->processPathSize;
		//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Path:%S\r\n", message->processPath);

	default:
		loc_clienthellosize = TBMESSAGE_OFFSET_PROCESSPATH + message->processPathSize;
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
			//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Client Hello Size:%x\r\n", message->clientHelloSize);
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
			//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Server Hello Size:%x\r\n", message->serverHelloSize);
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
			//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Data Size:%x\r\n", message->dataSize);
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

			//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Cursor %x\r\n", cursor);
			//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Message->data %x\r\n", message->data);
			//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "len %d\r\n", len);

			//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "before potential break\r\n");
			
			//message->data was NULL, WHY??????? Look at the windbg outputaaaa

			RtlCopyMemory(cursor, message->data + offset, len);									 //Failed on this line in the past ------------------------------------- memcpy error
																								
			//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "after potential break\r\n");
			cursor += len;
			DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Finished sending Certificate\r\n");
		}



		/*
		// find where we are
		if (message->bytesWritten + (cursor - buffer) == message->processPathSize + TBMessage_OFFSET_PROCESSPATH) {
			if (cursor + sizeof(UINT32) > bufend) break;
			*(UINT32 UNALIGNED *)cursor = message->dataSize;
			cursor += sizeof(UINT32);
			DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "DataSize:%x\r\n", message->dataSize);
		}

		// offset in the buffer is the amount we have written - the size of bytes before data
		// (bytes_written_in_prev_reads + bytes_written_this_read) - (offset_to_path + size_of_process_path + size_of_data_size_field)
		offset = (message->bytesWritten + (cursor - buffer)) - (TBMessage_OFFSET_PROCESSPATH + message->processPathSize + sizeof(UINT32));
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
	DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Bytes written this call %d, total %d\r\n", cursor - buffer, message->bytesWritten);
	//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Exited CopyMessage - Time=%llu\r\n", getTime());
	return status;
}

NTSTATUS TbFinishedMessage(IN TBMessage* message) {
	NTSTATUS status = STATUS_SUCCESS;
	size_t len;
	//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Entered FinishedMessage - Time=%llu\r\n", getTime());
	TbSizeMessage(message, &len);
	
	if (message->bytesWritten < len) {
		return STATUS_PARTIAL_COPY;
	}
	//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Exited FinishedMessage - Time=%llu\r\n", getTime());
	return status;
}

VOID TbPrintMessage(IN TBMessage * message) {
	//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Entered PrintMessage - Time=%llu\r\n", getTime());
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
	//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Exited PrintMessage - Time=%llu\r\n", getTime());
	return;
}

NTSTATUS TbInitResponseTable(IN TBResponseTable* table) {
	NTSTATUS status = STATUS_SUCCESS;
	RtlInitializeGenericTableAvl((PRTL_AVL_TABLE)table, (PRTL_AVL_COMPARE_ROUTINE)TbResponseCompare, (PRTL_AVL_ALLOCATE_ROUTINE)TbResponseAllocate, (PRTL_AVL_FREE_ROUTINE)TbResponseFree, NULL);
	return status;
}

NTSTATUS TbAddResponse(IN TBResponseTable* table, IN UINT64 flowHandle, IN UINT16 layerId, IN UINT32 streamFlags) {
	NTSTATUS status = STATUS_SUCCESS;
	TBResponse response;
	boolean success;

	//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Entered AddResponse - Time=%llu\r\n", getTime());
	response.flowHandle = flowHandle;
	response.layerId = layerId;
	response.streamFlags = streamFlags;
	response.response = WAITING_ON_RESPONSE;

	RtlInsertElementGenericTableAvl(table, (PVOID)&response, (CLONG)sizeof(TBResponse), &success);

	if (!success) {
		status = STATUS_UNSUCCESSFUL;
	} else {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Added to Response List, len now %d\r\n", RtlNumberGenericTableElementsAvl(table));
	}
	//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Exited AddResponse - Time=%llu\r\n", getTime());
	return status;
}

NTSTATUS TbHandleResponse(IN TBResponseTable* table, IN UINT64 flowHandle, IN TBResponseType answer) {
	NTSTATUS status = STATUS_SUCCESS;
	TBResponse* response;
	TBResponse lookup_response;
	//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Entered HandleResponse - Time=%llu\r\n", getTime());

	lookup_response.flowHandle = flowHandle;
	// find the response, add the answer, and call FwpsStreamContinue0
	response = (TBResponse*)RtlLookupElementGenericTableAvl(table, (PVOID)&lookup_response);
	if (response == NULL) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Could not find the response we want to handle\r\n");
		//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Exited HandleResponse - Time=%llu\r\n", getTime());
		return STATUS_NOT_FOUND;
	}

	response->response = answer; //Write the answer to the GenericTableAvl

	DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Added %d as a response\r\n", response->response);

	
	status = FwpsStreamContinue(flowHandle, TrustBase_callout_stream_id, response->layerId, response->streamFlags);
	if (status != STATUS_SUCCESS) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "NOT SUCCESSFUL - Continuing stream was NOT SUCCESSFUL for flow handle %x\r\n", flowHandle);
		//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Exited HandleResponse - Time=%llu\r\n", getTime());
		return status;
	}
	DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Continuing stream for flow handle %d\r\n", flowHandle);
	//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Exited HandleResponse - Time=%llu\r\n", getTime());
	return status;
}

NTSTATUS TbPopResponse(IN TBResponseTable* table, IN UINT64 flowHandle, OUT TBResponseType* answer) {
	NTSTATUS status = STATUS_SUCCESS;
	TBResponse* response;
	TBResponse lookup_response;
	//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Entered PopResponse - Time=%llu\r\n", getTime());
	
	lookup_response.flowHandle = flowHandle;
	response = (TBResponse*)RtlLookupElementGenericTableAvl(table, (PVOID)&lookup_response);
	if (response == NULL) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Could not find the response to handle");
		//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Exited PopResponse\r\n");
		return STATUS_NOT_FOUND;
	}
	if (response->response != WAITING_ON_RESPONSE) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Popping response = %d for flow handle: %d\r\n", response->response, flowHandle);
		//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Lets see the flow handle = %d\r\n", flowHandle);
	}


	*answer = response->response;

	if (response->response == WAITING_ON_RESPONSE) {     //We error out and basically say success. We don't pop it out of the table. BAD?
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Error! TbPopResponse was called too soon\r\n");
		//if (cleanup == 1) {
		//	return STATUS_NOT_FOUND               //We couldn't pop the response so should we just error? 
		//
		//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Exited PopResponse\r\n");
		return STATUS_WAIT_0; // Basically status success if you take it as zero 
	}

	if (!RtlDeleteElementGenericTableAvl(table, (PVOID)&lookup_response)) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Error! Could not find response in table");
		//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Exited PopResponse\r\n");
		return STATUS_NOT_FOUND;
	}
	//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Removed from Response List, len now %d\r\n", RtlNumberGenericTableElementsAvl(table));

	//*answer = 1; //Now change the other code to not store anything in the tables. There should never be anything in the Generic Table this way
	//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Exited PopResponse - Time=%llu\r\n", getTime());
	return status;
}

RTL_GENERIC_COMPARE_RESULTS TbResponseCompare(IN TBResponseTable* table, IN PVOID firstStruct, IN PVOID secondStruct) {
	UNREFERENCED_PARAMETER(table);
	TBResponse* response1;
	TBResponse* response2;
	//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Entered ResponseCompare\r\n");

	response1 = (TBResponse*)firstStruct;
	response2 = (TBResponse*)secondStruct;
	if (response1->flowHandle > response2->flowHandle) {
		//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Exited ResponseCompare\r\n");
		return GenericGreaterThan;
	} else if (response1->flowHandle < response2->flowHandle) {
		//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Exited ResponseCompare\r\n");
		return GenericLessThan;
	}
	//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Exited ResponseCompare\r\n");
	return GenericEqual;
}

PVOID TbResponseAllocate(IN TBResponseTable* table, IN CLONG byteSize) {
	UNREFERENCED_PARAMETER(table);
	PVOID response;
	//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Entered ResponseAllocate\r\n");
	response = ExAllocatePoolWithTag(NonPagedPool, byteSize, TB_POOL_TAG);
	//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Exited ResponseAllocate\r\n");
	return response;
}

VOID TbResponseFree(IN TBResponseTable table, IN PVOID buffer) {
	UNREFERENCED_PARAMETER(table);
	//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Entered ResponseFree\r\n");
	ExFreePoolWithTag(buffer, TB_POOL_TAG);
	//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Exited ResponseFree\r\n");
}