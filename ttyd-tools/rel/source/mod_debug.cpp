#include "mod_debug.h"

#include "common_functions.h"
#include "common_types.h"
#include "common_ui.h"
#include "custom_enemy.h"

#include <ttyd/battle_database_common.h>
#include <ttyd/battle_monosiri.h>
#include <ttyd/msgdrv.h>
#include <ttyd/system.h>

#include <cinttypes>
#include <cstdio>
#include <cstring>

namespace mod::infinite_pit {
    
namespace {
    
namespace BattleUnitType = ::ttyd::battle_database_common::BattleUnitType;

// Globals.
int32_t g_DebugMode = 0;
int32_t g_DebugEnemies[5] = { -1, -1, -1, -1, -1 };
int32_t g_CursorPos = 0;
int32_t g_DisplayMenu = 0;

}

void DebugManager::Update() {
    if (g_DebugMode) {
        uint32_t buttons = ttyd::system::keyGetButton(0);
        uint32_t button_trg = ttyd::system::keyGetButtonTrg(0);
        int32_t dir = 0;
        
        if (button_trg & ButtonId::DPAD_UP) {
            g_CursorPos = (g_CursorPos + 4) % 5;
        } else if (button_trg & ButtonId::DPAD_DOWN) {
            g_CursorPos = (g_CursorPos + 1) % 5;
        } else if (button_trg & ButtonId::DPAD_LEFT) {
            dir = -1;
        } else if (button_trg & ButtonId::DPAD_RIGHT) {
            dir = 1;
        } else if (button_trg & ButtonId::Y) {
            if (buttons & ButtonId::L) {
                g_DisplayMenu ^= 1;
            } else {
                // Clear all enemies after this point.
                for (int32_t i = g_CursorPos; i < 5; ++i)
                    g_DebugEnemies[i] = -1;
            }
        }
        
        // Don't allow changes to enemy loadout if menu is turned off.
        if (dir == 0 || !g_DisplayMenu) return;
        
        const int32_t max_enemy = BattleUnitType::BONETAIL - 1;
        const int32_t first_move = (buttons & ButtonId::L) ? 0x10 : 1;
        int32_t enemy_type = g_DebugEnemies[g_CursorPos];
        
        // Change selected enemy by +/-1; if L is held, add/subtract 0x10 first.
        if (dir < 0) {
            if (enemy_type < first_move) {
                enemy_type = -1;
            } else {
                enemy_type -= first_move;
            }
        } else {
            if (enemy_type + first_move > max_enemy) {
                enemy_type = -1;
            } else {
                enemy_type += first_move;
            }
        }
        
        do {
            if (g_CursorPos == 0) {
                // If valid for the front, fill the first 3 slots with the type.
                if (IsEligibleFrontEnemy(enemy_type)) {
                    const int32_t num_enemies = 
                        enemy_type == BattleUnitType::ATOMIC_BOO ? 1 : 3;
                    for (int32_t i = 0; i < 5; ++i)
                        g_DebugEnemies[i] = i < num_enemies ? enemy_type : -1;
                    break;
                }
            } else {
                // If valid, change the currently selected slot only.
                if (IsEligibleLoadoutEnemy(enemy_type) || enemy_type == -1) {
                    if (enemy_type != BattleUnitType::ATOMIC_BOO) {
                        g_DebugEnemies[g_CursorPos] = enemy_type;
                        // Exception: if type == "None", clear all later slots.
                        if (enemy_type == -1) {
                            for (int32_t i = g_CursorPos + 1; i < 5; ++i)
                                g_DebugEnemies[i] = -1;
                        }
                        break;
                    }
                }
            }

            // Not a valid enemy, keep looking for the next valid one.
            if (dir < 0 && enemy_type == -1) {
                enemy_type = max_enemy;
            } else if (dir > 0 && enemy_type == max_enemy) {
                enemy_type = -1;
            } else {
                enemy_type += dir;
            }
        } while (true);
    }
}

void DebugManager::Draw() {
    if (InMainGameModes() && g_DisplayMenu) {
        char buf[32];
        for (int32_t i = 0; i < 5; ++i) {
            int32_t bg_color = i == g_CursorPos ? 0xC00000E5u : 0x000000E5u;
            if (g_DebugEnemies[i] >= 1) {
                sprintf(buf, "%s (0x%02x)",
                    ttyd::msgdrv::msgSearch(
                        ttyd::battle_monosiri::battleGetUnitMonosiriPtr(
                            g_DebugEnemies[i])->unit_name),
                    g_DebugEnemies[i]);
            } else {
                sprintf(buf, "None");
            }
            DrawCenteredTextWindow(
                buf, 0, -20 - (30 * i), 0xFFu, true, 0xFFFFFFFFu, 0.7f, 
                bg_color, 10, 7);
        }
    }
}
    
void DebugManager::ChangeMode() {
    g_DebugMode ^= 1;
    g_DisplayMenu = g_DebugMode;
}

int32_t* DebugManager::GetEnemies() {
    if (g_DebugMode && g_DebugEnemies[0] >= 0) return g_DebugEnemies;
    return nullptr;
}

}