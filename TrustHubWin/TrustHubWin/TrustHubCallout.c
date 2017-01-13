#include <fwpsk.h>
#include <Ntstrsafe.h>
#include "HandshakeHandler.h"
#include "ConnectionContext.h"
#include "TrustHubCallout.h"

// static helper functions
static void NTAPI debugReadFlags(FWPS_STREAM_DATA *dataStream, FWPS_CLASSIFY_OUT *classifyOut);

void NTAPI trusthubCalloutClassify(const FWPS_INCOMING_VALUES * inFixedValues, const FWPS_INCOMING_METADATA_VALUES * inMetaValues, void * layerData, const void * classifyContext, const FWPS_FILTER * filter, UINT64 flowContext, FWPS_CLASSIFY_OUT * classifyOut) {
	FWPS_STREAM_CALLOUT_IO_PACKET *ioPacket;
	FWPS_STREAM_DATA *dataStream;
	REQUESTED_ACTION requestedAction;
	ConnectionFlowContext *context;
	UNREFERENCED_PARAMETER(classifyContext);
	UNREFERENCED_PARAMETER(filter);

	DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "______Classify Called______\r\n");

	// Find what metadata we have access to
	// What does currentL2MetadataValues do?
	// Inorder to get the pid, we need to intercept at the ALE level
	if ((inMetaValues->currentMetadataValues & FWPS_METADATA_FIELD_FLOW_HANDLE) == FWPS_METADATA_FIELD_FLOW_HANDLE) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Flow handle = %x\r\n", inMetaValues->flowHandle);
	} else {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "No flow handler in metadata\r\n");
	}

	// Get a pointer to the stream callout I/O packet
	ioPacket = (FWPS_STREAM_CALLOUT_IO_PACKET0 *)layerData;

	// Get the data fields from inFixedValues

	// Get any metadata fields from inMetaValues

	// Get the pointer to the data stream
	dataStream = ioPacket->streamData;
	if (dataStream == NULL) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Data stream is null\r\n", inMetaValues->processId);
		ioPacket->streamAction = FWPS_STREAM_ACTION_NONE;
		return;
	}

	// read the data flags
	debugReadFlags(dataStream, classifyOut);

	// Get any filter context data from filter->context

	// Get any flow context data from flowContext
	if (flowContext == 0) {
		//create the flow context
		context = createConnectionFlowContext(inFixedValues, inMetaValues);
	} else {
		context = (ConnectionFlowContext*)flowContext;
	}

	// Inspect the various data sources to determine
	// the action to be taken on the data

	requestedAction = updateState(dataStream, context);

	// see if we can grab certificate

	// if we are done with this data, and just care about what is coming
	if (requestedAction == RA_CONTINUE) {
		// Just keep giving us data as it comes in
		ioPacket->streamAction = FWPS_STREAM_ACTION_NONE;
		ioPacket->countBytesRequired = 0;
		//ioPacket->countBytesEnforced = dataStream->dataLength;
		ioPacket->countBytesEnforced = context->bytesRead;
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Continuing: req %x, read %x\r\n", ioPacket->countBytesRequired, ioPacket->countBytesEnforced);
		// Set the action to continue to the next filter
		classifyOut->actionType = FWP_ACTION_CONTINUE;

		// reset bytes read
		context->bytesRead = 0;

		return;
	}

	// If more stream data is required to make a determination
	if (requestedAction == RA_NEED_MORE) {
		// Let the filter engine know how many more bytes are needed
		ioPacket->streamAction = FWPS_STREAM_ACTION_NEED_MORE_DATA;
		ioPacket->countBytesRequired = context->bytesToRead;
		ioPacket->countBytesEnforced = context->bytesRead;
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Waiting for more: req %x, read %x\r\n", ioPacket->countBytesRequired, ioPacket->countBytesEnforced);
		// An action of NONE is required for NEED_MORE_DATA stuff
		classifyOut->actionType = FWP_ACTION_NONE;

		// don't reset bytes read

		return;
	}

	// if we are done with this stream all together
	if (requestedAction == RA_NOT_INTERESTED || requestedAction == RA_ERROR || requestedAction == RA_WAIT) {
		// Let the filter engine know we are good
		ioPacket->streamAction = FWPS_STREAM_ACTION_ALLOW_CONNECTION;
		ioPacket->countBytesRequired = 0;
		ioPacket->countBytesEnforced = 0;
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Done with this stream\r\n");
		// Set the action to permit
		classifyOut->actionType = FWP_ACTION_PERMIT;

		return;
	}

	// if we are at the certificate and need to send it before we decide to allow this stream
	if (requestedAction == RA_WAIT) {
		// Just keep giving us data as it comes in
		ioPacket->streamAction = FWPS_STREAM_ACTION_DEFER;
		// apperantly defer doesn't work any more, so we have to do stupid stuff on our own
		// https://social.msdn.microsoft.com/Forums/windowsdesktop/en-US/ab3bf8a9-834d-4ef7-b825-84ec290c21f1/filtering-tcp-outofband

		ioPacket->countBytesRequired = 0;
		ioPacket->countBytesEnforced = 0;
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Continuing: req %x, read %x\r\n", ioPacket->countBytesRequired, ioPacket->countBytesEnforced);
		// Set the action to continue to the next filter
		classifyOut->actionType = FWP_ACTION_NONE;

		// reset bytes read
		context->bytesRead = 0;

		return;
	}
}

NTSTATUS NTAPI trusthubCalloutNotify(FWPS_CALLOUT_NOTIFY_TYPE notifyType, const GUID * filterKey, const FWPS_FILTER * filter) {
	UNREFERENCED_PARAMETER(filterKey);
	UNREFERENCED_PARAMETER(filter);
	DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Notify Called\r\n");
	DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Notify Type = %s\r\n", (notifyType==FWPS_CALLOUT_NOTIFY_ADD_FILTER)?"Filter Add":(notifyType==FWPS_CALLOUT_NOTIFY_DELETE_FILTER)?"Filter Delete":"Other");
	return STATUS_SUCCESS;
}

void NTAPI trusthubCalloutFlowDelete(UINT16 layerId, UINT32 calloutId, UINT64 flowContext) {
	UNREFERENCED_PARAMETER(layerId);
	UNREFERENCED_PARAMETER(calloutId);
	UNREFERENCED_PARAMETER(flowContext);
	DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "FlowDelete Called\r\n");
}

void NTAPI debugReadFlags(FWPS_STREAM_DATA *dataStream, FWPS_CLASSIFY_OUT *classifyOut) {
	if (classifyOut->flags & FWPS_CLASSIFY_OUT_FLAG_BUFFER_LIMIT_REACHED) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "BADBADNOTGOOD! REACHED BUFFER LIMIT\r\n");
	}
	if (dataStream->flags & FWPS_STREAM_FLAG_RECEIVE) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Inbound data\r\n");
	}
	if (dataStream->flags & FWPS_STREAM_FLAG_RECEIVE_EXPEDITED) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "In Expedited data\r\n");
	}
	if (dataStream->flags & FWPS_STREAM_FLAG_RECEIVE_DISCONNECT) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "In Disconnect data\r\n");
	}
	if (dataStream->flags & FWPS_STREAM_FLAG_RECEIVE_ABORT) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "In Abort data\r\n");
	}
	if (dataStream->flags & FWPS_STREAM_FLAG_SEND) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Outbound data\r\n");
	}
	if (dataStream->flags & FWPS_STREAM_FLAG_SEND_EXPEDITED) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Out Expedited data\r\n");
	}
	if (dataStream->flags & FWPS_STREAM_FLAG_SEND_DISCONNECT) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Out Disconnect data\r\n");
	}
	if (dataStream->flags & FWPS_STREAM_FLAG_SEND_ABORT) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Out Abort data\r\n");
	}
	if (dataStream->flags & FWPS_STREAM_FLAG_SEND_NODELAY) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Out NoDelay data\r\n");
	}
}