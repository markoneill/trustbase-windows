/*++

Module Name:

    driver.h

Abstract:

    This file contains the driver definitions.

Environment:

    Kernel-mode Driver Framework

--*/
#ifndef TRUSTBASEDRIVER_H
#define TRUSTBASEDRIVER_H

#define NDIS61 1

#include <ntddk.h>
#include <wdf.h>
#pragma warning(push)
#pragma warning(disable: 4201)	// Disable "Nameless struct/union" compiler warning for fwpsk.h only
#include <fwpsk.h>				// Functions and enumerated types used to implement callouts in kernel mode
#pragma warning(pop)			// Re-enable "Nameless struct/union" compiler warning
#include <fwpvi.h>
#include <fwpmk.h>				// Functions used for managing IKE and AuthIP main mode (MM) policy and security associations

#include <ws2ipdef.h>
#include <in6addr.h>
#include <ip2string.h>

#include <guiddef.h>			// Used to define GUID's
#include <initguid.h>			// Used to define GUID's

#include "TrustBaseGuid.h"
#include "TrustBaseCallout.h" 
#include "TrustBaseCommunication.h"
#include "ConnectionContext.h"
#include "Public.h"

#define INITGUID
#include <guiddef.h>


/*
#define IOCTL_GREETINGS_FROM_USER\
   CTL_CODE(LAB_DEVICE_TYPE, 0, METHOD_BUFFERED,\
         FILE_READ_DATA | FILE_WRITE_DATA)
#define IOCTL_PRINT_DRIVER_VERSION\
   CTL_CODE(LAB_DEVICE_TYPE, 1, METHOD_BUFFERED,\
         FILE_READ_DATA | FILE_WRITE_DATA)   
*/



EXTERN_C_START

//
// WDFDRIVER Events
//

DRIVER_INITIALIZE DriverEntry;
//EVT_WDF_OBJECT_CONTEXT_CLEANUP DriverUnload;
//DRIVER_UNLOAD DriverUnload;
EVT_WDF_DRIVER_UNLOAD empty_evt_unload;
UINT64 alefilterId;
UINT64 streamfilterId;
UINT32 aleCallId;
UINT32 streamCallId;


EXTERN_C_END

#endif // TRUSTBASEDRIVER_H