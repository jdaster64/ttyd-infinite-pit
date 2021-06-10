#include "patches_mario_move.h"

#include "common_types.h"
#include "mod.h"
#include "mod_state.h"
#include "patch.h"

#include <ttyd/battle.h>
#include <ttyd/battle_database_common.h>
#include <ttyd/battle_mario.h>
#include <ttyd/battle_menu_disp.h>
#include <ttyd/battle_unit.h>
#include <ttyd/item_data.h>
#include <ttyd/mario_pouch.h>
#include <ttyd/msgdrv.h>
#include <ttyd/sac_zubastar.h>
#include <ttyd/sound.h>
#include <ttyd/system.h>

#include <cinttypes>
#include <cstdio>
#include <cstring>

namespace mod::infinite_pit {

namespace {

using ::ttyd::battle::BattleWorkCommandCursor;
using ::ttyd::battle::BattleWorkCommandOperation;
using ::ttyd::battle::BattleWorkCommandWeapon;
using ::ttyd::battle_database_common::BattleWeapon;
using ::ttyd::battle_unit::BattleWorkUnit;
using ::ttyd::item_data::itemDataTable;

namespace BattleUnitType = ::ttyd::battle_database_common::BattleUnitType;
namespace ItemType = ::ttyd::item_data::ItemType;

}

// Function hooks.
extern int32_t (*g_BtlUnit_GetWeaponCost_trampoline)(BattleWorkUnit*, BattleWeapon*);
extern int32_t (*g_pouchEquipCheckBadge_trampoline)(int16_t);
extern void (*g_DrawOperationWin_trampoline)();
extern void (*g_DrawWeaponWin_trampoline)();
// Patch addresses.

namespace mario_move {

namespace {
    
// Global variables and constants.
bool                g_InBattle = false;
int8_t              g_MaxMoveBadgeCounts[18];
int8_t              g_CurMoveBadgeCounts[18];
int8_t              g_MaxSpecialMoveLvls[8];
int8_t              g_CurSpecialMoveLvls[8];
char                g_MoveBadgeTextBuffer[24];
const char*         kMoveBadgeAbbreviations[18] = {
    "Power J.", "Multib.", "Power B.", "Tor. J.", "Shrink S.",
    "Sleep S.", "Soft S.", "Power S.", "Quake H.", "H. Throw",
    "Pier. B.", "H. Rattle", "Fire Drive", "Ice Smash",
    "Charge", "Charge", "Tough. Up", "Tough. Up"
};
const char*         kSpecialMoveAbbreviations[8] = {
    "Sweet Tr.", "Earth Tr.", "Clock Out", "Power Lift",
    "Art Attack", "Sweet F.", "Showst.", "Supernova"
};

// Returns the max level of a move badge based on the number of copies equipped.
int32_t MaxLevelForMoveBadges(int32_t badge_count) {
    int32_t max_level = badge_count;
    switch (g_Mod->state_.GetOptionValue(
        StateManager::MAX_BADGE_MOVE_LEVEL)) {
        case StateManager::MAX_MOVE_LEVEL_1X: {
            break;
        }
        case StateManager::MAX_MOVE_LEVEL_2X: {
            max_level *= 2;
            break;
        }
        case StateManager::MAX_MOVE_LEVEL_RANK: {
            if (!badge_count) return 0;
            return ttyd::mario_pouch::pouchGetPtr()->rank + 1;
        }
        case StateManager::MAX_MOVE_LEVEL_INFINITE: {
            if (!badge_count) return 0;
            return 99;
        }
    }
    if (max_level > 99) max_level = 99;
    return max_level;
}

// If the badge is one that can have its power level selected, returns the
// index of the value controlling its level; otherwise, returns -1.
int32_t GetWeaponLevelSelectionIndex(int16_t badge_id) {
    if (badge_id >= ItemType::POWER_JUMP && badge_id <= ItemType::ICE_SMASH) {
        return badge_id - ItemType::POWER_JUMP;
    }
    switch (badge_id) {
        case ItemType::CHARGE: return 14;
        case ItemType::CHARGE_P: return 15;
        case ItemType::SUPER_CHARGE: return 16;
        case ItemType::SUPER_CHARGE_P: return 17;
    }
    return -1;
}

// Gets the cost of badge moves that can have their power level selected.
// Returns -1 if the weapon is not one of these moves.
int32_t GetSelectedLevelWeaponCost(BattleWorkUnit* unit, BattleWeapon* weapon) {
    if (!weapon || !g_InBattle) return -1;
    if (int32_t idx = GetWeaponLevelSelectionIndex(weapon->item_id); idx >= 0) {
        const int32_t fp_cost =
            weapon->base_fp_cost * g_CurMoveBadgeCounts[idx] -
            unit->badges_equipped.flower_saver;
        return fp_cost < 1 ? 1 : fp_cost;
    }
    return -1;
}

// Extra code for the battle menus that allows selecting level of badge moves.
// Assumes the menu is one of the standard weapon menus (Jump, Hammer, etc.)
// if is_strategies_menu = false.
void CheckForSelectingWeaponLevel(bool is_strategies_menu) {
    const uint16_t buttons = ttyd::system::keyGetButtonTrg(0);
    const bool left_press = buttons & (ButtonId::L | ButtonId::DPAD_LEFT);
    const bool right_press = buttons & (ButtonId::R | ButtonId::DPAD_RIGHT);
    
    void** win_data = reinterpret_cast<void**>(
        ttyd::battle::g_BattleWork->command_work.window_work);
    if (!win_data || !win_data[0]) return;
    
    auto* cursor = reinterpret_cast<BattleWorkCommandCursor*>(win_data[0]);
    if (is_strategies_menu) {
        auto* battleWork = ttyd::battle::g_BattleWork;
        auto* strats = battleWork->command_work.operation_table;
        BattleWorkUnit* unit =
            battleWork->battle_units[battleWork->active_unit_idx];
        BattleWeapon* weapon = nullptr;
        int32_t idx = 0;
        for (int32_t i = 0; i < cursor->num_options; ++i) {
            // Not selecting Charge or Super Charge.
            if (strats[i].type < 1 || strats[i].type > 2) continue;
            
            if (strats[i].type == 1) {
                if (unit->current_kind == BattleUnitType::MARIO) {
                    weapon = &ttyd::battle_mario::badgeWeapon_Charge;
                } else {
                    weapon = &ttyd::battle_mario::badgeWeapon_ChargeP;
                }
            } else {
                if (unit->current_kind == BattleUnitType::MARIO) {
                    weapon = &ttyd::battle_mario::badgeWeapon_SuperCharge;
                } else {
                    weapon = &ttyd::battle_mario::badgeWeapon_SuperChargeP;
                }
            }
            idx = GetWeaponLevelSelectionIndex(weapon->item_id);
            if (idx < 0 || g_MaxMoveBadgeCounts[idx] <= 1) continue;
            
            // If current selection, and L/R pressed, change power level.
            if (i == cursor->abs_position) {
                if (left_press && g_CurMoveBadgeCounts[idx] > 1) {
                    --g_CurMoveBadgeCounts[idx];
                    ttyd::sound::SoundEfxPlayEx(0x478, 0, 0x64, 0x40);
                } else if (
                    right_press && 
                    g_CurMoveBadgeCounts[idx] < g_MaxMoveBadgeCounts[idx]) {
                    ++g_CurMoveBadgeCounts[idx];
                    ttyd::sound::SoundEfxPlayEx(0x478, 0, 0x64, 0x40);
                }
                
                // Overwrite default text based on current power level.
                sprintf(
                    g_MoveBadgeTextBuffer, "%s Lv. %" PRId8,
                    kMoveBadgeAbbreviations[idx], g_CurMoveBadgeCounts[idx]);
                strats[i].name = g_MoveBadgeTextBuffer;
            } else {
                strats[i].name = ttyd::msgdrv::msgSearch(
                    itemDataTable[weapon->item_id].name);
            }
            
            strats[i].cost = GetSelectedLevelWeaponCost(unit, weapon);
            strats[i].enabled =
                strats[i].cost <= ttyd::battle_unit::BtlUnit_GetFp(unit);
            strats[i].unk_08 = !strats[i].enabled;  // 1 if disabled: "no FP" msg
        }
        
        // Handle switch partner cost, if enabled.
        int32_t switch_fp_cost = g_Mod->state_.GetOptionValue(
            StateManager::SWITCH_PARTY_COST_FP);
        if (strats[0].type == 0 && switch_fp_cost) {
            // Reduce switch partner cost by Flower Savers.
            switch_fp_cost -= unit->badges_equipped.flower_saver;
            if (switch_fp_cost < 1) switch_fp_cost = 1;
            
            strats[0].cost = switch_fp_cost;
            strats[0].enabled =
                ttyd::battle_unit::BtlUnit_GetFp(unit) >= switch_fp_cost;
            strats[0].unk_08 = !strats[0].enabled;  // 1 if disabled: "no FP" msg
        }
    } else {
        auto* weapons = reinterpret_cast<BattleWorkCommandWeapon*>(win_data[2]);
        if (!weapons) return;
        for (int32_t i = 0; i < cursor->num_options; ++i) {
            if (!weapons[i].weapon) continue;
            auto* weapon = weapons[i].weapon;
            
            // Handle Special moves.
            if (weapon->base_sp_cost) {
                static const int8_t kSpCostLevels[] = {
                    1, 3, 5,    1, 2, 3,    2, 3, 5,    3, 4, 6,
                    3, 5, 7,    4, 6, 8,    2, 4, 6,    5, 7, 9,
                };
                
                // Match the Star Power by its icon.
                int32_t idx = 0;
                switch (weapon->icon) {
                    case 0x19f: { idx = 1; break; }
                    case 0x1a0: { idx = 2; break; }
                    case 0x1a2: { idx = 3; break; }
                    case 0x1a4: { idx = 4; break; }
                    case 0x1a5: { idx = 5; break; }
                    case 0x1a1: { idx = 6; break; }
                    case 0x1a3: { idx = 7; break; }
                }
                
                // If current selection, and L/R pressed, change power level.
                if (i == cursor->abs_position) {
                    if (left_press && g_CurSpecialMoveLvls[idx] > 1) {
                        --g_CurSpecialMoveLvls[idx];
                        ttyd::sound::SoundEfxPlayEx(0x478, 0, 0x64, 0x40);
                    } else if (
                        right_press &&
                        g_CurSpecialMoveLvls[idx] < g_MaxSpecialMoveLvls[idx]) {
                        ++g_CurSpecialMoveLvls[idx];
                        ttyd::sound::SoundEfxPlayEx(0x478, 0, 0x64, 0x40);
                    }
                    
                    // Overwrite default text based on current power level.
                    sprintf(
                        g_MoveBadgeTextBuffer, "%s Lv. %" PRId8,
                        kSpecialMoveAbbreviations[idx],
                        g_CurSpecialMoveLvls[idx]);
                    weapons[i].name = g_MoveBadgeTextBuffer;
                } else {
                    weapons[i].name = ttyd::msgdrv::msgSearch(weapon->name);
                }
                
                // Update actual SP cost.
                int8_t new_cost = 
                    kSpCostLevels[idx * 3 + g_CurSpecialMoveLvls[idx] - 1];
                mod::patch::writePatch(
                    &ttyd::battle_mario::superActionTable[idx]->base_sp_cost,
                    &new_cost, sizeof(new_cost));
            }
            
            // Otherwise, must be an FP-costing move (assuming a badge move).
            const int32_t idx = GetWeaponLevelSelectionIndex(weapon->item_id);
            if (idx < 0 || g_MaxMoveBadgeCounts[idx] <= 1) continue;
            
            // If current selection, and L/R pressed, change power level.
            if (i == cursor->abs_position) {
                if (left_press && g_CurMoveBadgeCounts[idx] > 1) {
                    --g_CurMoveBadgeCounts[idx];
                    ttyd::sound::SoundEfxPlayEx(0x478, 0, 0x64, 0x40);
                } else if (
                    right_press && 
                    g_CurMoveBadgeCounts[idx] < g_MaxMoveBadgeCounts[idx]) {
                    ++g_CurMoveBadgeCounts[idx];
                    ttyd::sound::SoundEfxPlayEx(0x478, 0, 0x64, 0x40);
                }
                
                // Overwrite default text based on current power level.
                sprintf(
                    g_MoveBadgeTextBuffer, "%s Lv. %" PRId8,
                    kMoveBadgeAbbreviations[idx], g_CurMoveBadgeCounts[idx]);
                weapons[i].name = g_MoveBadgeTextBuffer;
            } else {
                weapons[i].name = ttyd::msgdrv::msgSearch(
                    itemDataTable[weapon->item_id].name);
            }
        }
    }
}

}
    
void ApplyFixedPatches() {
    g_pouchEquipCheckBadge_trampoline = patch::hookFunction(
        ttyd::mario_pouch::pouchEquipCheckBadge, [](int16_t badge_id) {
            if (g_InBattle) {
                int32_t idx = GetWeaponLevelSelectionIndex(badge_id);
                if (idx >= 0) {
                    return static_cast<int32_t>(g_CurMoveBadgeCounts[idx]);
                }
            }
            return g_pouchEquipCheckBadge_trampoline(badge_id);
        });

    g_BtlUnit_GetWeaponCost_trampoline = patch::hookFunction(
        ttyd::battle_unit::BtlUnit_GetWeaponCost,
        [](BattleWorkUnit* unit, BattleWeapon* weapon) {
            int32_t cost = GetSelectedLevelWeaponCost(unit, weapon);
            if (cost >= 0) return cost;
            return g_BtlUnit_GetWeaponCost_trampoline(unit, weapon);
        });

    g_DrawOperationWin_trampoline = patch::hookFunction(
        ttyd::battle_menu_disp::DrawOperationWin, []() {
            CheckForSelectingWeaponLevel(/* is_strategies_menu = */ true);
            g_DrawOperationWin_trampoline();
        });
        
    g_DrawWeaponWin_trampoline = patch::hookFunction(
        ttyd::battle_menu_disp::DrawWeaponWin, []() {
            CheckForSelectingWeaponLevel(/* is_strategies_menu = */ false);
            g_DrawWeaponWin_trampoline();
        });
    
    // Increase attack power of Supernova to 4 per bar instead of 3.
    ttyd::sac_zubastar::weapon_zubastar.damage_function_params[1] = 4;
    ttyd::sac_zubastar::weapon_zubastar.damage_function_params[2] = 8;
    ttyd::sac_zubastar::weapon_zubastar.damage_function_params[3] = 12;
    ttyd::sac_zubastar::weapon_zubastar.damage_function_params[4] = 16;
    ttyd::sac_zubastar::weapon_zubastar.damage_function_params[5] = 20;
}

void OnEnterExitBattle(bool is_start) {
    if (is_start) {
        int8_t badge_count;
        int32_t max_level;
        for (int32_t i = 0; i < 14; ++i) {
            badge_count = ttyd::mario_pouch::pouchEquipCheckBadge(
                ItemType::POWER_JUMP + i);
            max_level = MaxLevelForMoveBadges(badge_count);
            g_MaxMoveBadgeCounts[i] = max_level;
            g_CurMoveBadgeCounts[i] = max_level < 99 ? max_level : 1;
        }
        for (int32_t i = 0; i < 2; ++i) {
            badge_count = ttyd::mario_pouch::pouchEquipCheckBadge(
                ItemType::CHARGE + i);
            max_level = MaxLevelForMoveBadges(badge_count);
            g_MaxMoveBadgeCounts[14 + i] = max_level;
            g_CurMoveBadgeCounts[14 + i] = max_level < 99 ? max_level : 1;
            badge_count = ttyd::mario_pouch::pouchEquipCheckBadge(
                ItemType::SUPER_CHARGE + i);
            max_level = MaxLevelForMoveBadges(badge_count);
            g_MaxMoveBadgeCounts[16 + i] = max_level;
            g_CurMoveBadgeCounts[16 + i] = max_level < 99 ? max_level : 1;
        }
        for (int32_t i = 0; i < 8; ++i) {
            // TODO: Should only be able to select up to the level unlocked.
            g_MaxSpecialMoveLvls[i] = 3;
            g_CurSpecialMoveLvls[i] = 1;
        }
        g_InBattle = true;
    } else {
        g_InBattle = false;
    }
}

int8_t GetToughenUpLevel(bool is_mario) {
    return g_CurMoveBadgeCounts[17 - is_mario];
}

}  // namespace mario_move
}  // namespace mod::infinite_pit