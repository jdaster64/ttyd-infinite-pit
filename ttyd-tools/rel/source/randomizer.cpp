#include "randomizer.h"

#include "common_functions.h"
#include "common_types.h"
#include "common_ui.h"
#include "patch.h"
#include "randomizer_patches.h"

#include <gc/OSLink.h>
#include <ttyd/dispdrv.h>
#include <ttyd/msgdrv.h>
#include <ttyd/seqdrv.h>
#include <ttyd/seq_title.h>
#include <ttyd/system.h>

#include <cstdint>
#include <cstdio>

namespace mod::pit_randomizer {

// TODO: REMOVE, for TESTING ONLY.
int32_t g_EnemyTypeToTest = 2;
    
namespace  {

using ::gc::OSLink::OSModuleInfo;
using ::ttyd::dispdrv::CameraId;

// Trampoline hooks for patching in custom logic to existing TTYD C functions.
bool (*g_OSLink_trampoline)(OSModuleInfo*, void*) = nullptr;
const char* (*g_msgSearch_trampoline)(const char*) = nullptr;

void DrawTitleScreenInfo() {
    const char* kTitleInfo =
        "Pit of Infinite Trials v0.00 by jdaster64\nPUT GITHUB LINK HERE";
    DrawCenteredTextWindow(
        kTitleInfo, 0, -50, 0xFFu, 0xFFFFFFFFu, 0.75f, 0x000000E5u, 15, 10);
}

// TODO: REMOVE, for TESTING ONLY.
void DrawCurrentlySelectedEnemyType() {
    // D-Pad Up or Down to change the type of enemy to test.
    if (ttyd::system::keyGetButtonTrg(0) & ButtonId::DPAD_UP) {
        ++g_EnemyTypeToTest;
    } else if (ttyd::system::keyGetButtonTrg(0) & ButtonId::DPAD_RIGHT) {
        g_EnemyTypeToTest += 10;
    } else if (ttyd::system::keyGetButtonTrg(0) & ButtonId::DPAD_DOWN) {
        --g_EnemyTypeToTest;
    } else if (ttyd::system::keyGetButtonTrg(0) & ButtonId::DPAD_LEFT) {
        g_EnemyTypeToTest -= 10;
    }
    if (g_EnemyTypeToTest > 105) g_EnemyTypeToTest = 105;
    if (g_EnemyTypeToTest < 1) g_EnemyTypeToTest = 1;
    
    // Print the current enemy type to the screen at all times.
    char buf[16];
    sprintf(buf, "%d", g_EnemyTypeToTest);
    DrawCenteredTextWindow(
        buf, -200, -150, 0xFFu, 0xFFFFFFFFu, 0.75f, 0x000000E5u, 15, 10);
}

}
    
Randomizer::Randomizer() {}

void Randomizer::Init() {
    // Hook functions with custom logic.
    
    g_OSLink_trampoline = patch::hookFunction(
        gc::OSLink::OSLink, [](OSModuleInfo* new_module, void* bss) {
            bool result = g_OSLink_trampoline(new_module, bss);
            if (new_module != nullptr && result) {
                OnModuleLoaded(new_module);
            }
            return result;
        });
        
    g_msgSearch_trampoline = patch::hookFunction(
        ttyd::msgdrv::msgSearch, [](const char* msg_key) {
            const char* replacement = GetReplacementMessage(msg_key);
            if (replacement) return replacement;
            return g_msgSearch_trampoline(msg_key);
        });
        
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
    // TODO: REMOVE, for TESTING ONLY.
    if (InMainGameModes()) {
        RegisterDrawCallback(DrawCurrentlySelectedEnemyType, CameraId::k2d);
    }
}

}