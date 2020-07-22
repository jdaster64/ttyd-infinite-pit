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
} __attribute__((__packed__));

static_assert(sizeof(RandomizerState) <= 0x30);

}