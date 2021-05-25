#pragma once

#include "common_types.h"

#include <cstdint>

namespace mod::infinite_pit::enemy_fix {

// Apply patches for various fixes to issues with enemies in battle.
void ApplyFixedPatches();

// Apply patches to module code.
void ApplyModuleLevelPatches(void* module_ptr, ModuleId::e module_id);

// Sums attack target weights for attacks that target randomly,
// ensuring that individual target weights are always > 0.
int32_t SumWeaponTargetRandomWeights(int32_t* weights);

}