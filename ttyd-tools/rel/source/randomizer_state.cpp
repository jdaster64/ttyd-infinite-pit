#include "randomizer_state.h"

#include <cstdint>

namespace mod::pit_randomizer {

void RandomizerState::InitializeRandomizerState(bool new_save) {
    // TODO: Load from save file.
    version_ = 1;
    for (int32_t i = 0; i < 7; ++i) partner_upgrades_[i] = 0;
    rng_state_ = 1;
    floor_ = 0;
    reward_flags_ = 0x00000000;
    debug_[0] = 2;
}
    
void RandomizerState::SeedRng(const char* str) {
    uint32_t hash = 0;
    for (const char* c = str; *c != 0; ++c) {
        hash = 37 * hash + *c;
    }
    rng_state_ = hash;
}

uint32_t RandomizerState::Rand(uint32_t range) {
    rng_state_ = rng_state_ * 0x41c64e6d + 12345;
    return ((rng_state_ >> 16) & 0x7fff) % range;
}

}