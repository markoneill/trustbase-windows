#pragma once


#define NDIS61 1
#include <ntddk.h>
#include <wdf.h>
#define NTSTRSAFE_LIB
#pragma warning(push)
#pragma warning(disable: 4201)	// Disable "Nameless struct/union" compiler warning for fwpsk.h only
#include <fwpsk.h>				// Functions and enumerated types used to implement callouts in kernel mode
#pragma warning(pop)			// Re-enable "Nameless struct/union" compiler warning
#include <fwpvi.h>
#include <fwpmk.h>				// Functions used for managing IKE and AuthIP main mode (MM) policy and security associations

// Called when the device receives an IRP_MJ_CREATE request
// TODO
//VOID ThIODeviceFileCreate(IN WDFDEVICE Device, IN WDFREQUEST Request, IN WDFFILEOBJECT FileObject);

// Called when all the handles of the FileObject are closed, and all reperences removed
// TODO
//VOID ThIOEvtFileClose(IN WDFFILEOBJECT FileObject);

// Called when the framework receives IRP_MJ_READ requests
VOID ThIoRead(IN WDFQUEUE Queue, IN WDFREQUEST Request, IN size_t Length);
	
// Called when the framework receives IRP_MJ_WRITE
VOID ThIoWrite(IN WDFQUEUE Queue, IN WDFREQUEST Request, IN size_t Length);

// Called when the framework receives IRP_MJ_DEVICE_CONTROL requests
VOID ThIoDeviceControl(IN WDFQUEUE Queue, IN WDFREQUEST Request, IN size_t OutputBufferLength, IN size_t InputBufferLength, IN ULONG IoControlCode);