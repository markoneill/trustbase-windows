#pragma once

#define NDIS61 1
#include <ntddk.h>
#include <wdf.h>
#define NTSTRSAFE_LIB
#include <Ntstrsafe.h>
#pragma warning(push)
#pragma warning(disable: 4201)	// Disable "Nameless struct/union" compiler warning for fwpsk.h only
#include <fwpsk.h>				// Functions and enumerated types used to implement callouts in kernel mode
#pragma warning(pop)

#include "TrustHubGuid.h"

#define THMESSAGEQUEUE_SIZE	256

typedef enum THMessageType { TH_CERTIFICATE } THMessageType;

typedef struct THMessage {
	THMessageType type;
	UINT64 processID;
	UINT32 processPathSize;
	UINT8* processPath;
	UINT32 dataSize;
	UINT8* data;
	LIST_ENTRY ListEntry;
} THMessage;

typedef struct THMessageQueue {
	KSPIN_LOCK lock;
	LIST_ENTRY ListHead;
} THMessageQueue;

NTSTATUS ThAddMessage(IN THMessageQueue* queue, IN THMessageType type, IN UINT64 processID, IN FWP_BYTE_BLOB processPath, IN UINT32 dataSize, IN UINT8* data);

NTSTATUS ThSizeNextMessage(IN THMessageQueue* queue, OUT size_t* len);

NTSTATUS ThSizeMessage(IN THMessage* message, OUT size_t* len);

NTSTATUS ThPopMessage(IN THMessageQueue* queue, OUT THMessage** message);

NTSTATUS ThFreeMessage(IN THMessage* message);

NTSTATUS ThInitMessageQueue(OUT THMessageQueue* queue);

NTSTATUS ThCopyMessage(IN UINT8* buffer, size_t bufsize, THMessage* message);