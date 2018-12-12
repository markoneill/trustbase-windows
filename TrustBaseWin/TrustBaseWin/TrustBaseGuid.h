#pragma once

// cea0131a-6ed3-4ed6-b40c-8a8fe8434b0b
DEFINE_GUID(
	TRUSTBASE_STREAM_CALLOUT_V4,
	0xcea0131a,
	0x6ed3,
	0x4ed6,
	0xb4, 0x0c, 0x8a, 0x8f, 0xe8, 0x43, 0x4b, 0x0b
);

// cea0131a-6ed3-4ed6-fee7-8a8fe843b00b
DEFINE_GUID(
	TRUSTBASE_ALE_CALLOUT_V4,
	0xcea0131a,
	0x6ed3,
	0xfee7,
	0xb4, 0x0c, 0x8a, 0x8f, 0xe8, 0x43, 0xb0, 0x0b
);

// b3241f1d-7cd2-4e7a-8721-2e97d07702ee
DEFINE_GUID(
	TRUSTBASE_SUBLAYER,
	0xb3241f1d,
	0x7cd2,
	0x4e7a,
	0x87, 0x21, 0x2e, 0x97, 0xd0, 0x77, 0x02, 0xee
);

#define TRUSTBASE_DEVICENAME L"\\Device\\TrustBase"
#define TRUSTBASE_SYMNAME L"\\DosDevices\\Global\\TrustBase"

#define TB_POOL_TAG 'bubt'

UINT32 TrustBase_callout_ale_id;
UINT32 TrustBase_callout_stream_id;
HANDLE g_EngineHandle;
WDFDEVICE g_wdm_device;
int cleanup;


