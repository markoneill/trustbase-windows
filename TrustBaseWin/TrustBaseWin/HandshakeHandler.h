#pragma once

#include <Fwpsk.h>
#include "Ndis.h"
#include "ConnectionContext.h"
#include "HandshakeTypes.h"

// calls the appropriate function to parse the data, and updates the state, for data sent
REQUESTED_ACTION updateState(IN FWPS_STREAM_DATA *dataStream, IN ConnectionFlowContext *context);