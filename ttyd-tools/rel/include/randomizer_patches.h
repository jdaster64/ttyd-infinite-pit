#pragma once

#include <gc/OSLink.h>
#include <ttyd/battle_unit.h>
#include <ttyd/evtmgr.h>

#include <cstdint>

namespace mod::pit_randomizer {

// Code that runs after linking a new module.
void OnModuleLoaded(gc::OSLink::OSModuleInfo* module);
// Replaces the existing logic for loading a map.
// Returns 1 if the map is not finished loading, and 2 if it is.
int32_t LoadMap();
// Code that runs immediately before unloading a map.
void OnMapUnloaded();

// Returns a string to display in place of the usual one for a given key,
// or nullptr if the default should be printed.
const char* GetReplacementMessage(const char* msg_key);

// Checks whether the battle condition was satisfied, and if so,
// adds a bonus item to the "recovered items" pool.
void CheckBattleCondition();

// Runs extra code on consuming an item and getting the item to be consumed,
// allowing for enemies to use generic cooking items.
// GetConsumeItem returns true if the evt was run by an enemy.
void EnemyConsumeItem(ttyd::evtmgr::EvtEntry* evt);
bool GetEnemyConsumeItem(ttyd::evtmgr::EvtEntry* evt);
// Checks for enemies to use additional types of items.
void* EnemyUseAdditionalItemsCheck(ttyd::battle_unit::BattleWorkUnit* unit);

// Apply patches to item, badge, and weapon data.
void ApplyItemAndAttackPatches();

// Apply miscellaneous small patches that do not require function hooks.
void ApplyMiscPatches();

// Fetches information required for dynamically spawning an enemy NPC,
// such as the model name, battle id, and initial position.
EVT_DECLARE_USER_FUNC(GetEnemyNpcInfo, 7)

}