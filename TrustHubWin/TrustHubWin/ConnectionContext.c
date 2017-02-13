#include "ConnectionContext.h"
#include "TrustHubGuid.h"

static NTSTATUS allocateConnectionFlowContext(_Out_ ConnectionFlowContext** flowContextOut);

VOID cleanupConnectionFlowContext(_In_ ConnectionFlowContext* context) {
	if (context->processPath.data) {
		ExFreePoolWithTag(context->processPath.data, FLOW_CONTEXT_POOL_TAG);
	}
	ExFreePoolWithTag(context, FLOW_CONTEXT_POOL_TAG);
}

NTSTATUS allocateConnectionFlowContext (_Out_ ConnectionFlowContext** flowContextOut)
{
	NTSTATUS status = STATUS_SUCCESS;
	ConnectionFlowContext* flowContext = NULL;

	*flowContextOut = NULL;

	flowContext = (ConnectionFlowContext*)ExAllocatePoolWithTag(NonPagedPool, sizeof(ConnectionFlowContext), FLOW_CONTEXT_POOL_TAG);

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
			ExFreePoolWithTag(flowContext, FLOW_CONTEXT_POOL_TAG);
		}
	}
	return status;
}

ConnectionFlowContext* createConnectionFlowContext (
	_In_ const FWPS_INCOMING_VALUES* inFixedValues,
	_In_ const FWPS_INCOMING_METADATA_VALUES* inMetaValues)
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
		// Process Path
		if (inMetaValues->currentMetadataValues & FWPS_METADATA_FIELD_PROCESS_PATH) {
			inMetaValues->processPath->data;
			flowContext->processPath.size = inMetaValues->processPath->size;
			// Allocate the data
			flowContext->processPath.data = (UINT8*)ExAllocatePoolWithTag(NonPagedPool, inMetaValues->processPath->size, FLOW_CONTEXT_POOL_TAG);
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
	} else {
		flowContext = NULL;
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Couldn't allocate a context for flow %x\r\n", inMetaValues->flowHandle);
	}

	if (flowContext) {
		NTSTATUS addStatus = FwpsFlowAssociateContext(inMetaValues->flowHandle, FWPS_LAYER_STREAM_V4, TrustHub_callout_id, (UINT64)flowContext);
		if (NT_SUCCESS(addStatus)) {
			DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Added a flow context to flow %x\r\n", inMetaValues->flowHandle);
		} else {
			DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Unable to add a flow context to flow %x\r\n", inMetaValues->flowHandle);
		}
	}


	return flowContext;
}