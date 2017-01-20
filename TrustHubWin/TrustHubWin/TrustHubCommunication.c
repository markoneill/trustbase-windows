#include "TrustHubCommunication.h"

// Called when the framework receives IRP_MJ_READ requests
VOID ThIoRead(IN WDFQUEUE Queue, IN WDFREQUEST Request, IN size_t Length) {
	UNREFERENCED_PARAMETER(Queue);
	UNREFERENCED_PARAMETER(Request);
	UNREFERENCED_PARAMETER(Length);
	PAGED_CODE();

	DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "File Read Called\r\n");
}

// Called when the framework receives IRP_MJ_WRITE
VOID ThIoWrite(IN WDFQUEUE Queue, IN WDFREQUEST Request, IN size_t Length) {
	UNREFERENCED_PARAMETER(Queue);
	UNREFERENCED_PARAMETER(Request);
	UNREFERENCED_PARAMETER(Length);
	PAGED_CODE();

	DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "File Write Called\r\n");
}

// Called when the framework receives IRP_MJ_DEVICE_CONTROL requests
VOID ThIoDeviceControl(IN WDFQUEUE Queue, IN WDFREQUEST Request, IN size_t OutputBufferLength, IN size_t InputBufferLength, IN ULONG IoControlCode) {
	UNREFERENCED_PARAMETER(Queue);
	UNREFERENCED_PARAMETER(Request);
	UNREFERENCED_PARAMETER(OutputBufferLength);
	UNREFERENCED_PARAMETER(InputBufferLength);
	UNREFERENCED_PARAMETER(IoControlCode);
	PAGED_CODE();

	DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "File Control Called\r\n");
}