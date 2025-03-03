#include "mod.h"

#include "common_ui.h"
#include "mod_achievements.h"
#include "mod_cheats.h"
#include "mod_debug.h"
#include "mod_gfxtest.h"
#include "mod_loading.h"
#include "mod_menu.h"
#include "mod_title.h"
#include "patch.h"
#include "patches_apply.h"

#include <gc/OSArena.h>
#include <ttyd/dispdrv.h>
#include <ttyd/fontmgr.h>
#include <ttyd/mariost.h>

#include <cstdint>
#include <cstring>

namespace mod {

uint32_t initMod() {
	infinite_pit::Mod* mod = new infinite_pit::Mod();
	mod->Init();
    
    return 0;
}

void main() {
    // Check if using v3 of the REL Loader
    if (ttyd::OSArena::__OSArenaLo == ttyd::OSArena::__OSArenaHi) {
        // Not using v3 of the REL Loader, so init stuff now
        initMod();
    } else {
        // Using v3 of the REL Loader, so hook initMod at the address
        // where REL Loader V1/V2 would normally run
#ifdef TTYD_US
        constexpr uint32_t marioStInitAddress = 0x8006FE38;
#elif defined TTYD_JP
        constexpr uint32_t marioStInitAddress = 0x8006EBD8;
#elif defined TTYD_EU
        constexpr uint32_t marioStInitAddress = 0x800710F4;
#endif
        patch::writeBranchBL(
            reinterpret_cast<void*>(marioStInitAddress),
            reinterpret_cast<void*>(initMod));
    }
}

}

namespace mod::infinite_pit {

// Global instance of Mod class.
Mod* g_Mod = nullptr;
    
namespace {
    
using ::ttyd::dispdrv::CameraId;
    
// Main trampoline to call once-a-frame update logic from.
void (*marioStMain_trampoline_)() = nullptr;

}

Mod::Mod() {}

void Mod::Init() {
    // Initialize global mod instance variable.
	g_Mod = this;
    
    // Clear the mod's state completely.
    memset(&state_, 0, sizeof(state_));
	
    // Hook the game's main function, so Update runs exactly once per frame.
	marioStMain_trampoline_ = patch::hookFunction(
        ttyd::mariost::marioStMain, [](){
            // Call the mod's update and draw functions, then run game code.
            g_Mod->Update();
            g_Mod->Draw();
            marioStMain_trampoline_();
        });

	// Initialize typesetting early (to display mod information on title screen)
	ttyd::fontmgr::fontmgrTexSetup();
	patch::hookFunction(ttyd::fontmgr::fontmgrTexSetup, [](){});
    
    // Hook / patch other functions with custom logic.
    ApplyAllFixedPatches();
}

void Mod::Update() {
    DebugManager::Update();
    LoadingManager::Update();
    CheatsManager::Update();
    AchievementsManager::Update();
    TitleScreenManager::Update();
    MenuManager::Update();
}

void Mod::Draw() {
    RegisterDrawCallback(DebugManager::Draw,        CameraId::kDebug3d);
    RegisterDrawCallback(LoadingManager::Draw,      CameraId::kDebug3d);
    RegisterDrawCallback(CheatsManager::Draw,       CameraId::kDebug3d);
    RegisterDrawCallback(AchievementsManager::Draw, CameraId::kDebug3d);
    RegisterDrawCallback(TitleScreenManager::Draw,  CameraId::k2d);
    RegisterDrawCallback(MenuManager::Draw,         CameraId::k2d);
}

}