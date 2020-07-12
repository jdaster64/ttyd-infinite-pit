#pragma once

#include <cstdint>

#include "evtmgr.h"

namespace ttyd::evt_msg {

extern "C" {

// evt_msg_npc
// evt_msg_pri
// evt_msg_togelr
EVT_DECLARE_USER_FUNC(evt_msg_toge, 4)
// evt_msg_select
// evt_msg_close
// evt_msg_repeat
// evt_msg_continue
// evt_msg_print_add_insert
// evt_msg_print_add
EVT_DECLARE_USER_FUNC(evt_msg_print_insert, -1)
// evt_msg_print_battle_party
// evt_msg_print_party_add
// evt_msg_print_party
EVT_DECLARE_USER_FUNC(evt_msg_print, 4)
// _evt_msg_print
// evt_msg_init

}

}