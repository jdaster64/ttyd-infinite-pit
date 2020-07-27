#pragma once

#include "battle_database_common.h"

#include <cstdint>

namespace ttyd::unit_party_vivian {

extern "C" {

// .text
// _disp_heart_entry_stop_check
// _disp_heart_entry_stop
// _disp_heart
// _disp_heart_entry
// _make_kagenuke_weapon
// unk_JP_US_EU_52_80182cc4
// _get_move_frame
// __makeTechMenuFunc
// _vivian_make_extra_work_area
// battle_evt_majo_disp_off
// battle_evt_majo_disp_on

// .data
// defence
// defence_attr
// regist
// pose_table_vivian_stay
// data_table
// unitpartsdata_Party_Vivian
// unitdata_Party_Vivian
// battle_entry_event
// init_event
// damage_event
// vivian_hide_event
// attack_event
// wait_event
extern battle_database_common::BattleWeapon partyWeapon_VivianNormalAttack;
extern battle_database_common::BattleWeapon partyWeapon_VivianShadowGuard;
extern battle_database_common::BattleWeapon partyWeapon_VivianMagicalPowder;
extern battle_database_common::BattleWeapon partyWeapon_VivianCharmKissAttack;
// partyVivianAttack_NormalAttack
// partyVivianAttack_ShadowGuard
// partyVivianAttack_MagicalPowder
// partyVivianAttack_CharmKissAttack
// vivian_shadow_tail_event
// vivian_counter_damage_event
// vivian_appeal_event
// party_win_reaction
// attack_audience_event

}

}