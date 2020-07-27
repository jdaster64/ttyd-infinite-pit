#pragma once

#include "battle_database_common.h"

#include <cstdint>

namespace ttyd::unit_party_christine {

extern "C" {

// .text
// _set_hustle
// _dictionary
// _monosiri_flag_on
// btlevtcmd_get_monosiri_msg_no
// __makeTechMenuFunc
// krb_get_dir

// .data
// defence
// defence_attr
// regist
// pose_table_christine_stay
// data_table
// unitpartsdata_Party_Christine
// unitdata_Party_Christine
// battle_entry_event
// init_event
// damage_event
// attack_event
// wait_event
extern battle_database_common::BattleWeapon partyWeapon_ChristineNormalAttack;
extern battle_database_common::BattleWeapon partyWeapon_ChristineMonosiri;
extern battle_database_common::BattleWeapon partyWeapon_ChristineRenzokuAttack;
extern battle_database_common::BattleWeapon partyWeapon_ChristineKiss;
// partyChristineAttack_NormalAttack
// christine_dictionary_event
// partyChristineAttack_Monosiri
// partyChristineAttack_RenzokuAttack
// partyChristineAttack_Kiss
// christine_appeal_event
// party_win_reaction
// attack_audience_event

}

}