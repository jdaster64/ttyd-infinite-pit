#pragma once

#include <cstdint>

namespace mod::pit_randomizer {

class Randomizer {
public:
    Randomizer();
    
    // Sets up necessary hooks for Shufflizer code to run.
    void Init();
    // Code that runs every frame.
    void Update();
    // Code that runs drawing-related code every frame.
    void Draw();
};

// TODO: REMOVE, for TESTING ONLY.
extern int32_t g_EnemyTypeToTest;
 
}