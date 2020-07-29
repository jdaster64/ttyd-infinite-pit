#pragma once

#include <cstdint>

namespace mod::pit_randomizer {

struct RandomizerState {
    uint8_t     version_;
    uint8_t     partner_upgrades_[7];
    uint32_t    rng_state_;
    uint32_t    rng_state_floor_start_;
    int32_t     floor_;
    uint32_t    reward_flags_;
    uint32_t    debug_[4];
        
    // Initializes the randomizer state based on the current save file.
    void InitializeRandomizerState(bool new_save);
    // Seeds the randomizer's RNG state with an input string.
    void SeedRng(const char* str);
    // Increments the RNG state and returns a value in a range [0, n).
    uint32_t Rand(uint32_t range);
} __attribute__((__packed__));

static_assert(sizeof(RandomizerState) <= 0x38);

}