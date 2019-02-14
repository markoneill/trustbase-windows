#include <fwpsk.h>
#include <Ntstrsafe.h>
#include "ConnectionContext.h"
#include "TrustBaseGuid.h"
#include "HandshakeHandler.h"

#define UINT24 UINT32

#define DEBUGHEXLEN 8
void NTAPI printData(FWPS_STREAM_DATA *dataStream);
BOOL stateCanTransition(IN FWPS_STREAM_DATA *dataStream, IN ConnectionFlowContext *context);
void ffBytes(IN ConnectionFlowContext *context, IN int amount);
byte nextByte(IN FWPS_STREAM_DATA *dataStream, IN ConnectionFlowContext *context);
byte peekByte(IN FWPS_STREAM_DATA *dataStream, IN ConnectionFlowContext *context);
UINT16 nextUint16(IN FWPS_STREAM_DATA *dataStream, IN ConnectionFlowContext *context);
UINT24 nextUint24(IN FWPS_STREAM_DATA *dataStream, IN ConnectionFlowContext *context);
UINT32 nextUint32(IN FWPS_STREAM_DATA *dataStream, IN ConnectionFlowContext *context);
NTSTATUS nextNetBuffer(IN NET_BUFFER* currentBuffer, IN NET_BUFFER_LIST* currentList, OUT NET_BUFFER** newBuffer, OUT NET_BUFFER_LIST** newList, OUT ULONG* newLen);

REQUESTED_ACTION NTAPI handleStateUnknown(IN FWPS_STREAM_DATA *dataStream, IN ConnectionFlowContext *context);
REQUESTED_ACTION NTAPI handleStateRecordLayer(IN FWPS_STREAM_DATA *dataStream, IN ConnectionFlowContext *context);
REQUESTED_ACTION NTAPI handleStateHandshakeLayer(IN FWPS_STREAM_DATA *dataStream, IN ConnectionFlowContext *context);

NTSTATUS copyData(IN FWPS_STREAM_DATA *dataStream, IN ConnectionFlowContext *context, IN unsigned int copy_len, OUT UINT8** data);

// calls the appropriate function to parse the data, and updates the state
REQUESTED_ACTION NTAPI updateState(IN FWPS_STREAM_DATA *dataStream, IN ConnectionFlowContext *context ) {
	REQUESTED_ACTION ra;
	ra = RA_NEED_MORE;

	//printData(dataStream);

	while (stateCanTransition(dataStream, context)) {
		switch (context->currentState) {
		case PS_UNKNOWN:
			//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Unknown connection state\r\n");
			ra = handleStateUnknown(dataStream, context);
			break;
		case PS_RECORD_LAYER:
			//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Record Layer\r\n");
			ra = handleStateRecordLayer(dataStream, context);
			break;
		case PS_HANDSHAKE_LAYER:
			//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Handshake Layer\r\n");
			ra = handleStateHandshakeLayer(dataStream, context);
			break;
		case PS_CERTIFICATE:
			return ra;
		default:
			DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Bad connection state\r\n");
			return ra;
		}
	}
	return ra;
}

NTSTATUS copyData(IN FWPS_STREAM_DATA *dataStream, IN ConnectionFlowContext *context, IN unsigned int copy_len, OUT UINT8** data) {
	NTSTATUS status = STATUS_SUCCESS;
	NET_BUFFER_LIST* currentList;
	NET_BUFFER* currentBuffer;
	ULONG datalen = 0;
	ULONG traversed = 0;
	ULONG copied = 0;
	ULONG offset = 0;
	UINT8* buf;

	*data = NULL;

	// seek to the certificate
	currentList = dataStream->netBufferListChain;

	currentBuffer = NET_BUFFER_LIST_FIRST_NB(currentList);
	if (currentBuffer == NULL) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Null buffer\r\n");
		datalen = 0;
	} else {
		datalen = currentBuffer->DataLength;
	}

	// seek to byte
	// while we still need to jump over buffers, and we have bytes to jump over
	while (context->bytesRead >= traversed + datalen && datalen > 0) {
		traversed += datalen;
		status = nextNetBuffer(currentBuffer, currentList, &currentBuffer, &currentList, &datalen);
		if (!NT_SUCCESS(status)) {
			DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Error, did not get Certificate, could not get the next net buffer\r\n");
			return STATUS_NOT_FOUND;
		}
	}
	if (datalen == 0) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Error, did not get Certificate\r\n");
		return STATUS_NOT_FOUND;
	}
	// we now have the correct buffer, but we may have to keep traversing multiple buffers to get all the data
	// allocate the area for the certificate
	(*data) = (UINT8*)ExAllocatePoolWithTag(NonPagedPool, copy_len, TB_POOL_TAG);
	
	// copy over the data
	offset = context->bytesRead - traversed;
	while (copied < copy_len) {
		if (datalen >= copy_len - copied) {
			// clean copy
			buf = (UINT8*)NdisGetDataBuffer(currentBuffer, offset + (copy_len - copied), NULL, 1, 0);
			if (buf == NULL) {
				DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Error, did not get Certificate, could not get the data buffer while copying\r\n");
				*data = NULL;
				return STATUS_NOT_FOUND;
			}
			RtlCopyMemory(&((*data)[copied]), &(buf[offset]), copy_len - copied);
			break;
		} else {
			// copy what we can
			buf = (UINT8*)NdisGetDataBuffer(currentBuffer, datalen, NULL, 1, 0);
			if (buf == NULL) {
				DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Error, did not get Certificate, could not get the data buffer while copying a portion\r\n");
				*data = NULL;
				return STATUS_NOT_FOUND;
			}
			RtlCopyMemory(&((*data)[copied]), &(buf[offset]), datalen - offset);
			copied += datalen - offset;
			offset = 0;
		}

		status = nextNetBuffer(currentBuffer, currentList, &currentBuffer, &currentList, &datalen);
		if (!NT_SUCCESS(status)) {
			DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Error, did not get Certificate, could not get the next net buffer while copying\r\n");
			*data = NULL;
			return STATUS_NOT_FOUND;
		}
	}

	context->bytesRead += copy_len;
	context->bytesToRead -= copy_len;
	return status;
}

REQUESTED_ACTION NTAPI handleStateUnknown(IN FWPS_STREAM_DATA *dataStream, IN ConnectionFlowContext *context) {
	byte nbyte = peekByte(dataStream, context);
	if (nbyte == TB_TLS_HANDSHAKE_IDENTIFIER) {
		context->currentState = PS_RECORD_LAYER;
		context->bytesToRead = TB_TLS_RECORD_HEADER_SIZE;
		return RA_NEED_MORE;
	} else {
		context->currentState = PS_IRRELEVANT;
		context->bytesToRead = 0;
	}
	return RA_NOT_INTERESTED;
}

REQUESTED_ACTION NTAPI handleStateRecordLayer(IN FWPS_STREAM_DATA *dataStream, IN ConnectionFlowContext *context) {
	byte nbyte;
	UINT16 record_length;

	nbyte = nextByte(dataStream, context);
	//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "From record: see first byte as %x\r\n", nbyte);
	if (nbyte != TB_TLS_HANDSHAKE_IDENTIFIER) {
		context->currentState = PS_IRRELEVANT;
		context->bytesToRead = 0;
		return RA_NOT_INTERESTED;
	}

	// TLS version #
	nbyte = nextByte(dataStream, context);
	//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "TLS major %x\r\n", nbyte);
	nbyte = nextByte(dataStream, context);
	//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "TLS minor %x\r\n", nbyte);

	// version
	record_length = nextUint16(dataStream, context);
	//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Record Length %x\r\n", record_length);
	context->bytesToRead = record_length;
	context->recordLength = record_length;
	context->currentState = PS_HANDSHAKE_LAYER;
	return RA_NEED_MORE;
}

REQUESTED_ACTION NTAPI handleStateHandshakeLayer(IN FWPS_STREAM_DATA *dataStream, IN ConnectionFlowContext *context) {
	NTSTATUS status;
	byte nbyte;
	UINT24 handshake_message_length;
	unsigned int copy_len = 0;

	while (context->bytesToRead > 0) { // read all the records we can
		nbyte = nextByte(dataStream, context);
		handshake_message_length = nextUint24(dataStream, context);
		//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Got handshake message type %x, length %x\r\n", nbyte, handshake_message_length);

		// handle different handshake message types
		if (nbyte == TYPE_CLIENT_HELLO) {
			//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Sent a client hello\r\n");
			//ffBytes(context, handshake_message_length);
			// save the client hello
			context->bytesRead -= 3;
			copy_len = handshake_message_length + 3;
			context->message->clientHelloSize = handshake_message_length + 3;
			copyData(dataStream, context, copy_len, &(context->message->clientHello));

			// Expect a header next packet
			context->bytesToRead = TB_TLS_RECORD_HEADER_SIZE;
			context->currentState = PS_RECORD_LAYER;
			return RA_CONTINUE; // we don't care about the rest of this record
		} else if (nbyte == TYPE_SERVER_HELLO) {
			//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Received a Server Hello\r\n");
			//ffBytes(context, handshake_message_length); // fastforward past the server hello
			// copy the server hello
			context->bytesRead -= 3;
			copy_len = handshake_message_length + 3;
			context->message->serverHelloSize = handshake_message_length + 3;
			copyData(dataStream, context, copy_len, &(context->message->serverHello));
			// if we have read all of this record, lets hop out
			// else current state remains at handshake
			if (context->bytesRead <= (unsigned)(context->recordLength + TB_TLS_RECORD_HEADER_SIZE)) {
				context->bytesToRead = TB_TLS_RECORD_HEADER_SIZE;
				context->currentState = PS_RECORD_LAYER;
				return RA_NEED_MORE;
			}
		} else if (nbyte == TYPE_CERTIFICATE) {
			// get the certificates size
			//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Found the Certificate\r\n");
			handshake_message_length = nextUint24(dataStream, context);

			context->bytesRead -= 3;
			context->bytesToRead = handshake_message_length + 3;
			context->currentState = PS_CERTIFICATE;

			context->message->dataSize = handshake_message_length + 3;
			copy_len = handshake_message_length + 3;
			DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Certificate size %d\r\n", context->message->dataSize);
			status = copyData(dataStream, context, copy_len, &(context->message->data));
			if (status == STATUS_NOT_FOUND) {
				return RA_ERROR;
			}
			return RA_WAIT;

		} else if (nbyte == TYPE_CERTIFICATE_VERIFY || nbyte == TYPE_CLIENT_KEY_EXCHANGE) {
			DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Got handshake message type verify or key exchange, and we should not have gotten here.\r\n");
			context->bytesToRead = 0;
			context->currentState = PS_ERROR;
			return RA_ERROR; // we don't care about the rest of this record
		} else {
			DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Got an unknown record type\r\n");
			context->bytesToRead = 0;
			context->currentState = PS_ERROR;
			return RA_ERROR;
		}
	}

	return RA_ERROR;
}

BOOL stateCanTransition(IN FWPS_STREAM_DATA *dataStream, IN ConnectionFlowContext *context) {
	//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Can transition? State %x, b2r %x, br %x, dl %x\r\n", context->currentState, context->bytesToRead, context->bytesRead, dataStream->dataLength);
	return ((context->bytesToRead > 0) && (context->bytesRead < dataStream->dataLength) && (context->bytesToRead <= dataStream->dataLength - context->bytesRead));
}

byte peekByte(IN FWPS_STREAM_DATA *dataStream, IN ConnectionFlowContext *context) {
	byte retbyte = nextByte(dataStream, context);
	context->bytesRead -= 1;
	context->bytesToRead += 1;
	return retbyte;
}


void ffBytes(IN ConnectionFlowContext *context, IN int amount) {
	context->bytesRead += amount;
}

UINT16 nextUint16(IN FWPS_STREAM_DATA *dataStream, IN ConnectionFlowContext *context) {
	// TODO: change this to something not intel specific

	UINT16 ret = 0;
	UINT16 piece;

	piece = (UINT16)nextByte(dataStream, context);
	ret = piece << 8;
	ret |= (UINT16)nextByte(dataStream, context);
	return ret;
}

UINT24 nextUint24(IN FWPS_STREAM_DATA *dataStream, IN ConnectionFlowContext *context) {
	// TODO: change this to something not intel specific

	UINT24 ret = 0;
	UINT24 piece;
	piece = (UINT24)nextByte(dataStream, context);
	ret |= piece << 16;
	piece = (UINT24)nextByte(dataStream, context);
	ret |= piece << 8;
	ret |= (UINT24)nextByte(dataStream, context);

	return ret;
}

UINT32 nextUint32(IN FWPS_STREAM_DATA *dataStream, IN ConnectionFlowContext *context) {
	// TODO: change this to something not intel specific

	UINT32 ret = 0;
	UINT32 piece;
	piece = (UINT24)nextByte(dataStream, context);
	ret |= piece << 24;
	piece = (UINT24)nextByte(dataStream, context);
	ret |= piece << 16;
	piece = (UINT24)nextByte(dataStream, context);
	ret |= piece << 8;
	ret |= (UINT24)nextByte(dataStream, context);

	return ret;
}

byte nextByte(IN FWPS_STREAM_DATA *dataStream, IN ConnectionFlowContext *context) {
	NET_BUFFER_LIST* currentList;
	NET_BUFFER* currentBuffer;
	byte* data;
	byte retbyte;
	ULONG datalen = 0;
	ULONG traversed = 0;

	currentList = dataStream->netBufferListChain;

	currentBuffer = NET_BUFFER_LIST_FIRST_NB(currentList);
	if (currentBuffer == NULL) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Null buffer\r\n");
		datalen = 0;
	} else {
		datalen = currentBuffer->DataLength;
	}

	// seek to byte
	// while we still need to jump over buffers, and we have bytes to jump over
	while (context->bytesRead >= traversed + datalen && datalen > 0) {
		traversed += datalen;
		nextNetBuffer(currentBuffer, currentList, &currentBuffer, &currentList, &datalen);
	}

	// read byte
	if (datalen > 0) {
		data = NdisGetDataBuffer(currentBuffer, 1 + (context->bytesRead - traversed), NULL, 1, 0);
		retbyte = data[context->bytesRead - traversed];
		context->bytesRead += 1;
		context->bytesToRead -= 1;
	} else {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Error, did not get a byte\r\n");
		retbyte = 0x0;
	}
	return retbyte;
}

NTSTATUS nextNetBuffer(IN NET_BUFFER* currentBuffer, IN NET_BUFFER_LIST* currentList, OUT NET_BUFFER** newBuffer, OUT NET_BUFFER_LIST** newList, OUT ULONG* newLen) {
	NTSTATUS status = STATUS_SUCCESS;

	(*newBuffer) = NET_BUFFER_NEXT_NB(currentBuffer);
	if ((*newBuffer) == NULL) {
		// first try to get the next list
		*newList = NET_BUFFER_LIST_NEXT_NBL(currentList);
		if ((*newList) == NULL) {
			// reached end, no other lists
			DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Hit a Null list while getting next buffer\r\n");
			*newLen = 0;
			*newBuffer = NULL;
			return STATUS_NOT_FOUND;
		} else {
			// found a new list
			// get next buffer
			*newBuffer = NET_BUFFER_LIST_FIRST_NB(*newList);
			if ((*newBuffer) == NULL) {
				// reached end anyways
				DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Hit a Null buffer while getting next buffer\r\n");
				*newLen = 0;
				return STATUS_NOT_FOUND;
			} else {
				// this buffer is good
				*newLen = (*newBuffer)->DataLength;
			}
		}
	} else {
		*newLen = (*newBuffer)->DataLength;
	}
	return status;
}

void NTAPI printData(FWPS_STREAM_DATA *dataStream) {
	NET_BUFFER_LIST *currentList;
	NET_BUFFER *currentBuffer;
	ULONG datalen = 0;
	byte *data;
	int bufcount;
	int listcount;
	ULONG i, j;
	char debugout[] = " __ __ __ __ __ __ __ __";

	//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Entered printData\r\n");

	currentList = dataStream->netBufferListChain;
	listcount = 0;

	while (currentList != NULL) {
		currentBuffer = NET_BUFFER_LIST_FIRST_NB(currentList);
		bufcount = 0;

		while (currentBuffer != NULL) {
			data = NdisGetDataBuffer(currentBuffer, 1, NULL, 1, 0);
			datalen = currentBuffer->DataLength;

			//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "List %d Buffer %d len %x\r\n", listcount, bufcount, datalen);
			for (i = 0; i < datalen; i += DEBUGHEXLEN) {
				for (j = 0; j < DEBUGHEXLEN; j++) {
					if (i + j < datalen) {
						RtlStringCchPrintfA(debugout + (j * 3), 4, " %02x\r\n", data[i + j]);
					}
					else {
						break;
					}
				}
				//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "%s\r\n", debugout);
			}
			currentBuffer = NET_BUFFER_NEXT_NB(currentBuffer);
			bufcount++;
		}
		currentList = NET_BUFFER_LIST_NEXT_NBL(currentList);
		listcount++;
	}
	return;	
}