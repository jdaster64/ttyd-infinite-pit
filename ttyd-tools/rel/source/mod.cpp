#include "mod.h"

#include "patch.h"

#include <ttyd/system.h>
#include <ttyd/mariost.h>
#include <ttyd/fontmgr.h>
#include <ttyd/dispdrv.h>
#include <ttyd/seqdrv.h>
#include <ttyd/seq_logo.h>
#include <ttyd/mario.h>

#include <gc/os.h>

#include <cstdio>
#include <cstring>

namespace mod {
    
namespace {

Mod *gMod = nullptr;

}

void main()
{
	Mod *mod = new Mod();
	mod->init();
}

Mod::Mod()
{
	
}

void Mod::init()
{
	gMod = this;
	
    // Hook the game's main function, so updateEarly runs exactly once per frame
	marioStMain_trampoline_ = 
        patch::hookFunction(ttyd::mariost::marioStMain, [](){
		gMod->updateEarly();
	});

	// Initialize typesetting early (to display mod information on title screen)
	ttyd::fontmgr::fontmgrTexSetup();
	patch::hookFunction(ttyd::fontmgr::fontmgrTexSetup, [](){});

	// Run mod-specific initialization logic.
	randomizer_mod_.Init();
}

void Mod::updateEarly()
{
    // Run mod-specific game logic and drawing code.
	randomizer_mod_.Update();
    randomizer_mod_.Draw();

	// Call original function.
	marioStMain_trampoline_();
}

}