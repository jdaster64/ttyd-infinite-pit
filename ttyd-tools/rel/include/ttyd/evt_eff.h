#pragma once

#include <cstdint>

#include "evtmgr.h"

namespace ttyd::evt_eff {

extern "C" {

// evt_eff_fukidashi
// evt_eff_delete_ptr
// evt_eff_softdelete_ptr
// evt_eff_delete
EVT_DECLARE_USER_FUNC(evt_eff_softdelete, 1)
// evt_eff64
EVT_DECLARE_USER_FUNC(evt_eff, 14)

}

}