#pragma once


#define NDIS61 1
#include <ntddk.h>
#include <wdf.h>
#define NTSTRSAFE_LIB
#include <Ntstrsafe.h>
#pragma warning(push)
#pragma warning(disable: 4201)	// Disable "Nameless struct/union" compiler warning for fwpsk.h only
#include <fwpsk.h>				// Functions and enumerated types used to implement callouts in kernel mode
#pragma warning(pop)			// Re-enable "Nameless struct/union" compiler warning
#include <fwpvi.h>
#include <fwpmk.h>				// Functions used for managing IKE and AuthIP main mode (MM) policy and security associations
#include "TrustBaseMessage.h"
#include "TrustBaseGuid.h"

// global IO queues
WDFQUEUE TBReadQueue;
WDFQUEUE TBWriteQueue;

// global workitems
WDFWORKITEM  TBReadyReadItem; //Only used in one place, the value isn't changed

//global message queues - the queues are set up using WdfIoQueueDispatchSequential. Thus I/o requests only get taken care of 1 at a time.
TBMessageQueue TBOutputQueue; 
TBResponseTable TBResponses;  

NTSTATUS TbInitQueues(IN WDFDEVICE device);
NTSTATUS TbInitWorkItems(IN WDFDEVICE device);

// Called when the framework receives IRP_MJ_READ requests
VOID TbIoRead(IN WDFQUEUE Queue, IN WDFREQUEST Request, IN size_t Length);
	
// Called when the framework receives IRP_MJ_WRITE
VOID TbIoWrite(IN WDFQUEUE Queue, IN WDFREQUEST Request, IN size_t Length);

// Called when we have recieved a certificate, and can enable the read queue
VOID TBReadyRead(IN WDFWORKITEM WorkItem);