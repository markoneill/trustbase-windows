#include "ConnectionContext.h"
#include "TrustBaseGuid.h"

static NTSTATUS allocateConnectionFlowContext(OUT ConnectionFlowContext** flowContextOut);

VOID cleanupConnectionFlowContext(IN ConnectionFlowContext* context) {
	// message should be freed from TbIoRead
	TBMessage* message = context->message;
	//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Entered cleanupConnectionFlowContext - Time=%llu\r\n", getTime());

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
	context->message = NULL;
	if (context->processPath.data) {
		ExFreePoolWithTag(context->processPath.data, TB_POOL_TAG);
	}
	ExFreePoolWithTag(context, TB_POOL_TAG);
	//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Exited cleanupConnectionFlowContext - Time=%llu\r\n", getTime());
}


NTSTATUS allocateConnectionFlowContext (OUT ConnectionFlowContext** flowContextOut)
{
	NTSTATUS status = STATUS_SUCCESS;
	ConnectionFlowContext* flowContext = NULL;

	*flowContextOut = NULL;

	flowContext = (ConnectionFlowContext*)ExAllocatePoolWithTag(NonPagedPool, sizeof(ConnectionFlowContext), TB_POOL_TAG);

	if (!flowContext) {
		status = STATUS_NO_MEMORY;
	} else {
		RtlZeroMemory(flowContext,
			sizeof(ConnectionFlowContext));

		*flowContextOut = flowContext;
	}

	if (!NT_SUCCESS(status)) {
		if (flowContext)
		{
			ExFreePoolWithTag(flowContext, TB_POOL_TAG);
		}
	}
	return status;
}

ConnectionFlowContext* createConnectionFlowContext (
	IN const FWPS_INCOMING_VALUES* inFixedValues,
	IN const FWPS_INCOMING_METADATA_VALUES* inMetaValues)
{
	UNREFERENCED_PARAMETER(inFixedValues);

	ConnectionFlowContext* flowContext = NULL;
	NTSTATUS status;


	status = allocateConnectionFlowContext(&flowContext);
	if (NT_SUCCESS(status)) {
		flowContext->currentState = PS_UNKNOWN;
		flowContext->bytesRead = 0;
		flowContext->bytesToRead = 1;
		flowContext->recordLength = 0;
		flowContext->answer = WAITING_ON_RESPONSE;
		// Process Path
		if (inMetaValues->currentMetadataValues & FWPS_METADATA_FIELD_PROCESS_PATH) {
			inMetaValues->processPath->data;
			flowContext->processPath.size = inMetaValues->processPath->size;
			// Allocate the data
			flowContext->processPath.data = (UINT8*)ExAllocatePoolWithTag(NonPagedPool, inMetaValues->processPath->size, TB_POOL_TAG);
			// copy the data
			RtlCopyMemory(flowContext->processPath.data, inMetaValues->processPath->data, inMetaValues->processPath->size);
		} else {
			flowContext->processPath.size = 0;
		}
		// Process Id
		if (inMetaValues->currentMetadataValues & FWPS_METADATA_FIELD_PROCESS_ID) {
			flowContext->processId = inMetaValues->processId;
		} else {
			flowContext->processId = 0;
		}
		// Message
		if (!NT_SUCCESS(TbMakeMessage(&(flowContext->message), inMetaValues->flowHandle, inMetaValues->processId, flowContext->processPath))) {
			DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Couldn't allocate a message in context for flow %x\r\n", inMetaValues->flowHandle);
		}

	} else {
		flowContext = NULL;
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Couldn't allocate a context for flow %x\r\n", inMetaValues->flowHandle);
	}

	if (flowContext) {					//Associate contexts with the TrustBase_callout_stream_id since it will be the one doing things with the context
		NTSTATUS addStatus = FwpsFlowAssociateContext(inMetaValues->flowHandle, FWPS_LAYER_STREAM_V4, TrustBase_callout_stream_id, (UINT64)flowContext);
		if (NT_SUCCESS(addStatus)) {
			//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Added a flow context to flow %x\r\n", inMetaValues->flowHandle);
			contextArray[inMetaValues->flowHandle % MAX_C] = inMetaValues->flowHandle;

		} else {
			DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Unable to add a flow context to flow %x\r\n", inMetaValues->flowHandle);
		}
	}

	return flowContext;
}