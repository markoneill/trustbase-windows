#include <fwpsk.h>
#include <Ntstrsafe.h>
#include "ConnectionContext.h"
#include "HandshakeHandler.h"

#define UINT24 UINT32

#define DEBUGHEXLEN 8
void NTAPI printData(FWPS_STREAM_DATA *dataStream);
BOOL stateCanTransition(_In_ FWPS_STREAM_DATA *dataStream, _In_ ConnectionFlowContext *context);
void ffBytes(_In_ ConnectionFlowContext *context, _In_ int amount);
byte nextByte(_In_ FWPS_STREAM_DATA *dataStream, _In_ ConnectionFlowContext *context);
byte peekByte(_In_ FWPS_STREAM_DATA *dataStream, _In_ ConnectionFlowContext *context);
UINT16 nextUint16(_In_ FWPS_STREAM_DATA *dataStream, _In_ ConnectionFlowContext *context);
UINT24 nextUint24(_In_ FWPS_STREAM_DATA *dataStream, _In_ ConnectionFlowContext *context);
UINT32 nextUint32(_In_ FWPS_STREAM_DATA *dataStream, _In_ ConnectionFlowContext *context);

REQUESTED_ACTION NTAPI handleStateUnknown(_In_ FWPS_STREAM_DATA *dataStream, _In_ ConnectionFlowContext *context);
REQUESTED_ACTION NTAPI handleStateRecordLayer(_In_ FWPS_STREAM_DATA *dataStream, _In_ ConnectionFlowContext *context);
REQUESTED_ACTION NTAPI handleStateHandshakeLayer(_In_ FWPS_STREAM_DATA *dataStream, _In_ ConnectionFlowContext *context);

// calls the appropriate function to parse the data, and updates the state
REQUESTED_ACTION NTAPI updateState(_In_ FWPS_STREAM_DATA *dataStream, _In_ ConnectionFlowContext *context ) {
	REQUESTED_ACTION ra;
	ra = RA_ERROR;
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
			DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Found the Certificate\r\n");
			return ra;
		default:
			DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Bad connection state\r\n");
			return ra;
		}
	}
	return ra;
}

REQUESTED_ACTION NTAPI handleStateUnknown(_In_ FWPS_STREAM_DATA *dataStream, _In_ ConnectionFlowContext *context) {
	byte nbyte = peekByte(dataStream, context);
	if (nbyte == TH_TLS_HANDSHAKE_IDENTIFIER) {
		context->currentState = PS_RECORD_LAYER;
		context->bytesToRead = TH_TLS_RECORD_HEADER_SIZE;
		return RA_NEED_MORE;
	} else {
		context->currentState = PS_IRRELEVANT;
		context->bytesToRead = 0;
	}
	return RA_NOT_INTERESTED;
}

REQUESTED_ACTION NTAPI handleStateRecordLayer(_In_ FWPS_STREAM_DATA *dataStream, _In_ ConnectionFlowContext *context) {
	byte nbyte;
	UINT16 record_length;

	nbyte = nextByte(dataStream, context);
	//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "From record: see first byte as %x\r\n", nbyte);
	if (nbyte != TH_TLS_HANDSHAKE_IDENTIFIER) {
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

REQUESTED_ACTION NTAPI handleStateHandshakeLayer(_In_ FWPS_STREAM_DATA *dataStream, _In_ ConnectionFlowContext *context) {
	byte nbyte;
	UINT24 handshake_message_length;
	while (context->bytesToRead > 0) { // read all the records we can
		nbyte = nextByte(dataStream, context);
		handshake_message_length = nextUint24(dataStream, context);
		//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Got handshake message type %x, length %x\r\n", nbyte, handshake_message_length);

		// handle different handshake message types
		if (nbyte == TYPE_CLIENT_HELLO) {
			DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Sent a client hello\r\n");
			context->bytesToRead = TH_TLS_RECORD_HEADER_SIZE;
			context->currentState = PS_RECORD_LAYER;
			ffBytes(context, handshake_message_length);
			return RA_CONTINUE; // we don't care about the rest of this record
			// TODO save the client hello
		} else if (nbyte == TYPE_SERVER_HELLO) {
			DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Received a Server Hello\r\n");
			ffBytes(context, handshake_message_length); // fastforward past the server hello
			// if we have read all of this record, lets hop out
			if (context->bytesRead <= (unsigned)(context->recordLength + TH_TLS_RECORD_HEADER_SIZE)) {
				context->bytesToRead = TH_TLS_RECORD_HEADER_SIZE;
				context->currentState = PS_RECORD_LAYER;
				return RA_NEED_MORE;
			}
		} else if (nbyte == TYPE_CERTIFICATE) {
			// get the certificates size
			handshake_message_length = nextUint24(dataStream, context);
			context->bytesToRead = handshake_message_length;
			context->currentState = PS_CERTIFICATE;
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

BOOL stateCanTransition(_In_ FWPS_STREAM_DATA *dataStream, _In_ ConnectionFlowContext *context) {
	//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Can transition? State %x, b2r %x, br %x, dl %x\r\n", context->currentState, context->bytesToRead, context->bytesRead, dataStream->dataLength);
	return ((context->bytesToRead > 0) && (context->bytesRead < dataStream->dataLength) && (context->bytesToRead <= dataStream->dataLength - context->bytesRead));
}

byte peekByte(_In_ FWPS_STREAM_DATA *dataStream, _In_ ConnectionFlowContext *context) {
	byte retbyte = nextByte(dataStream, context);
	context->bytesRead -= 1;
	context->bytesToRead += 1;
	return retbyte;
}


void ffBytes(_In_ ConnectionFlowContext *context, _In_ int amount) {
	context->bytesRead += amount;
}

UINT16 nextUint16(_In_ FWPS_STREAM_DATA *dataStream, _In_ ConnectionFlowContext *context) {
	// TODO: change this to something not intel specific
	UINT16 ret = 0;
	UINT16 piece;
	piece = (UINT16)nextByte(dataStream, context);
	ret = piece << 8;
	ret |= (UINT16)nextByte(dataStream, context);
	return ret;
}

UINT24 nextUint24(_In_ FWPS_STREAM_DATA *dataStream, _In_ ConnectionFlowContext *context) {
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

UINT32 nextUint32(_In_ FWPS_STREAM_DATA *dataStream, _In_ ConnectionFlowContext *context) {
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

byte nextByte(_In_ FWPS_STREAM_DATA *dataStream, _In_ ConnectionFlowContext *context) {
	NET_BUFFER_LIST *currentList;
	NET_BUFFER *currentBuffer;
	byte *data;
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
		currentBuffer = NET_BUFFER_NEXT_NB(currentBuffer);
		if (currentBuffer == NULL) {
			// first try to get the next list
			currentList = NET_BUFFER_LIST_NEXT_NBL(currentList);
			if (currentList == NULL) {
				// reached end, no other lists
				DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Hit a Null list in nextByte %x\r\n", traversed);
				datalen = 0;
			} else {
				// found a new list
				// get next buffer
				currentBuffer = NET_BUFFER_LIST_FIRST_NB(currentList);
				if (currentBuffer == NULL) {
					// reached end anyways
					DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Hit a Null buffer in nextByte %x\r\n", traversed);
					datalen = 0;
				} else {
					// this buffer is good
					datalen = datalen = currentBuffer->DataLength;
				}
			}
		} else {
			datalen = currentBuffer->DataLength;
		}
	}

	// read byte
	if (datalen > 0) {
		data = NdisGetDataBuffer(currentBuffer, 1, NULL, 1, 0);
		retbyte = data[context->bytesRead - traversed];
		context->bytesRead += 1;
		context->bytesToRead -= 1;
	} else {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Error, did not get a byte\r\n");
		retbyte = 0x0;
	}
	return retbyte;
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

	currentList = dataStream->netBufferListChain;
	listcount = 0;

	while (currentList != NULL) {
		currentBuffer = NET_BUFFER_LIST_FIRST_NB(currentList);
		bufcount = 0;

		while (currentBuffer != NULL) {
			data = NdisGetDataBuffer(currentBuffer, 1, NULL, 1, 0);
			datalen = currentBuffer->DataLength;

			DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "List %d Buffer %d len %x\r\n", listcount, bufcount, datalen);
			for (i = 0; i < datalen; i += DEBUGHEXLEN) {
				for (j = 0; j < DEBUGHEXLEN; j++) {
					if (i + j < datalen) {
						RtlStringCchPrintfA(debugout + (j * 3), 4, " %02x\r\n", data[i + j]);
					}
					else {
						break;
					}
				}
				DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "%s\r\n", debugout);
			}
			currentBuffer = NET_BUFFER_NEXT_NB(currentBuffer);
			bufcount++;
		}
		currentList = NET_BUFFER_LIST_NEXT_NBL(currentList);
		listcount++;
	}
	return;	
}