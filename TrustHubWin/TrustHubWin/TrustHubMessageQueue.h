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

#define THMESSAGEQUEUE_SIZE	256

typedef enum THMessageType { TH_CERTIFICATE } THMessageType;

typedef struct THMessage {
	THMessageType type;
	UINT64 processID;
	FWP_BYTE_BLOB processPath;
	UINT32 size;
	UINT8* data;
} THMessage;

typedef struct THMessageQueue {
	UINT16 messageCount;
	THMessage* messages[THMESSAGEQUEUE_SIZE];
	UINT16 newest;
	UINT16 oldest;
	UINT16 messageCapacity;
} THMessageQueue;

NTSTATUS ThInitMessage(OUT THMessage* message, IN THMessageType type, IN UINT64 processID, IN FWP_BYTE_BLOB processPath, IN UINT32 dataSize, IN const UINT8* data);

NTSTATUS ThInitMessageQueue(OUT THMessageQueue* queue);

NTSTATUS ThPushMessage(IN THMessageQueue queue, IN THMessage message);

NTSTATUS ThPopMessage(IN THMessageQueue queue, OUT THMessage* message);