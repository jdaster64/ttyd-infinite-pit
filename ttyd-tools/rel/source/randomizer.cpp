#include "randomizer.h"

#include "common_functions.h"
#include "common_types.h"
#include "common_ui.h"
#include "patch.h"
#include "randomizer_data.h"
#include "randomizer_patches.h"
#include "randomizer_state.h"

#include <gc/OSLink.h>
#include <ttyd/battle_actrecord.h>
#include <ttyd/battle_enemy_item.h>
#include <ttyd/battle_event_cmd.h>
#include <ttyd/battle_information.h>
#include <ttyd/battle_seq.h>
#include <ttyd/battle_unit.h>
#include <ttyd/cardmgr.h>
#include <ttyd/dispdrv.h>
#include <ttyd/event.h>
#include <ttyd/evtmgr.h>
#include <ttyd/msgdrv.h>
#include <ttyd/seqdrv.h>
#include <ttyd/seq_title.h>
#include <ttyd/statuswindow.h>
#include <ttyd/system.h>

#include <cstdint>
#include <cstdio>
#include <cstring>

namespace mod::pit_randomizer {
    
Randomizer* g_Randomizer = nullptr;
    
namespace {

using ::gc::OSLink::OSModuleInfo;
using ::ttyd::battle_unit::BattleWorkUnit;
using ::ttyd::dispdrv::CameraId;
using ::ttyd::evtmgr::EvtEntry;
using ::ttyd::npcdrv::FbatBattleInformation;
using ::ttyd::seqdrv::SeqIndex;

// Trampoline hooks for patching in custom logic to existing TTYD C functions.
void (*g_stg0_00_init_trampoline)(void) = nullptr;
void (*g_cardCopy2Main_trampoline)(int32_t) = nullptr;
bool (*g_OSLink_trampoline)(OSModuleInfo*, void*) = nullptr;
const char* (*g_msgSearch_trampoline)(const char*) = nullptr;
void (*g_BtlActRec_JudgeRuleKeep_trampoline)(void) = nullptr;
void (*g__rule_disp_trampoline)(void) = nullptr;
void (*g_BattleInformationSetDropMaterial_trampoline)(FbatBattleInformation*) = nullptr;
int32_t (*g_btlevtcmd_ConsumeItem_trampoline)(EvtEntry*, bool) = nullptr;
int32_t (*g_btlevtcmd_GetConsumeItem_trampoline)(EvtEntry*, bool) = nullptr;
void* (*g_BattleEnemyUseItemCheck_trampoline)(BattleWorkUnit*) = nullptr;
void (*g_seqSetSeq_trampoline)(SeqIndex, const char*, const char*) = nullptr;
void (*g_statusWinDisp_trampoline)(void) = nullptr;
void (*g_gaugeDisp_trampoline)(double, double, int32_t) = nullptr;

bool g_CueGameOver = false;
bool g_DrawDebug = false;

void DrawTitleScreenInfo() {
    // TODO: Update with final text before release.
    const char* kTitleInfo =
        "PM:TTYD Infinite Pit v0.00 r4 by jdaster64\nGuide / GitHub link: TBA";
    DrawCenteredTextWindow(
        kTitleInfo, 0, -50, 0xFFu, true, 0xFFFFFFFFu, 0.75f, 0x000000E5u, 15, 10);
}

// Handles printing stuff to the screen for debugging purposes; no longer used.
void DrawDebuggingFunctions() {
    uint32_t& enemyTypeToTest = g_Randomizer->state_.debug_[0];
    
    // D-Pad Up or Down to change the type of enemy to test.
    if (ttyd::system::keyGetButtonTrg(0) & ButtonId::DPAD_UP) {
        ++enemyTypeToTest;
    } else if (ttyd::system::keyGetButtonTrg(0) & ButtonId::DPAD_RIGHT) {
        enemyTypeToTest += 10;
    } else if (ttyd::system::keyGetButtonTrg(0) & ButtonId::DPAD_DOWN) {
        --enemyTypeToTest;
    } else if (ttyd::system::keyGetButtonTrg(0) & ButtonId::DPAD_LEFT) {
        enemyTypeToTest -= 10;
    }
    if (enemyTypeToTest > 101) enemyTypeToTest = 101;
    if (enemyTypeToTest < 1) enemyTypeToTest = 1;
    
    // Print the current enemy type to the screen at all times.
    char buf[16];
    sprintf(buf, "%d", enemyTypeToTest);
    DrawCenteredTextWindow(
        buf, -200, -150, 0xFFu, true, 0xFFFFFFFFu, 0.75f, 0x000000E5u, 15, 10);
}

}
    
Randomizer::Randomizer() {}

void Randomizer::Init() {
    g_Randomizer = this;
    
    // Hook functions with custom logic.
    
    g_stg0_00_init_trampoline = patch::hookFunction(
        ttyd::event::stg0_00_init, []() {
            // Replaces existing logic, includes loading the randomizer state.
            OnFileLoad(/* new_file = */ true);
        });
        
    g_cardCopy2Main_trampoline = patch::hookFunction(
        ttyd::cardmgr::cardCopy2Main, [](int32_t save_file_number) {
            g_cardCopy2Main_trampoline(save_file_number);
            OnFileLoad(/* new_file = */ false);
            // If invalid randomizer file loaded, give the player a Game Over.
            if (!g_Randomizer->state_.Load(/* new_save = */ false)) {
                g_CueGameOver = true;
            }
        });
    
    g_OSLink_trampoline = patch::hookFunction(
        gc::OSLink::OSLink, [](OSModuleInfo* new_module, void* bss) {
            bool result = g_OSLink_trampoline(new_module, bss);
            if (new_module != nullptr && result) {
                OnModuleLoaded(new_module);
            }
            return result;
        });
    
    g_seqSetSeq_trampoline = patch::hookFunction(
        ttyd::seqdrv::seqSetSeq, 
        [](SeqIndex seq, const char* mapName, const char* beroName) {
            OnEnterExitBattle(/* is_start = */ seq == SeqIndex::kBattle);
            // Check for failed file load.
            if (g_CueGameOver) {
                seq = SeqIndex::kGameOver;
                mapName = reinterpret_cast<const char*>(1);
                beroName = 0;
                g_CueGameOver = false;
            } else if (
                seq == SeqIndex::kMapChange && !strcmp(mapName, "aaa_00") && 
                !strcmp(beroName, "prologue")) {
                // If loading a new file, load the player into the pre-Pit room.
                mapName = "tik_06";
                beroName = "e_bero";
            }
            g_seqSetSeq_trampoline(seq, mapName, beroName);
        });
        
    g_msgSearch_trampoline = patch::hookFunction(
        ttyd::msgdrv::msgSearch, [](const char* msg_key) {
            const char* replacement = GetReplacementMessage(msg_key);
            if (replacement) return replacement;
            return g_msgSearch_trampoline(msg_key);
        });
        
    g_BtlActRec_JudgeRuleKeep_trampoline = patch::hookFunction(
        ttyd::battle_actrecord::BtlActRec_JudgeRuleKeep, []() {
            g_BtlActRec_JudgeRuleKeep_trampoline();
            CheckBattleCondition();
        });
        
    g__rule_disp_trampoline = patch::hookFunction(
        ttyd::battle_seq::_rule_disp, []() {
            // Replaces the original logic completely.
            DisplayBattleCondition();
        });
        
    g_BattleInformationSetDropMaterial_trampoline = patch::hookFunction(
        ttyd::battle_information::BattleInformationSetDropMaterial,
        [](FbatBattleInformation* fbat_info) {
            // Replaces the original logic completely.
            GetDropMaterials(fbat_info);
        });
        
    g_btlevtcmd_ConsumeItem_trampoline = patch::hookFunction(
        ttyd::battle_event_cmd::btlevtcmd_ConsumeItem,
        [](EvtEntry* evt, bool isFirstCall) {
            EnemyConsumeItem(evt);
            return g_btlevtcmd_ConsumeItem_trampoline(evt, isFirstCall);
        });
        
    g_btlevtcmd_GetConsumeItem_trampoline = patch::hookFunction(
        ttyd::battle_event_cmd::btlevtcmd_GetConsumeItem,
        [](EvtEntry* evt, bool isFirstCall) {
            if (GetEnemyConsumeItem(evt)) return 2;
            return g_btlevtcmd_GetConsumeItem_trampoline(evt, isFirstCall);
        });
        
    g_BattleEnemyUseItemCheck_trampoline = patch::hookFunction(
        ttyd::battle_enemy_item::BattleEnemyUseItemCheck,
        [](BattleWorkUnit* unit) {
            void* evt_code = g_BattleEnemyUseItemCheck_trampoline(unit);
            if (!evt_code) {
                evt_code = EnemyUseAdditionalItemsCheck(unit);
            }
            return evt_code;
        });
        
    g_statusWinDisp_trampoline = patch::hookFunction(
        ttyd::statuswindow::statusWinDisp, []() {
            g_statusWinDisp_trampoline();
            DisplayStarPowerNumber();
        });
        
    g_gaugeDisp_trampoline = patch::hookFunction(
        ttyd::statuswindow::gaugeDisp, [](double x, double y, int32_t sp) {
            // Replaces the original logic completely.
            DisplayStarPowerOrbs(x, y, sp);
        });
        
    ApplyEnemyStatChangePatches();
    ApplyWeaponLevelSelectionPatches();
    ApplyItemAndAttackPatches();
    ApplyMiscPatches();
}

void Randomizer::Update() {}

void Randomizer::Draw() {
    if (CheckSeq(ttyd::seqdrv::SeqIndex::kTitle)) {
        const uint32_t curtain_state = *reinterpret_cast<uint32_t*>(
            reinterpret_cast<uintptr_t>(ttyd::seq_title::seqTitleWorkPointer2)
            + 0x8);
        if (curtain_state >= 2 && curtain_state < 12) {
            // Curtain is not fully down; draw title screen info.
            RegisterDrawCallback(DrawTitleScreenInfo, CameraId::k2d);
        }
    }
    if (InMainGameModes() && g_DrawDebug) {
        RegisterDrawCallback(DrawDebuggingFunctions, CameraId::k2d);
    }
}

}