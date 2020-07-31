#pragma once

#include <cstdint>

#include "evtmgr.h"

namespace ttyd::evt_bero {

extern "C" {
    
struct BeroEntry {
    const char* name;
    uint32_t    unk_04;
    int32_t     unk_08;
    int32_t     unk_0c;
    int32_t     unk_10;
    int32_t     unk_14;
    int32_t     unk_18;
    void*       entry_evt_code;         // default_entevt
    int32_t     unk_20;
    void*       out_evt_code;           // default_outevt
    const char* target_map;
    const char* target_bero;
    int16_t     unk_30;
    int16_t     unk_32;
    void*       unk_close_evt_code;
    void*       unk_open_evt_code;
} __attribute__((__packed__));

static_assert(sizeof(BeroEntry) == 0x3c);

// evt_camera_change_event_from_road
// evt_bero_1stcheck
// evt_bero_mario_go_wait
// evt_bero_mario_go
// evt_bero_mario_go_init
// evt_bero_cam_shift_init
// evt_bero_cam3d_change
// evt_bero_overwrite
// evt_bero_move_mario_speed
// evt_bero_disppos_load
// evt_bero_disppos_save
// evt_bero_case_id_load
// evt_bero_case_id_save
// evt_bero_get_number
// evt_bero_switch_off
// evt_bero_switch_on
// bero_set_disp_position_pipe
// bero_get_position_pipe2
// bero_get_position_pipe_pure
// bero_get_position_pipe_sub
// bero_get_position_normal
// evt_bero_get_info_num
// evt_bero_id_filter
// evt_bero_read_mario_pera
// evt_bero_get_into_info
// evt_bero_get_info
// evt_bero_get_now_number
// evt_bero_set_now_number
// evt_bero_get_info_nextarea
// evt_bero_get_info_kinddir
// evt_bero_get_info_length
// evt_bero_get_info_anime
// evt_bero_get_end_position
// evt_bero_get_start_position
EVT_DECLARE_USER_FUNC(evt_bero_exec_wait, 1)
// evt_bero_exec_get
EVT_DECLARE_USER_FUNC(evt_bero_exec_onoff, 2)
// evt_bero_get_entername
// evt_bero_mapchange
// bero_id_filter
// bero_clear_Offset
// bero_get_ptr
// bero_get_BeroEZ
// bero_get_BeroEY
// bero_get_BeroEX
// bero_get_BeroSZ
// bero_get_BeroSY
// bero_get_BeroSX
// bero_get_BeroNUM
// bero_get_BeroEXEC
// N_evt_bero_set_mario_bottomless_respawn_pos_to_current_mario_pos
// N_evt_bero_set_mario_bottomless_respawn_pos_on_bero_entry

}

}