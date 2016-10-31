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

#define INITGUID
#define NDIS61 1

#include <ntddk.h>
#include <wdf.h>
#pragma warning(push)
#pragma warning(disable: 4201)	// Disable "Nameless struct/union" compiler warning for fwpsk.h only
#include <fwpsk.h>				// Functions and enumerated types used to implement callouts in kernel mode
#pragma warning(pop)			// Re-enable "Nameless struct/union" compiler warning

#include <fwpmk.h>				// Functions used for managing IKE and AuthIP main mode (MM) policy and security associations
#include <fwpvi.h>

#include <guiddef.h>			// Used to define GUID's
#include <initguid.h>			// Used to define GUID's

#include "device.h"
#include "queue.h"
#include "trace.h"
#include "TrustHubGuid.h"
#include "TrustHubCallout.h" 

EXTERN_C_START

//
// WDFDRIVER Events
//

DRIVER_INITIALIZE DriverEntry;
EVT_WDF_OBJECT_CONTEXT_CLEANUP TrustHubWinEvtDriverContextCleanup;

EXTERN_C_END

#endif // TRUSTHUBDRIVER_H