#ifndef TRUSTBASECALLOUT_H
#define TRUSTBASECALLOUT_H

#ifndef _HRESULT_DEFINED
#define _HRESULT_DEFINED
#ifdef __midl
typedef LONG HRESULT;
#else
typedef _Return_type_success_(return >= 0) long HRESULT;
#endif // __midl
#endif // !_HRESULT_DEFINED

//
// ULONGLONG -> ULONG_PTR conversion
//
#ifdef _WIN64
_Must_inspect_result_
__inline
HRESULT
ULongLongToULongPtr(
	IN ULONGLONG ullOperand,
	OUT _Deref_out_range_(== , ullOperand) ULONG_PTR* pulResult)
{
	*pulResult = ullOperand;
	return STATUS_SUCCESS;
}
#else
#define ULongLongToULongPtr ULongLongToULong
#endif

#include <Wdf.h>
#pragma warning(push)
#pragma warning(disable: 4201)	// Disable "Nameless struct/union" compiler warning for fwpsk.h only
#include <fwpsk.h>				// Functions and enumerated types used to implement callouts in kernel mode
#pragma warning(pop)			// Re-enable "Nameless struct/union" compiler warning

// STREAM Callouts
void NTAPI trustbaseCalloutClassify(
	const FWPS_INCOMING_VALUES * inFixedValues,
	const FWPS_INCOMING_METADATA_VALUES * inMetaValues,
	void * layerData,
	const void * classifyContext,
	const FWPS_FILTER * filter,
	UINT64 flowContext,
	FWPS_CLASSIFY_OUT * classifyOut);

NTSTATUS NTAPI trustbaseCalloutNotify(
	FWPS_CALLOUT_NOTIFY_TYPE notifyType,
	const GUID * filterKey,
	const FWPS_FILTER * filter);

void NTAPI trustbaseCalloutFlowDelete(
	UINT16 layerId,
	UINT32 calloutId,
	UINT64 flowContext);

// ALE Callouts
void NTAPI trustbaseALECalloutClassify(
	const FWPS_INCOMING_VALUES * inFixedValues,
	const FWPS_INCOMING_METADATA_VALUES * inMetaValues,
	void * layerData,
	const void * classifyContext,
	const FWPS_FILTER * filter,
	UINT64 flowContext,
	FWPS_CLASSIFY_OUT * classifyOut);

NTSTATUS NTAPI trustbaseALECalloutNotify(
	FWPS_CALLOUT_NOTIFY_TYPE notifyType,
	const GUID * filterKey,
	const FWPS_FILTER * filter);

void NTAPI trustbaseALECalloutFlowDelete(
	UINT16 layerId,
	UINT32 calloutId,
	UINT64 flowContext);

#endif // TRUSTBASECALLOUT_H