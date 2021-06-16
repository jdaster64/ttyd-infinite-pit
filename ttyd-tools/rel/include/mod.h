#pragma once

#include "mod_state.h"

#include <cstdint>

namespace mod::infinite_pit {

class Mod {
public:
	Mod();
    
    // Sets up necessary hooks for the mod's code to run.
    void Init();
    // Code that runs every frame.
    void Update();
    // Code that runs drawing-related code every frame.
    void Draw();
    
    // TODO: Rename back to "state_" once migration to StateManager_v2 is done.
    StateManager_v2 ztate_;
};

extern Mod* g_Mod;

}