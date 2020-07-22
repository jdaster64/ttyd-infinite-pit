#pragma once

#include "randomizer_state.h"

#include <cstdint>

namespace mod::pit_randomizer {

class Randomizer {
public:
    Randomizer();
    
    // Sets up necessary hooks for the randomizer's code to run.
    void Init();
    // Code that runs every frame.
    void Update();
    // Code that runs drawing-related code every frame.
    void Draw();
    
    RandomizerState state_;
};

extern Randomizer* g_Randomizer;
 
}