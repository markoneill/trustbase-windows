#pragma once

#define NDIS61 1
#include <Ntddk.h>
#include <wdf.h>
#define NTSTRSAFE_LIB
#include <Ntstrsafe.h>
#pragma warning(push)
#pragma warning(disable: 4201)	// Disable "Nameless struct/union" compiler warning for fwpsk.h only
#include <fwpsk.h>				// Functions and enumerated types used to implement callouts in kernel mode
#pragma warning(pop)

#include "TrustHubGuid.h"

#define THMESSAGEQUEUE_SIZE		256

// we need this, because just doing FIELD_OFFSET is not reliable because padding
#define THMESSAGE_OFFSET_LEN				(0)
#define THMESSAGE_OFFSET_FLOWHANDLE			(THMESSAGE_OFFSET_LEN + sizeof(UINT64))
#define THMESSAGE_OFFSET_PROCESSID			(THMESSAGE_OFFSET_FLOWHANDLE + sizeof(UINT64))
#define THMESSAGE_OFFSET_PROCESSPATHSIZE	(THMESSAGE_OFFSET_PROCESSID + sizeof(UINT64))
#define THMESSAGE_OFFSET_PROCESSPATH		(THMESSAGE_OFFSET_PROCESSPATHSIZE + sizeof(UINT32))

#define THRESPONSE_MESSAGE_SIZE						(sizeof(UINT64)+sizeof(UINT8))

typedef enum THResponseType {RESPONSE_ALLOW=0, RESPONSE_BLOCK=1, WAITING_ON_RESPONSE=7} THResponseType;

typedef struct THMessage {
	UINT64 flowHandle;
	UINT64 processID;
	UINT32 processPathSize;
	UINT8* processPath;
	UINT32 dataSize;
	UINT8* data;
	
	LIST_ENTRY ListEntry;
	size_t bytesWritten;
} THMessage;

typedef struct THResponse {
	UINT64 flowHandle;
	THResponseType response;

	//UINT32 calloutId; // don't need this either
	UINT16 layerId;
	UINT32 streamFlags;
} THResponse;

typedef struct THMessageQueue {
	KSPIN_LOCK lock;
	LIST_ENTRY ListHead;
} THMessageQueue;

typedef RTL_AVL_TABLE THResponseTable;

NTSTATUS ThAddMessage(IN THMessageQueue* queue, IN UINT64 flowHandle, IN UINT64 processID, IN FWP_BYTE_BLOB processPath, IN UINT32 dataSize, IN UINT8* data);

NTSTATUS ThSizeNextMessage(IN THMessageQueue* queue, OUT size_t* len);

VOID ThSizeMessage(IN THMessage* message, OUT size_t* len);

NTSTATUS ThPopMessage(IN THMessageQueue* queue, OUT THMessage** message);

NTSTATUS ThGetMessage(IN THMessageQueue* queue, OUT THMessage** message);

NTSTATUS ThRemMessage(IN THMessageQueue* queue);

NTSTATUS ThFreeMessage(IN THMessage* message);

NTSTATUS ThInitMessageQueue(OUT THMessageQueue* queue);

NTSTATUS ThCopyMessage(IN UINT8* buffer, size_t bufsize, THMessage* message, size_t* bytesWritten);

NTSTATUS ThFinishedMessage(IN THMessage* message);

NTSTATUS ThInitResponseTable(IN THResponseTable* table);

NTSTATUS ThAddResponse(IN THResponseTable* table, IN UINT64 flowHandle, IN UINT16 layerId, IN UINT32 streamFlags);

NTSTATUS ThHandleResponse(IN THResponseTable* table, IN UINT64 flowHandle, IN THResponseType response);

NTSTATUS ThPopResponse(IN THResponseTable* table, IN UINT64 flowHandle, OUT THResponseType* answer);

RTL_GENERIC_COMPARE_RESULTS ThResponseCompare(IN THResponseTable* table, IN PVOID firstStruct, IN PVOID secondStruct);
PVOID ThResponseAllocate(IN THResponseTable* table, IN CLONG byteSize);
VOID ThResponseFree(IN THResponseTable table, IN PVOID buffer);