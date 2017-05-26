#pragma once

// cea0131a-6ed3-4ed6-b40c-8a8fe8434b0b
DEFINE_GUID(
	TRUSTHUB_STREAM_CALLOUT_V4,
	0xcea0131a,
	0x6ed3,
	0x4ed6,
	0xb4, 0x0c, 0x8a, 0x8f, 0xe8, 0x43, 0x4b, 0x0f
);

// cea0131a-6ed3-4ed6-fee7-8a8fe843b00b
DEFINE_GUID(
	TRUSTHUB_ALE_CALLOUT_V4,
	0xcea0131a,
	0x6ed3,
	0xfee7,
	0xb4, 0x0c, 0x8a, 0x8f, 0xe8, 0x43, 0xb0, 0x0c
);

// b3241f1d-7cd2-4e7a-8721-2e97d07702ee
DEFINE_GUID(
	TRUSTHUB_SUBLAYER,
	0xb3241f1d,
	0x7cd2,
	0x4e7a,
	0x87, 0x21, 0x2e, 0x97, 0xd0, 0x77, 0x02, 0xef
);

#define TRUSTHUB_DEVICENAME L"\\Device\\TrustHub"
#define TRUSTHUB_SYMNAME L"\\DosDevices\\Global\\TrustHub"

#define TH_POOL_TAG 'buht'

UINT32 TrustHub_callout_id;