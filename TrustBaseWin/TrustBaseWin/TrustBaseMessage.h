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

#include "TrustBaseGuid.h"

#define TBMESSAGEQUEUE_SIZE		256

// we need this, because just doing FIELD_OFFSET is not reliable because padding

#define TBMESSAGE_OFFSET_LEN				(0)
#define TBMESSAGE_OFFSET_FLOWHANDLE			(TBMESSAGE_OFFSET_LEN + sizeof(UINT64))
#define TBMESSAGE_OFFSET_PROCESSID			(TBMESSAGE_OFFSET_FLOWHANDLE + sizeof(UINT64))
#define TBMESSAGE_OFFSET_PROCESSPATHSIZE	(TBMESSAGE_OFFSET_PROCESSID + sizeof(UINT64))
#define TBMESSAGE_OFFSET_PROCESSPATH		(TBMESSAGE_OFFSET_PROCESSPATHSIZE + sizeof(UINT32))

#define TBRESPONSE_MESSAGE_SIZE						(sizeof(UINT64)+sizeof(UINT8))

// same as PLUGIN_RESPONSE_VALID, PLUGIN_RESPONSE_VALID
typedef enum TBResponseType {RESPONSE_ALLOW=1, RESPONSE_BLOCK=0, WAITING_ON_RESPONSE=7} TBResponseType;

typedef struct TBMessage {
	UINT64 flowHandle;
	UINT64 processID;
	UINT32 processPathSize;
	UINT8* processPath;
	UINT32 clientHelloSize;
	UINT8* clientHello;
	UINT32 serverHelloSize;
	UINT8* serverHello;
	UINT32 dataSize;
	UINT8* data;
	
	LIST_ENTRY ListEntry;
	size_t bytesWritten;
} TBMessage;

typedef struct TBResponse {
	UINT64 flowHandle;
	TBResponseType response;

	//UINT32 calloutId; // don't need this either
	UINT16 layerId;
	UINT32 streamFlags;
} TBResponse;

typedef struct TBMessageQueue {
	KSPIN_LOCK lock;
	LIST_ENTRY ListHead;
} TBMessageQueue;

typedef RTL_AVL_TABLE TBResponseTable;

NTSTATUS TbMakeMessage(OUT TBMessage** msg, IN UINT64 flowHandle, IN UINT64 processID, IN FWP_BYTE_BLOB processPath);

NTSTATUS TbAddMessage(IN TBMessageQueue* queue, IN TBMessage* msg);

NTSTATUS TbSizeNextMessage(IN TBMessageQueue* queue, OUT size_t* len);

VOID TbSizeMessage(IN TBMessage* message, OUT size_t* len);

NTSTATUS TbPopMessage(IN TBMessageQueue* queue, OUT TBMessage** message);

NTSTATUS TbGetMessage(IN TBMessageQueue* queue, OUT TBMessage** message);

NTSTATUS TbRemMessage(IN TBMessageQueue* queue);

NTSTATUS TbFreeMessage(IN TBMessage* message);

NTSTATUS TbInitMessageQueue(OUT TBMessageQueue* queue);

NTSTATUS TbCopyMessage(IN UINT8* buffer, size_t bufsize, TBMessage* message, size_t* bytesWritten);

NTSTATUS TbFinishedMessage(IN TBMessage* message);

VOID TbPrintMessage(IN TBMessage* message);

NTSTATUS TbInitResponseTable(IN TBResponseTable* table);

NTSTATUS TbAddResponse(IN TBResponseTable* table, IN UINT64 flowHandle, IN UINT16 layerId, IN UINT32 streamFlags);

NTSTATUS TbHandleResponse(IN TBResponseTable* table, IN UINT64 flowHandle, IN TBResponseType response);

NTSTATUS TbPopResponse(IN TBResponseTable* table, IN UINT64 flowHandle, OUT TBResponseType* answer);

RTL_GENERIC_COMPARE_RESULTS TbResponseCompare(IN TBResponseTable* table, IN PVOID firstStruct, IN PVOID secondStruct);
PVOID TbResponseAllocate(IN TBResponseTable* table, IN CLONG byteSize);
VOID TbResponseFree(IN TBResponseTable table, IN PVOID buffer);
ULONG64 getTime(VOID);
VOID queueStats(WDFQUEUE queue);