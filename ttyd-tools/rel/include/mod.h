#pragma once

#include "randomizer_state.h"

#include <cstdint>

namespace mod::infinite_pit {

class Mod {
public:
	Mod();
    
    // Sets up necessary hooks for the randomizer's code to run.
    void Init();
    // Code that runs every frame.
    void Update();
    // Code that runs drawing-related code every frame.
    void Draw();
    
    StateManager state_;
};

extern Mod* g_Mod;

}