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
// the battle that would normally be loaded).
void BuildBattle(
    uintptr_t pit_module_ptr, int32_t floor,
    ttyd::npcdrv::NpcTribeDescription** out_npc_tribe_description,
    ttyd::npcdrv::NpcSetupInfo** out_npc_setup_info);

}