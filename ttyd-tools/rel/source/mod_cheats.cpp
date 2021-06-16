#include "mod_cheats.h"

#include "common_functions.h"
#include "common_types.h"
#include "common_ui.h"
#include "mod.h"
#include "mod_debug.h"
#include "mod_menu.h"
#include "mod_state.h"

#include <ttyd/item_data.h>
#include <ttyd/mario_pouch.h>
#include <ttyd/sound.h>
#include <ttyd/swdrv.h>
#include <ttyd/system.h>

#include <cinttypes>
#include <cstdio>
#include <cstring>

namespace mod::infinite_pit {
    
namespace {
    
namespace ItemType = ::ttyd::item_data::ItemType;

// Constants for secret codes.
uint32_t secretCode_BonusOptions1   = 012651265;
uint32_t secretCode_BonusOptions2   = 043652131;
uint32_t secretCode_BonusOptions3   = 031313141;
uint32_t secretCode_RtaTimer        = 034345566;
uint32_t secretCode_UnlockFxBadges  = 026122146;
uint32_t secretCode_DebugMode       = 036363636;

bool g_DrawRtaTimer = false;

}

void CheatsManager::Update() {
    // Process cheat codes.
    static uint32_t code_history = 0;
    int32_t code = 0;
    if (ttyd::system::keyGetButtonTrg(0) & ButtonId::A) code = 1;
    if (ttyd::system::keyGetButtonTrg(0) & ButtonId::B) code = 2;
    if (ttyd::system::keyGetButtonTrg(0) & ButtonId::L) code = 3;
    if (ttyd::system::keyGetButtonTrg(0) & ButtonId::R) code = 4;
    if (ttyd::system::keyGetButtonTrg(0) & ButtonId::X) code = 5;
    if (ttyd::system::keyGetButtonTrg(0) & ButtonId::Y) code = 6;
    if (code) code_history = (code_history << 3) | code;
    if ((code_history & 0xFFFFFF) == secretCode_RtaTimer) {
        code_history = 0;
        g_DrawRtaTimer = true;
        ttyd::sound::SoundEfxPlayEx(0x265, 0, 0x64, 0x40);
    }
    if ((code_history & 0xFFFFFF) == secretCode_BonusOptions1) {
        code_history = 0;
        MenuManager::SetMenuPageVisibility(5, true);
        ttyd::sound::SoundEfxPlayEx(0x265, 0, 0x64, 0x40);
    }
    if ((code_history & 0xFFFFFF) == secretCode_BonusOptions2) {
        code_history = 0;
        MenuManager::SetMenuPageVisibility(6, true);
        ttyd::sound::SoundEfxPlayEx(0x265, 0, 0x64, 0x40);
    }
    if ((code_history & 0xFFFFFF) == secretCode_BonusOptions3) {
        code_history = 0;
        // TODO: Implement toggling BGM on/off.
        // MenuManager::SetMenuPageVisibility(7, true);
        ttyd::sound::SoundEfxPlayEx(0x265, 0, 0x64, 0x40);
    }
    if ((code_history & 0xFFFFFF) == secretCode_UnlockFxBadges) {
        code_history = 0;
        // Check Journal for whether the FX badges were already unlocked.
        bool has_fx_badges = ttyd::swdrv::swGet(
            ItemType::ATTACK_FX_R - ItemType::POWER_JUMP + 0x80);
        if (!has_fx_badges && ttyd::mario_pouch::pouchGetHaveBadgeCnt() < 196) {
            ttyd::mario_pouch::pouchGetItem(ItemType::ATTACK_FX_P);
            ttyd::mario_pouch::pouchGetItem(ItemType::ATTACK_FX_G);
            ttyd::mario_pouch::pouchGetItem(ItemType::ATTACK_FX_B);
            ttyd::mario_pouch::pouchGetItem(ItemType::ATTACK_FX_Y);
            ttyd::mario_pouch::pouchGetItem(ItemType::ATTACK_FX_R);
            ttyd::sound::SoundEfxPlayEx(0x265, 0, 0x64, 0x40);
        }
    }
    // TODO: Turn off before releases!
    if ((code_history & 0xFFFFFF) == secretCode_DebugMode) {
        code_history = 0;
        DebugManager::ChangeMode();
    }
}

void CheatsManager::Draw() {
    if (InMainGameModes() && g_DrawRtaTimer) {
        // Print the current RTA timer to the screen.
        char buf[32];
        sprintf(buf, "%s", g_Mod->ztate_.GetCurrentTimeString());
        DrawText(buf, -260, -195, 0xFF, true, ~0U, 0.75f, /* center-left */ 3);
    }
}

}