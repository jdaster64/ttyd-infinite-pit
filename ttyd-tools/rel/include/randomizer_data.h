#pragma once

#include "common_types.h"

#include <ttyd/npcdrv.h>

#include <cstdint>

namespace mod::pit_randomizer {

// Selects the enemies to spawn on a given floor, and returns the supplemental
// module to be loaded, if any.
ModuleId::e SelectEnemies(int32_t floor);
    
// Procedurally builds a NpcSetupInfo and BattleSetupData based on the floor #.
// Returns the salient NPC info (the battle is constructed in place over
// the battle that would normally be loaded), as well as the lead enemy's type.
void BuildBattle(
    uintptr_t pit_module_ptr, int32_t floor,
    ttyd::npcdrv::NpcTribeDescription** out_npc_tribe_description,
    ttyd::npcdrv::NpcSetupInfo** out_npc_setup_info, int32_t* out_lead_type);
    
// Randomly sets parameters for a battle condition that grants a bonus item.
void SetBattleCondition(ttyd::npcdrv::NpcBattleInfo* npc_info, bool enable = true);

// Returns a string based on the current battle condition, if any.
void GetBattleConditionString(char* out_buf);

// Picks an item from the standardized pool of items / stackable badges used
// for various purposes (enemy items, Charlieton, Kiss Thief, etc.),
// using either the mod's random state (seeded = true) or TTYD's RNG (false).
// Returns 0 if the "no item" case was picked.
int32_t PickRandomItem(
    bool seeded, int32_t normal_item_weight, int32_t recipe_item_weight,
    int32_t badge_weight, int32_t no_item_weight = 0);
    
// Picks a reward for a chest, updating the randomizer state accordingly.
// Reward is either an item/badge (if the result > 0) or a partner (-1 to -7).
int16_t PickChestReward();

}