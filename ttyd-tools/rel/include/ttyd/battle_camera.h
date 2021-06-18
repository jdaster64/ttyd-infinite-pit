#pragma once

#include "evtmgr.h"

#include <cstdint>

namespace ttyd::battle_camera {

extern "C" {

// evt_btl_camera_wait_move_end
// evt_btl_camera_off_posoffset_manual
// btl_camera_off_posoffset_manual
// evt_btl_camera_set_posoffset
// btl_camera_set_posoffset
// evt_btl_camera_nomove_z_onoff
// btl_camera_nomove_z_onoff
// evt_btl_camera_nomove_y_onoff
// btl_camera_nomove_y_onoff
// evt_btl_camera_nomove_x_onoff
// btl_camera_nomove_x_onoff
// evt_btl_camera_noshake
// btl_camera_noshake
EVT_DECLARE_USER_FUNC(evt_btl_camera_shake_h, 5)
// btl_camera_shake_h
// evt_btl_camera_shake_w
// btl_camera_shake_w
EVT_DECLARE_USER_FUNC(evt_btl_camera_set_moveto, 9)
// btl_camera_set_moveto
// evt_btl_camera_set_zoomSpeedLv
EVT_DECLARE_USER_FUNC(evt_btl_camera_set_moveSpeedLv, 2)
// btl_camera_set_zoomSpeedLv
// btl_camera_set_moveSpeedLv
// evt_btl_camera_add_zoom
// evt_btl_camera_set_zoom
// btl_camera_add_zoom
// btl_camera_set_zoom
// evt_btl_camera_set_homing_unit_audience
// evt_btl_camera_set_homing_unitparts
// evt_btl_camera_set_homing_unit
EVT_DECLARE_USER_FUNC(evt_btl_camera_set_mode, 2)
// btl_camera_set_mode
EVT_DECLARE_USER_FUNC(evt_btl_camera_set_prilimit, 1)
// btl_camera_set_prilimit
// battleCameraGetPosMoveSpeed
// battleCameraMoveTo
// battleCameraMain
// battleCameraInit

}

}