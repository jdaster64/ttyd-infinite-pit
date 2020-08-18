#pragma once

#include <cstdint>

#include "evtmgr.h"

namespace ttyd::evt_msg {

extern "C" {

EVT_DECLARE_USER_FUNC(unk_US_EU_05_800d0998, 4)
// unk_US_EU_06_800d0a4c
EVT_DECLARE_USER_FUNC(unk_US_EU_07_800d12b0, 4)
// unk_US_EU_08_800d1364
// evt_msg_npc
// evt_msg_pri
// evt_msg_togelr
EVT_DECLARE_USER_FUNC(evt_msg_toge, 4)
EVT_DECLARE_USER_FUNC(evt_msg_select, 2)
// evt_msg_close
// evt_msg_repeat
EVT_DECLARE_USER_FUNC(evt_msg_continue, 0)
// evt_msg_print_add_insert
EVT_DECLARE_USER_FUNC(evt_msg_print_add, 2)
EVT_DECLARE_USER_FUNC(evt_msg_print_insert, -1)
// evt_msg_print_battle_party
// evt_msg_print_party_add
// evt_msg_print_party
EVT_DECLARE_USER_FUNC(evt_msg_print, 4)
// _evt_msg_print
// evt_msg_init

}

}