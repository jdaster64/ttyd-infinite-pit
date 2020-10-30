#pragma once

#include <cstdint>

namespace mod::pit_randomizer {

class RandomizerMenu {
public:
    RandomizerMenu();
    
    // Initializes the menu variables.
    void Init();
    // Code that runs every frame.
    void Update();
    // Code that runs drawing-related code every frame.
    void Draw();
    
    // Determines whether individual pages of the options menu can be accessed.
    // Used to unlock the two pages of bonus options.
    void SetMenuPageVisibility(int32_t page, bool enabled);
};
 
}