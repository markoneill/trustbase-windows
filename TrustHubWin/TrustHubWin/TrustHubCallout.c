#include <fwpsk.h>
#include "TrustHubCallout.h"
#include <Ntstrsafe.h>

#define DEBUGHEXLEN 8

static int NTAPI processData(FWPS_STREAM_DATA *dataStream) {
	NET_BUFFER *currentBuffer;
	int bufcount = 0;
	ULONG i, j;
	ULONG datalen = 0;
	byte *data;
	byte *printloc;

	char debugout[] = " __ __ __ __ __ __ __ __";
	
	currentBuffer = NET_BUFFER_LIST_FIRST_NB(dataStream->netBufferListChain);
	while (currentBuffer != NULL) {
		datalen = currentBuffer->DataLength;
		data = NdisGetDataBuffer(currentBuffer, datalen, NULL, 1, 0); // last params are allignment and offset
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Buffer #%d", bufcount);
		for (i = 0; i < datalen; i += DEBUGHEXLEN) {
			printloc = (data + i);
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
	}


	return 0;
}

void NTAPI trusthubCalloutClassify(const FWPS_INCOMING_VALUES * inFixedValues, const FWPS_INCOMING_METADATA_VALUES * inMetaValues, void * layerData, const void * classifyContext, const FWPS_FILTER * filter, UINT64 flowContext, FWPS_CLASSIFY_OUT * classifyOut) {
	FWPS_STREAM_CALLOUT_IO_PACKET *ioPacket;
	FWPS_STREAM_DATA * dataStream;
	UINT32 bytesRequired;
	SIZE_T bytesToPermit;
	SIZE_T bytesToBlock;
	UNREFERENCED_PARAMETER(inFixedValues);
	UNREFERENCED_PARAMETER(inMetaValues);
	UNREFERENCED_PARAMETER(classifyContext);
	UNREFERENCED_PARAMETER(filter);
	UNREFERENCED_PARAMETER(flowContext);

	bytesToBlock = 1024;
	bytesToPermit = 1024;

	DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Classify Called");

	// Find what metadata we have access to
	// What does currentL2MetadataValues do?
	DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Flow handle = %ud", inMetaValues->processId);
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
	processData(dataStream);

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