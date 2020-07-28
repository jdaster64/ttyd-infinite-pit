#pragma once

#include "battle_database_common.h"
#include "evtmgr.h"

#include <cstdint>

namespace ttyd::unit_party_chuchurina {

extern "C" {

// .text
// mono_off
// mono_on
// mono_capture_event
// mono_main
// mono_disp
// mono_capture
EVT_DECLARE_USER_FUNC(_chuchu_item_steal, 3)
EVT_DECLARE_USER_FUNC(_get_itemsteal_param, 3)
EVT_DECLARE_USER_FUNC(_make_madowase_weapon, 2)
// _get_binta_hit_position
// __makeTechMenuFunc
// _chuchu_make_extra_work_area

// .data
// defence
// defence_attr
// regist
// pose_table_chuchurina_stay
// data_table
// unitpartsdata_Party_Chuchurina
// unitdata_Party_Chuchurina
// battle_entry_event
// init_event
// damage_event
// attack_event
// wait_event
extern battle_database_common::BattleWeapon partyWeapon_ChuchurinaNormalAttackLeft;
extern battle_database_common::BattleWeapon partyWeapon_ChuchurinaNormalAttackRight;
extern battle_database_common::BattleWeapon partyWeapon_ChuchurinaNormalAttackLeftLast;
extern battle_database_common::BattleWeapon partyWeapon_ChuchurinaNormalAttackRightLast;
extern battle_database_common::BattleWeapon partyWeapon_ChuchurinaMadowaseAttack;
extern battle_database_common::BattleWeapon partyWeapon_ChuchurinaItemSteal;
extern battle_database_common::BattleWeapon partyWeapon_ChuchurinaKiss;
// partyChuchurinaAttack_NormalAttack
// main_evt
// partyChuchurinaAttack_MadowaseAttack
// partyChuchurinaAttack_ItemSteal
// partyChuchurinaAttack_Kiss
// chuchurina_appeal_event
// party_win_reaction
// attack_audience_event

}

}