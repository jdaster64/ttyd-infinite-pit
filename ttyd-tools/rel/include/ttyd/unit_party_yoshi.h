#pragma once

#include "battle_database_common.h"

#include <cstdint>

namespace ttyd::unit_party_yoshi {

extern "C" {

// .text
// _btlYoshiDisp
// _gundan_yoshi_run_effect
// _wait_yoshig_complete
// _wait_yoshig_run
// _check_swallow_attribute
// _get_swallow_param
// _get_nomikomi_hit_position
// _yoshi_slide_move_sound
// btl_yoshi_yoroyoro_jump_move
// btl_yoshi_yoroyoro_jump_calc_param
// __makeTechMenuFunc
// yoshi_original_color_anim_set

// .data
// defence
// defence_attr
// regist
// pose_table_yoshi_stay
// pose_table_egg_g
// pose_table_egg_y
// pose_table_egg_p
// data_table
// unitpartsdata_Party_Yoshi
// unitdata_Party_Yoshi
// battle_entry_event
// init_event
// damage_event
// attack_event
// wait_event
// _yoshig_tpl_no_tbl
extern battle_database_common::BattleWeapon partyWeapon_YoshiNormalAttack;
extern battle_database_common::BattleWeapon partyWeapon_YoshiNomikomi_Shot;
extern battle_database_common::BattleWeapon partyWeapon_YoshiNomikomi_Spew;
extern battle_database_common::BattleWeapon partyWeapon_YoshiNomikomi_Dmg0;
extern battle_database_common::BattleWeapon partyWeapon_YoshiNomikomi_Fire;
extern battle_database_common::BattleWeapon partyWeapon_YoshiNomikomi_Involved;
extern battle_database_common::BattleWeapon partyWeapon_YoshiEggAttack_Minimini;
extern battle_database_common::BattleWeapon partyWeapon_YoshiCallGuard;
// posesound_normal_attack
// partyYoshiAttack_NormalAttack
// partyYoshiAttack_Nomikomi
// partyYoshiAttack_EggAttack
// _egg_attack_event
// partyYoshiAttack_CallGuard
// yoshi_appeal_event
// party_win_reaction
// attack_audience_event

}

}