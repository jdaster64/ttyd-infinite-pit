#include "randomizer_state.h"

#include <cstdint>

namespace mod::pit_randomizer {

void RandomizerState::InitializeRandomizerState(bool new_save) {
    // TODO: Load from save file.
    version_ = 1;
    for (int32_t i = 0; i < 7; ++i) partner_upgrades_[i] = 0;
    rng_state_ = 1;
    rng_state_floor_start_ = 1;
    floor_ = 1;
    reward_flags_ = 0x00000000;
    debug_[0] = 2;
}

}