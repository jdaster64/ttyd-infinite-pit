#pragma once

#include <cstdint>

namespace mod::pit_randomizer {

struct RandomizerState {
    uint8_t     version_;
    uint8_t     partner_upgrades_[7];
    uint32_t    rng_state_;
    int32_t     floor_;
    uint32_t    reward_flags_;
    int16_t     charlieton_items_[6];
    uint32_t    debug_[4];

    // Initializes the randomizer state based on the current save file.
    // Returns whether the randomizer was successfully initialized.
    bool Load(bool new_save);
    // Copies the randomizer state to g_MarioSt so it can be saved.
    void Save();
    
    // Seeds the randomizer's RNG state with an input string.
    void SeedRng(const char* str);
    // Increments the RNG state and returns a value in a range [0, n).
    uint32_t Rand(uint32_t range);
} __attribute__((__packed__));

static_assert(sizeof(RandomizerState) <= 0x38);

}