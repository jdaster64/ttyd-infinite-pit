#pragma once

#include <cstdint>

#include "evtmgr.h"

namespace ttyd::evt_snd {

extern "C" {

// evt_snd_sfx_all_stop
// evt_snd_set_rev_mode
// evt_snd_env_lpf
// evt_snd_envoff_f
// evt_snd_envoff
// evt_snd_envon_f
// evt_snd_envon
// evt_snd_sfx_dist
// evt_snd_sfx_pos
// evt_snd_sfx_vol
// evt_snd_sfx_pit
// evt_snd_sfxchk
// evt_snd_sfxoff
// evt_snd_sfxon_3d_ex
// evt_snd_sfxon_3d
// evt_snd_sfxon_
// evt_snd_sfxon__
// evt_snd_sfxon
// evt_snd_bgm_scope
// evt_snd_bgm_freq
// evt_snd_bgmoff_f
EVT_DECLARE_USER_FUNC(evt_snd_bgmoff, 1)
// unk_JP_US_EU_37_801524c8
EVT_DECLARE_USER_FUNC(evt_snd_bgmon_f, 3)
EVT_DECLARE_USER_FUNC(evt_snd_bgmon, 2)

}

}