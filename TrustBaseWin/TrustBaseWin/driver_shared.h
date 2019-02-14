#ifndef __DRIVER_SHARED__
#define __DRIVER_SHARED__


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

#define IOCTL_GREETINGS_FROM_USER\
   CTL_CODE(LAB_DEVICE_TYPE, 0, METHOD_BUFFERED,\
         FILE_READ_DATA | FILE_WRITE_DATA)
#define IOCTL_PRINT_DRIVER_VERSION\
   CTL_CODE(LAB_DEVICE_TYPE, 1, METHOD_BUFFERED,\
         FILE_READ_DATA | FILE_WRITE_DATA)   

#define IOCTL_SEND_CERTIFICATE\
	CTL_CODE(LAB_DEVICE_TYPE, 2, METHOD_BUFFERED,\
		 FILE_READ_DATA | FILE_WRITE_DATA)
#define IOCTL_GET_DECISION\
	CTL_CODE(LAB_DEVICE_TYPE, 3, METHOD_BUFFERED,\
		 FILE_READ_DATA | FILE_WRITE_DATA)

#endif
