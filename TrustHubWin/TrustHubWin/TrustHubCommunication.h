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
#include "TrustHubMessage.h"


// global IO queues
WDFQUEUE THReadQueue;
WDFQUEUE THWriteQueue;

// global workitems
WDFWORKITEM  THReadyReadItem;

//global message queues
THMessageQueue THOutputQueue;

NTSTATUS ThInitQueues(IN WDFDEVICE device);
NTSTATUS ThInitWorkItems(IN WDFDEVICE device);

// Called when the framework receives IRP_MJ_READ requests
VOID ThIoRead(IN WDFQUEUE Queue, IN WDFREQUEST Request, IN size_t Length);
	
// Called when the framework receives IRP_MJ_WRITE
VOID ThIoWrite(IN WDFQUEUE Queue, IN WDFREQUEST Request, IN size_t Length);

// Called when we have recieved a certificate, and can enable the read queue
VOID THReadyRead(IN WDFWORKITEM WorkItem);