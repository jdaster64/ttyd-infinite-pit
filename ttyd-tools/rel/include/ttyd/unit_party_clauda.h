#pragma once

#include "battle_database_common.h"

#include <cstdint>

namespace ttyd::unit_party_clauda {

extern "C" {

// .text
// _make_kumoguard_weapon
// _get_clauda_kiss_hit_position
// _make_kiss_weapon;
// _make_breath_weapon;
// _clauda_breath_effect_fire
// _clauda_breath_effect_ready
// _check_blow_rate
// __makeTechMenuFunc
// _clauda_make_extra_work_area

// .data
extern battle_database_common::BattleWeapon partyWeapon_ClaudaNormalAttack;
extern battle_database_common::BattleWeapon partyWeapon_ClaudaBreathAttack;
extern battle_database_common::BattleWeapon partyWeapon_ClaudaLipLockAttack;
extern battle_database_common::BattleWeapon partyWeapon_ClaudaKumogakureAttack;
// defence
// defence_attr
// regist
// pose_table_clauda_stay
// data_table
// unitpartsdata_Party_Clauda
// unitdata_Party_Clauda
// battle_entry_event
// init_event
// damage_event
// attack_event
// wait_event
// partyClaudaAttack_NormalAttack
// partyClaudaAttack_BreathAttack
// partyClaudaAttack_LipLockAttack
// partyClaudaAttack_KumoGuard
// clauda_appeal_event
// party_win_reaction
// attack_audience_event
// attack_bad_audience_event
// attack_good_audience_event

}

}