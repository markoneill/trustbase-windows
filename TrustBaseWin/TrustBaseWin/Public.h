/*++

Module Name:

    public.h

Abstract:

    This module contains the common declarations shared by driver
    and user applications.

Environment:

    user and kernel

--*/

//
// Define an Interface Guid so that app can find the device and talk to it.
//

DEFINE_GUID (GUID_DEVINTERFACE_TrustBaseWin,
    0x3d4dc8fe,0x3bde,0x4430,0x93,0xeb,0xd9,0xbd,0xd6,0x05,0xe3,0x4a);
// {3d4dc8fe-3bde-4430-93eb-d9bdd605e34a}
