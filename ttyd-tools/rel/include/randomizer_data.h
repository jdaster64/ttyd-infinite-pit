#pragma once

#include "common_types.h"

#include <cstdint>

namespace mod::pit_randomizer {
    
namespace ttyd::npcdrv {
struct NpcSetupInfo;
}

// Procedurally constructs NpcSetupInfo and BattleSetupData based on a floor #.
// Returns the salient NPC info (the battle is constructed in place over
// the battle that would normally be loaded), and which module to load (if any).
void BuildBattle(
    uintptr_t pit_module_ptr, int32_t floor,
    const char** out_tribe_name, const char** out_model_name,
    ttyd::npcdrv::NpcSetupInfo* out_npc_setup_info, 
    ModuleId::e* out_secondary_module);

}