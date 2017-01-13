/*++

Module Name:

    driver.h

Abstract:

    This file contains the driver definitions.

Environment:

    Kernel-mode Driver Framework

--*/
#ifndef TRUSTHUBDRIVER_H
#define TRUSTHUBDRIVER_H

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

#include "queue.h"
#include "TrustHubGuid.h"
#include "TrustHubCallout.h" 
#include "TrustHubCommunication.h"

#define INITGUID
#include <guiddef.h>

EXTERN_C_START

//
// WDFDRIVER Events
//

DRIVER_INITIALIZE DriverEntry;
//EVT_WDF_OBJECT_CONTEXT_CLEANUP DriverUnload;
DRIVER_UNLOAD DriverUnload;
EVT_WDF_DRIVER_UNLOAD empty_evt_unload;

EXTERN_C_END

#endif // TRUSTHUBDRIVER_H