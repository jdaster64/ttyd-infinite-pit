#pragma once

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
 
}