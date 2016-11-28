#include <fwpsk.h>
#include "TrustHubCallout.h"
#include <Ntstrsafe.h>

#define TLS_TYPE_HANDSHAKE 0x16
#define TLS_TYPE_CERTIFICATE 0x0b

typedef enum REQUESTED_ACTION { RA_FOUND_CERT, RA_NEED_MORE, RA_NOT_INTERESTED, RA_ERROR } REQUESTED_ACTION;
typedef enum PROCESSING_STATE { PS_RECORD_TYPE, PS_RECORD_VERSION, PS_RECORD_LENGTH, PS_HANDSHAKE_TYPE, PS_HANDSHAKE_LENGTH, PS_CERT_LENGTH, PS_FULL_CERT, PS_DONE } PROCESSING_STATE;
#define PS_RECORD_TYPE_LEN 1;
#define PS_RECORD_VERSION_LEN 2;
#define PS_RECORD_LENGTH_LEN 2;
#define PS_HANDSHAKE_TYPE_LEN 1;
#define PS_HANDSHAKE_LENGTH_LEN 3;
#define PS_CERT_LENGTH_LEN 3;


#define DEBUGHEXLEN 8

static REQUESTED_ACTION NTAPI processData(FWPS_STREAM_DATA *dataStream) {
	NET_BUFFER *currentBuffer;
	ULONG datalen = 0;
	byte *data;
	byte *datap;
	UINT32 bytes_left = 0;
	PROCESSING_STATE ps;

	UINT16 version;
	UINT32 length = 0;
	//byte* certificate;

	//int bufcount = 0;
	//ULONG i, j;
	// Debug print
	/*char debugout[] = " __ __ __ __ __ __ __ __";
	currentBuffer = NET_BUFFER_LIST_FIRST_NB(dataStream->netBufferListChain);
	while (currentBuffer != NULL) {
		data = NdisGetDataBuffer(currentBuffer, 1, NULL, 1, 0);
		datalen = currentBuffer->DataLength;

		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Buffer #%d", bufcount);
		for (i = 0; i < datalen; i += DEBUGHEXLEN) {
			for (j = 0; j < DEBUGHEXLEN; j++) {
				if (i + j < datalen) {
					RtlStringCchPrintfA(debugout + (j * 3), 4, " %02x", data[i + j]);
				} else {
					break;
				}
			}
			DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "%s", debugout);
		}
		currentBuffer = NET_BUFFER_NEXT_NB(currentBuffer);
		bufcount++;
	}*/

	ps = PS_RECORD_TYPE;
	bytes_left = PS_RECORD_TYPE_LEN;
	currentBuffer = NET_BUFFER_LIST_FIRST_NB(dataStream->netBufferListChain);
	if (currentBuffer == NULL) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Error, expected data, but got NULL");
		return RA_ERROR;
	}
	data = NdisGetDataBuffer(currentBuffer, 1, NULL, 1, 0);
	if (data == NULL) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Error, expected data, but got an empty buffer");
		return RA_ERROR;
	}
	datalen = currentBuffer->DataLength;
	datap = data;
	while (ps != PS_DONE) {
		// check if there is a byte at left at datap;
		if (datalen - (datap - data) <= 0) {
			DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Continuing to next buffer");
			// get next buffer
			currentBuffer = NET_BUFFER_NEXT_NB(currentBuffer);
			if (currentBuffer == NULL) {
				DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Error, expected more data, but got end of linked list");
				return RA_ERROR;
			}
			data = NdisGetDataBuffer(currentBuffer, 1, NULL, 1, 0);
			if (data == NULL) {
				DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Error, expected more data, but got an empty buffer");
				return RA_ERROR;
			}
			datalen = currentBuffer->DataLength;
			datap = data;
		}

		switch (ps) {
		case PS_RECORD_TYPE:
			// check that we have enough in this buffer
			DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Record Type %x", *(datap));
			if (*datap == TLS_TYPE_HANDSHAKE) {
				// next state
				datap += 1;
				ps = PS_RECORD_VERSION;
				bytes_left = PS_RECORD_VERSION_LEN;
			} else {
				DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Not a handshake message, exiting");
				return RA_NOT_INTERESTED;
			}
			break;
		case PS_RECORD_VERSION:
			if (bytes_left > 0) {
				version |= (*datap) << ((8)*(bytes_left - 1));
				bytes_left -= 1;
				datap += 1;
			} else {
				// print out the version
				DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "TLS version :%04x", version);
				// next state
				ps = PS_RECORD_LENGTH;
				bytes_left = PS_RECORD_LENGTH_LEN;
			}
			break;
		case PS_RECORD_LENGTH:
			if (bytes_left > 0) {
				length |= (*datap) << ((8)*(bytes_left - 1));
				bytes_left -= 1;
				datap += 1;
			}
			else {
				// print out the length
				DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Record length :%04x", (UINT16)length);
				// next state
				ps = PS_HANDSHAKE_TYPE;
				bytes_left = PS_HANDSHAKE_TYPE_LEN;
			}
			break;
		case PS_HANDSHAKE_TYPE:
			DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Handshake Type %x", *(datap));
			if (*datap == TLS_TYPE_CERTIFICATE) {
				// next state
				ps = PS_HANDSHAKE_LENGTH;
				bytes_left = PS_HANDSHAKE_LENGTH_LEN;
				datap += 1;
			} else {
				DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Not a certificate message, exiting");
				return RA_NOT_INTERESTED;
			}
			break;
		case PS_HANDSHAKE_LENGTH:
			if (bytes_left > 0) {
				length |= (*datap) << ((8)*(bytes_left - 1));
				bytes_left -= 1;
				datap += 1;
			}
			else {
				// print out the length
				DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Handshake length :%06x", length);
				// next state
				ps = PS_CERT_LENGTH;
				bytes_left = PS_CERT_LENGTH_LEN;
			}
			break;
		case PS_CERT_LENGTH:
			if (bytes_left > 0) {
				length |= (*datap) << ((8)*(bytes_left - 1));
				bytes_left -= 1;
				datap += 1;
			}
			else {
				// print out the length
				DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Cert length :%06x", length);
				// next state
				//ps = PS_FULL_CERT;
				ps = PS_DONE;
				bytes_left = length;
			}
			break;
		}
	}


	return RA_FOUND_CERT;
}

void NTAPI trusthubCalloutClassify(const FWPS_INCOMING_VALUES * inFixedValues, const FWPS_INCOMING_METADATA_VALUES * inMetaValues, void * layerData, const void * classifyContext, const FWPS_FILTER * filter, UINT64 flowContext, FWPS_CLASSIFY_OUT * classifyOut) {
	FWPS_STREAM_CALLOUT_IO_PACKET *ioPacket;
	FWPS_STREAM_DATA * dataStream;
	UINT32 bytesRequired;
	SIZE_T bytesToPermit;
	SIZE_T bytesToBlock;
	REQUESTED_ACTION requestedAction;
	UNREFERENCED_PARAMETER(inFixedValues);
	UNREFERENCED_PARAMETER(classifyContext);
	UNREFERENCED_PARAMETER(filter);
	UNREFERENCED_PARAMETER(flowContext);

	bytesToBlock = 1024;
	bytesToPermit = 1024;

	DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Classify Called");

	// Find what metadata we have access to
	// What does currentL2MetadataValues do?
	// Inorder to get the pid, we need to intercept at the ALE level
	if ((inMetaValues->currentMetadataValues & FWPS_METADATA_FIELD_FLOW_HANDLE) == FWPS_METADATA_FIELD_FLOW_HANDLE) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Flow handle = %ud", inMetaValues->flowHandle);
	} else {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "No flow handler in metadata");
	}
	if ((inMetaValues->currentMetadataValues & FWPS_METADATA_FIELD_PROCESS_ID) == FWPS_METADATA_FIELD_PROCESS_ID) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Process Id = %ud", inMetaValues->processId);
	} else {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "No process Id in metadata");
	}
	if ((inMetaValues->currentMetadataValues & FWPS_METADATA_FIELD_PROCESS_PATH) == FWPS_METADATA_FIELD_PROCESS_PATH) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Process Id = %s", inMetaValues->processPath);
	} else {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "No process path in metadata");
	}

	// Get a pointer to the stream callout I/O packet
	ioPacket = (FWPS_STREAM_CALLOUT_IO_PACKET0 *)layerData;

	// Get the data fields from inFixedValues

	// Get any metadata fields from inMetaValues

	// Get the pointer to the data stream
	dataStream = ioPacket->streamData;
	if (dataStream == NULL) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Data stream is null", inMetaValues->processId);
		ioPacket->streamAction = FWPS_STREAM_ACTION_NONE;
		return;
	}

	// Check connection direction
	DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, ((dataStream->flags & FWPS_STREAM_FLAG_SEND) == FWPS_STREAM_FLAG_SEND) ? "Outbound data":"Inbound data");

	// Get any filter context data from filter->context

	// Get any flow context data from flowContext

	// Inspect the various data sources to determine
	// the action to be taken on the data
	requestedAction = processData(dataStream);

	// If more stream data is required to make a determination...
	if (FALSE) {
		// Let the filter engine know how many more bytes are needed
		ioPacket->streamAction = FWPS_STREAM_ACTION_NEED_MORE_DATA;
		ioPacket->countBytesRequired = bytesRequired;
		ioPacket->countBytesEnforced = 0;

		// Set the action to continue to the next filter
		classifyOut->actionType = FWP_ACTION_CONTINUE;

		return;
	}
	

	// If some or all of the data should be permitted...
	if (TRUE) {
		// No stream-specific action is required
		ioPacket->streamAction = FWPS_STREAM_ACTION_ALLOW_CONNECTION;

		// Let the filter engine know how many of the leading bytes
		// in the stream should be permitted
		ioPacket->countBytesRequired = 0;
		ioPacket->countBytesEnforced = bytesToPermit;

		// Set the action to permit the data
		classifyOut->actionType = FWP_ACTION_PERMIT;

		return;
	}


	// If some or all of the data should be blocked...
	if (FALSE) {
		// No stream-specific action is required
		ioPacket->streamAction = FWPS_STREAM_ACTION_NONE;

		// Let the filter engine know how many of the leading bytes
		// in the stream should be blocked
		ioPacket->countBytesRequired = 0;
		ioPacket->countBytesEnforced = bytesToBlock;

		// Set the action to block the data
		classifyOut->actionType = FWP_ACTION_BLOCK;

		return;
	}
}

NTSTATUS NTAPI trusthubCalloutNotify(FWPS_CALLOUT_NOTIFY_TYPE notifyType, const GUID * filterKey, const FWPS_FILTER * filter) {
	UNREFERENCED_PARAMETER(filterKey);
	UNREFERENCED_PARAMETER(filter);
	DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Notify Called");
	DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Notify Type = %s", (notifyType==FWPS_CALLOUT_NOTIFY_ADD_FILTER)?"Filter Add":(notifyType==FWPS_CALLOUT_NOTIFY_DELETE_FILTER)?"Filter Delete":"Other");
	return STATUS_SUCCESS;
}

void NTAPI trusthubCalloutFlowDelete(UINT16 layerId, UINT32 calloutId, UINT64 flowContext) {
	UNREFERENCED_PARAMETER(layerId);
	UNREFERENCED_PARAMETER(calloutId);
	UNREFERENCED_PARAMETER(flowContext);
	DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "FlowDelete Called");
}