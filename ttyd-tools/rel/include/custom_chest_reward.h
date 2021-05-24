#pragma once

#include <cstdint>

namespace mod::infinite_pit {
    
// Picks a reward for a chest, updating the randomizer state accordingly.
// Reward is either an item/badge (if the result > 0) or a partner (-1 to -7).
int16_t PickChestReward();

}