#include "mod.h"

#include "common_ui.h"
#include "patch.h"
#include "randomizer_cheats.h"
#include "randomizer_menu.h"
#include "randomizer_patches.h"
#include "randomizer_title.h"

#include <ttyd/dispdrv.h>
#include <ttyd/fontmgr.h>
#include <ttyd/mariost.h>

#include <cstdint>

namespace mod {

void main() {
	infinite_pit::Mod* mod = new infinite_pit::Mod();
	mod->Init();
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
    // TODO: Split into more reasonable siloed chunks.
    ApplyCorePatches();
    ApplyEnemyStatChangePatches();
    ApplyWeaponLevelSelectionPatches();
    ApplyItemAndAttackPatches();
    ApplyPlayerStatTrackingPatches();
    ApplyMiscPatches();
}

void Mod::Update() {
    TitleScreenManager::Update();
    CheatsManager::Update();
    MenuManager::Update();
}

void Mod::Draw() {
    RegisterDrawCallback(TitleScreenManager::Draw, CameraId::k2d);
    RegisterDrawCallback(CheatsManager::Draw, CameraId::kDebug3d);
    RegisterDrawCallback(MenuManager::Draw, CameraId::k2d);
}

}