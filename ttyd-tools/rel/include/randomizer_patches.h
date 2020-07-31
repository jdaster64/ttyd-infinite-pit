#pragma once

#include <gc/OSLink.h>
#include <ttyd/battle_database_common.h>
#include <ttyd/battle_unit.h>
#include <ttyd/evtmgr.h>
#include <ttyd/npcdrv.h>

#include <cstdint>

namespace mod::pit_randomizer {

// Code that runs after linking a new module.
void OnModuleLoaded(gc::OSLink::OSModuleInfo* module);
// Replaces the existing logic for loading a map.
// Returns 1 if the map is not finished loading, and 2 if it is.
int32_t LoadMap();
// Code that runs immediately before unloading a map.
void OnMapUnloaded();

// Code to run at start / end of battle.
void OnEnterExitBattle(bool is_start);
// If the badge is one that can have its power level selected, returns the
// index of the value controlling its level; otherwise, returns -1.
int32_t GetWeaponLevelSelectionIndex(int16_t badge_id);
// Gets the cost of badge moves that can have their power level selected.
// Returns -1 if the weapon is not one of these moves.
int32_t GetSelectedLevelWeaponCost(
    ttyd::battle_unit::BattleWorkUnit* unit,
    ttyd::battle_database_common::BattleWeapon* weapon);
// Extra code for the battle menus that allows selecting level of badge moves.
// Assumes the menu is one of the standard weapon menus (Jump, Hammer, etc.)
// if is_strategies_menu = false.
void CheckForSelectingWeaponLevel(bool is_strategies_menu);

// Ranks up and fully heals the selected party member when using a Shine Sprite.
void UseShineSprite();

// Checks whether the battle condition was satisfied, and if so,
// adds a bonus item to the "recovered items" pool.
void CheckBattleCondition();
// Displays text associated with the battle condition.
void DisplayBattleCondition();

// Replaces the logic for getting HP, FP, and item drops after a battle.
void GetDropMaterials(ttyd::npcdrv::FbatBattleInformation* fbat_info);

// Runs extra code on consuming an item and getting the item to be consumed,
// allowing for enemies to use generic cooking items.
// GetConsumeItem returns true if the evt was run by an enemy.
void EnemyConsumeItem(ttyd::evtmgr::EvtEntry* evt);
bool GetEnemyConsumeItem(ttyd::evtmgr::EvtEntry* evt);
// Checks for enemies to use additional types of items.
void* EnemyUseAdditionalItemsCheck(ttyd::battle_unit::BattleWorkUnit* unit);

// Displays the Star Power in 0.01 units numerically below the status window.
void DisplayStarPowerNumber();

// Display the orbs representing the Star Power (replaces the vanilla logic
// since it wasn't built around receiving Star Powers out of order).
void DisplayStarPowerOrbs(double x, double y, int32_t star_power);

// Returns a string to display in place of the usual one for a given key,
// or nullptr if the default should be printed.
const char* GetReplacementMessage(const char* msg_key);

// Apply patches related to selecting the power of badge moves in battle.
void ApplyWeaponLevelSelectionPatches();

// Apply patches to item, badge, and weapon data.
void ApplyItemAndAttackPatches();

// Apply miscellaneous small patches that do not require function hooks.
void ApplyMiscPatches();

// Fetches information required for dynamically spawning an enemy NPC,
// such as the model name, battle id, and initial position.
EVT_DECLARE_USER_FUNC(GetEnemyNpcInfo, 7)

// Sets final battle info on a Pit enemy, as well as setting any
// battle conditions.
EVT_DECLARE_USER_FUNC(SetEnemyNpcBattleInfo, 2)

// Returns the item or partner to spawn from the chest on a Pit reward floor.
EVT_DECLARE_USER_FUNC(GetChestReward, 1)

// If the item is a Crystal Star, gives the player +1.00 max SP and
// the respective Star Power.
EVT_DECLARE_USER_FUNC(AddItemStarPower, 1)

// If Vivian's Infatuate lands successfully, tries changing the effect from
// Confuse to the enemy permanently swapping its alliance.
// (Also runs the replaced code from btlevtcmd_AudienceDeclareACResult).
EVT_DECLARE_USER_FUNC(InfatuateChangeAlliance, 2)

// Returns the percentage of max HP a battle unit currently has.
EVT_DECLARE_USER_FUNC(GetPercentOfMaxHP, 2)

// Replaces the behavior of Mowz's Kiss Thief.
EVT_DECLARE_USER_FUNC(GetKissThiefResult, 3)

}