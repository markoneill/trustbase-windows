#pragma once

#include <fwpsk.h>
#include <Ntstrsafe.h>
#include "HandshakeTypes.h"
#include "TrustBaseMessage.h"

#define NDIS61 1 // Need to declare this to compile WFP stuff on Win7, I'm not sure why
#pragma warning(push)
#pragma warning(disable: 4201)	// Disable "Nameless struct/union" compiler warning for fwpsk.h only!
#include <fwpsk.h>				// Functions and enumerated types used to implement callouts in kernel mode
#pragma warning(pop)

#define MAX_C 2000   //Max size for arrays keeping track of flow handles


typedef struct ConnectionFlowContext {
	UINT64 processId;
	FWP_BYTE_BLOB processPath;
	enum PROCESSING_STATE currentState;
	unsigned bytesRead;
	unsigned bytesToRead;
	UINT16 recordLength;
	TBMessage* message;
	TBResponseType answer;
} ConnectionFlowContext;

VOID cleanupConnectionFlowContext(IN ConnectionFlowContext* context);

ConnectionFlowContext* createConnectionFlowContext(
	IN const FWPS_INCOMING_VALUES* inFixedValues,
	IN const FWPS_INCOMING_METADATA_VALUES* inMetaValues);

 //If it is global I probably need synconization?
 UINT64 contextArray[MAX_C];
 //int numContexts;
