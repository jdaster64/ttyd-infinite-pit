#pragma once

#include "battle_database_common.h"

#include <cstdint>

namespace ttyd::unit_party_sanders {

extern "C" {

// .text
// _judge_on_stage
// _shot_move
// _make_counterset_weapon
// _get_bomb_hit_position
// __makeTechMenuFunc
// _sanders_make_extra_work_area

// .data
// defence
// defence_attr
// regist
// pose_table_sanders_stay
// pose_table_counter_sanders
// data_table
// unitpartsdata_Party_Sanders
// unitdata_Party_Sanders
// battle_entry_event
// init_event
// damage_event
// attack_event
// wait_event
extern battle_database_common::BattleWeapon partyWeapon_SandersFirstAttack;
extern battle_database_common::BattleWeapon partyWeapon_SandersNormalAttack;
extern battle_database_common::BattleWeapon partyWeapon_SandersTimeBombSet;
extern battle_database_common::BattleWeapon partyWeapon_SandersCounterSet;
extern battle_database_common::BattleWeapon partyWeapon_SandersSuperBombAttack;
// partySandersAttack_FirstAttack
// partySandersAttack_NormalAttack
// partySandersAttack_SuperBombAttack
// partySandersAttack_CounterSet
// entry_bomzou
// partySandersAttack_TimeBombSet
// _shot_bomb_event
// sanders_appeal_event
// party_win_reaction
// attack_audience_event
// UNKNOWN_MS_z0x8038b678

}

}