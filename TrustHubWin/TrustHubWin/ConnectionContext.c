#include "ConnectionContext.h"
#include "TrustHubGuid.h"

static NTSTATUS allocateConnectionFlowContext(_Out_ ConnectionFlowContext** flowContextOut);

NTSTATUS allocateConnectionFlowContext (_Out_ ConnectionFlowContext** flowContextOut)
{
	NTSTATUS status = STATUS_SUCCESS;
	ConnectionFlowContext* flowContext = NULL;

	*flowContextOut = NULL;

	flowContext = (ConnectionFlowContext*)ExAllocatePoolWithTag(NonPagedPool,
		sizeof(ConnectionFlowContext),
		FLOW_CONTEXT_POOL_TAG);

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