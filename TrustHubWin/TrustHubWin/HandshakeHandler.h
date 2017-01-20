#pragma once

#include <Fwpsk.h>
#include "ConnectionContext.h"
#include "HandshakeTypes.h"

// calls the appropriate function to parse the data, and updates the state, for data sent
REQUESTED_ACTION updateState(_In_ FWPS_STREAM_DATA *dataStream, _In_ ConnectionFlowContext *context);

