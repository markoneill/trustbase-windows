#pragma once

#include <fwpsk.h>
#include <Ntstrsafe.h>
#include "HandshakeTypes.h"

#define NDIS61 1 // Need to declare this to compile WFP stuff on Win7, I'm not sure why
#pragma warning(push)
#pragma warning(disable: 4201)	// Disable "Nameless struct/union" compiler warning for fwpsk.h only!
#include <fwpsk.h>				// Functions and enumerated types used to implement callouts in kernel mode
#pragma warning(pop)

#define FLOW_CONTEXT_POOL_TAG 'buht'

typedef struct ConnectionFlowContext {
	enum PROCESSING_STATE currentState;
	unsigned bytesRead;
	unsigned bytesToRead;
	UINT16 recordLength;
} ConnectionFlowContext;

ConnectionFlowContext* createConnectionFlowContext(
	_In_ const FWPS_INCOMING_VALUES* inFixedValues,
	_In_ const FWPS_INCOMING_METADATA_VALUES* inMetaValues);
