#pragma once

#include "timer.h"
#include "keyboard.h"

#include <cstdint>

#include "randomizer.h"

namespace mod {

class Mod
{
public:
	Mod();
	void init();
	
private:
	void updateEarly();
    
    // Encapsulates all the main mod logic.
    pit_randomizer::Randomizer randomizer_mod_;
    
    // Main trampoline to call once-a-frame update logic from.
    void (*marioStMain_trampoline_)() = nullptr;
};

}