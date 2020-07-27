#pragma once

#include "battle_database_common.h"

#include <cstdint>

namespace ttyd::unit_party_nokotarou {

extern "C" {

// .text
// _check_guard_koura
// _tsuranuki_effect_control
// _color_lv_set
// _check_mario_move_count
// __makeTechMenuFunc

// .data
// defence
// defence_attr
// defence_turn
// regist
// pose_table_nokotarou_stay
// pose_table_nokotarou_turn
// data_table
// unitpartsdata_Party_Nokotarou
// unitdata_Party_Nokotarou
// battle_entry_event
// init_event
// damage_event
// change_party_spawn_init_event
// dmg_turn_event
// wakeup_event
// attack_event
// wait_event
extern battle_database_common::BattleWeapon partyWeapon_NokotarouFirstAttack;
extern battle_database_common::BattleWeapon partyWeapon_NokotarouKouraAttack;
extern battle_database_common::BattleWeapon partyWeapon_NokotarouSyubibinKoura;
extern battle_database_common::BattleWeapon partyWeapon_NokotarouKouraGuard;
extern battle_database_common::BattleWeapon partyWeapon_NokotarouTsuranukiKoura;
// _change_koura_pose
// _change_koura_pose_fast
// _restore_koura_pose
// _koura_rotate_start
// _koura_rotate_stop
// partyNokotarouAttack_FirstAttack
// partyNokotarouAttack_NormalAttack
// partyNokotarouAttack_SyubibinKoura
// entry_koura
// partyNokotarouAttack_KouraGuard
// partyNokotarouAttack_TsuranukiKoura
// nokotarou_appeal_event
// party_win_reaction
// attack_audience_event
// UNKNOWN_MS_z0x80393a14

}

}