#include <fwpsk.h>
#include <Ntstrsafe.h>
#include "HandshakeHandler.h"
#include "ConnectionContext.h"
#include "TrustBaseCallout.h"
#include "TrustBaseCommunication.h"
#include "TrustBaseMessage.h"

// static helper functions
static void NTAPI debugReadStreamFlags(FWPS_STREAM_DATA *dataStream, FWPS_CLASSIFY_OUT *classifyOut);
static NTSTATUS sendCertificate(IN FWPS_STREAM_DATA *dataStream, IN ConnectionFlowContext* context, IN UINT64 flowHandle, IN UINT16 layerId);

void NTAPI trustbaseCalloutClassify(const FWPS_INCOMING_VALUES * inFixedValues, const FWPS_INCOMING_METADATA_VALUES * inMetaValues, void * layerData, const void * classifyContext, const FWPS_FILTER * filter, UINT64 flowContext, FWPS_CLASSIFY_OUT * classifyOut) {
	FWPS_STREAM_CALLOUT_IO_PACKET *ioPacket;
	FWPS_STREAM_DATA *dataStream;
	REQUESTED_ACTION requestedAction;
	ConnectionFlowContext *context;
	UNREFERENCED_PARAMETER(inFixedValues);
	UNREFERENCED_PARAMETER(classifyContext);
	UNREFERENCED_PARAMETER(filter);

	DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "______Classify Called______\r\n");

	// Get any filter context data from filter->context

	// Get any flow context data from flowContext
	if (flowContext == 0) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Error! No context for this flow!\r\n");
		// Set the action to permit
		classifyOut->actionType = FWP_ACTION_PERMIT;
		return;
	}
	// We shouldnt have to check if there is a context or not, because we have the FWP_CALLOUT_FLAG_CONDITIONAL_ON_FLOW;
	context = (ConnectionFlowContext*)flowContext;
	if (context->processPath.size > 0) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Connection with %S (%x)\r\n", context->processPath.data, inMetaValues->flowHandle);
	} else {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Connection with flow %x\r\n", inMetaValues->flowHandle);
	}

	// Find what metadata we have access to
	// What does currentL2MetadataValues do?
	// Inorder to get the pid, we need to intercept at the ALE level

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

	// See if we should already have the answer
	if (context->currentState == PS_DONE) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Got called for PS_DONE\r\n", inMetaValues->processId);
		// find the response in the table
		if (context->answer == WAITING_ON_RESPONSE) {
			if (!NT_SUCCESS(TbPopResponse(&TBResponses, inMetaValues->flowHandle, &(context->answer)))) {
				DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Error! Could not pop the response");
				context->answer = RESPONSE_ALLOW; // Default to allow on error
			}
			DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Read the answer as %x", context->answer);
		}
		if (context->answer == RESPONSE_BLOCK) {
			requestedAction = RA_BLOCK; // stop this connection
		} else {
			requestedAction = RA_NOT_INTERESTED; // allow this connection from here on out
		}
	} else { // Process the incoming data

		// read the data flags
		//debugReadStreamFlags(dataStream, classifyOut);

		// Inspect the various data sources to determine
		// the action to be taken on the data

		// DEBUG
		requestedAction = updateState(dataStream, context);
	}

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
	if (requestedAction == RA_NOT_INTERESTED || requestedAction == RA_ERROR) {
		// Ok, don't process anything else on this stream
		context->answer = RESPONSE_ALLOW;
		context->currentState = PS_DONE;

		// Let the filter engine know we are good
		ioPacket->streamAction = FWPS_STREAM_ACTION_ALLOW_CONNECTION;
		ioPacket->countBytesRequired = 0;
		ioPacket->countBytesEnforced = 0;
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Done with this stream\r\n");
		// Set the action to permit
		classifyOut->actionType = FWP_ACTION_NONE; // need this to permit
		return;
	}

	// we are done with this stream and have decided to block it
	if (requestedAction == RA_BLOCK) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Blocking this stream\r\n");
		ioPacket->streamAction = FWPS_STREAM_ACTION_DROP_CONNECTION;
		classifyOut->actionType = FWP_ACTION_NONE; // need this to drop
		return;
	}

	// if we are at the certificate and need to send it before we decide to allow this stream
	if (requestedAction == RA_WAIT) {
		sendCertificate(dataStream, context, inMetaValues->flowHandle, inFixedValues->layerId);
		context->currentState = PS_DONE; // set this so next time we get called, we finish
		context->answer = WAITING_ON_RESPONSE;
		// Just keep giving us data as it comes in
		ioPacket->streamAction = FWPS_STREAM_ACTION_DEFER;

		ioPacket->countBytesRequired = 0;
		ioPacket->countBytesEnforced = 0;
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Waiting on handle %x\r\n", inMetaValues->flowHandle);
		// Set the action to continue to the next filter
		classifyOut->actionType = FWP_ACTION_NONE;

		// reset bytes read
		context->bytesRead = 0;

		return;
	}
}

NTSTATUS NTAPI trustbaseCalloutNotify(FWPS_CALLOUT_NOTIFY_TYPE notifyType, const GUID * filterKey, const FWPS_FILTER * filter) {
	UNREFERENCED_PARAMETER(filterKey);
	UNREFERENCED_PARAMETER(filter);
	DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Stream Notify Called : Notify Type = %s\r\n", (notifyType==FWPS_CALLOUT_NOTIFY_ADD_FILTER)?"Filter Add":(notifyType==FWPS_CALLOUT_NOTIFY_DELETE_FILTER)?"Filter Delete":"Other");
	return STATUS_SUCCESS;
}

void NTAPI trustbaseCalloutFlowDelete(UINT16 layerId, UINT32 calloutId, UINT64 flowContext) {
	UNREFERENCED_PARAMETER(layerId);
	UNREFERENCED_PARAMETER(calloutId);
	UNREFERENCED_PARAMETER(flowContext);
	DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "FlowDelete Called\r\n");
	cleanupConnectionFlowContext((ConnectionFlowContext*)flowContext);
}


// ALE Callouts

void NTAPI trustbaseALECalloutClassify(const FWPS_INCOMING_VALUES * inFixedValues, const FWPS_INCOMING_METADATA_VALUES * inMetaValues, void * layerData, const void * classifyContext, const FWPS_FILTER * filter, UINT64 flowContext, FWPS_CLASSIFY_OUT * classifyOut) {
	UNREFERENCED_PARAMETER(classifyContext);
	UNREFERENCED_PARAMETER(filter);
	UNREFERENCED_PARAMETER(layerData);
	ConnectionFlowContext *context;

	DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "______ALE Classify Called______\r\n");

	// Find what metadata we have access to
	// What does currentL2MetadataValues do?
	// Get any metadata fields from inMetaValues
	// Process Path
	if (inMetaValues->currentMetadataValues & FWPS_METADATA_FIELD_PROCESS_PATH) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Process Path = %S\r\n", inMetaValues->processPath->data);
	} else {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "No Process Path in metadata\r\n");
	}
	// Process Id
	if (inMetaValues->currentMetadataValues & FWPS_METADATA_FIELD_PROCESS_ID) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Process ID = %x\r\n", inMetaValues->processId);
	} else {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "No Process ID in metadata\r\n");
	}

	// Get the data fields from inFixedValues

	// Get any filter context data from filter->context

	// Get any flow context data from flowContext
	if (flowContext == 0) {
		//create the flow context
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Creating a Connection Context from ALE Callout\r\n");
		context = createConnectionFlowContext(inFixedValues, inMetaValues);
	} else {
		context = (ConnectionFlowContext*)flowContext;
	}
	
	// Set the action to continue with the next filter
	classifyOut->actionType = FWP_ACTION_CONTINUE;
	return;
}

NTSTATUS NTAPI trustbaseALECalloutNotify(FWPS_CALLOUT_NOTIFY_TYPE notifyType, const GUID * filterKey, const FWPS_FILTER * filter) {
	UNREFERENCED_PARAMETER(filterKey);
	UNREFERENCED_PARAMETER(filter);
	DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "ALE Notify Called : Notify Type = %s\r\n", (notifyType == FWPS_CALLOUT_NOTIFY_ADD_FILTER) ? "Filter Add" : (notifyType == FWPS_CALLOUT_NOTIFY_DELETE_FILTER) ? "Filter Delete" : "Other");
	return STATUS_SUCCESS;
}

void NTAPI trustbaseALECalloutFlowDelete(UINT16 layerId, UINT32 calloutId, UINT64 flowContext) {
	UNREFERENCED_PARAMETER(layerId);
	UNREFERENCED_PARAMETER(calloutId);
	UNREFERENCED_PARAMETER(flowContext);
	DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "ALE FlowDelete Called\r\n");
}

NTSTATUS sendCertificate(IN FWPS_STREAM_DATA *dataStream, IN ConnectionFlowContext* context, IN UINT64 flowHandle, IN UINT16 layerId) {
	NTSTATUS status;
	status = STATUS_SUCCESS;

	TbPrintMessage(context->message);

	// create a new TBResponse for this
	TbAddResponse(&TBResponses, flowHandle, layerId, dataStream->flags);

	// put it all in our outgoing message queue
	status = TbAddMessage(&TBOutputQueue, context->message);
	if (!NT_SUCCESS(status)) {
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Had a problem adding the certificate to the message queue\r\n");
		return STATUS_NOT_FOUND;
	}
	// Now that the message is in the queue, make sure we don't keep a reference in the context
	context->message = NULL;

	// Use our workitem to open our read queue
	DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Added query message to the message queue, enabling read queue\r\n");
	WdfWorkItemEnqueue(TBReadyReadItem);

	return status;
}


void NTAPI debugReadStreamFlags(FWPS_STREAM_DATA *dataStream, FWPS_CLASSIFY_OUT *classifyOut) {
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