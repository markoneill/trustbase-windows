#pragma once


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

//DEFINE_GUID (GUID_DEVINTERFACE_TrustBaseWin,
//    0x3d4dc8fe,0x3bde,0x4430,0x93,0xeb,0xd9,0xbd,0xd6,0x05,0xe3,0x4a);
// {3d4dc8fe-3bde-4430-93eb-d9bdd605e34a}


#define LAB_DEVICE_TYPE 0x8888

#ifndef CTL_CODE
#define CTL_CODE( DeviceType, Function, Method, Access ) (                 \
    ((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method) \
)


#endif
#ifndef METHOD_BUFFERED
#define METHOD_BUFFERED                 0
#endif
#ifndef METHOD_IN_DIRECT
#define METHOD_IN_DIRECT                1
#endif
#ifndef METHOD_OUT_DIRECT
#define METHOD_OUT_DIRECT               2
#endif
#ifndef METHOD_NEITHER
#define METHOD_NEITHER                  3
#endif


#define IOCTL_SEND_CERTIFICATE\
	CTL_CODE(LAB_DEVICE_TYPE, 0, METHOD_BUFFERED,\
		 FILE_READ_DATA | FILE_WRITE_DATA)
#define IOCTL_GET_DECISION\
	CTL_CODE(LAB_DEVICE_TYPE, 1, METHOD_BUFFERED,\
		 FILE_READ_DATA | FILE_WRITE_DATA)
#define IOCTL_SHUTDOWN\
	CTL_CODE(LAB_DEVICE_TYPE, 2, METHOD_BUFFERED,\
		 FILE_READ_DATA | FILE_WRITE_DATA)