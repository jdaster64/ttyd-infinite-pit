#pragma once

#include "evt_cmd.h"

#include <ttyd/evtmgr.h>

#include <cstdint>

namespace mod::infinite_pit::partner {

// Apply patches related to partner balance changes, etc.
void ApplyFixedPatches();

// Initializes + fully heals the selected party member.
EVT_DECLARE_USER_FUNC(InitializePartyMember, 1)

}