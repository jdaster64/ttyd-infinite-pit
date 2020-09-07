#pragma once

#include <cstdint>

namespace mod::pit_randomizer {

struct RandomizerState {
    enum Options_Flags {
        NUM_CHEST_REWARDS       = 0x7,      // 0 ~ 5 (0 = random); changes seeds
        MERLEE                  = 0x10,     // Infinite Merlee curses
        SUPERGUARDS_COST_FP     = 0x20,     // Superguards cost 1 FP
        NO_EXP_MODE             = 0x40,     // Start at level 99 w/99 BP, no EXP
        START_WITH_FX           = 0x100,    // Start with Attack FX badges
        START_WITH_ITEMS        = 0x200,    // Start with a preset set of items
        SWITCH_PARTY_COST_FP    = 0x400,    // Switching partners costs 1 FP
    };

    // Save file revision; makes it possible to add fields while maintaining
    // backwards compatibility, and detect when a vanilla file is loaded.
    // Current version = 2, compatible versions = 1 ~ 2.
    uint8_t     version_;
    
    // Game state.
    uint8_t     partner_upgrades_[7];
    uint32_t    rng_state_;
    int32_t     floor_;
    uint32_t    reward_flags_;
    // Used for reloading a save.
    uint32_t    saved_rng_state_;
    uint8_t     load_from_save_;
    uint8_t     disable_partner_badges_in_shop_;
    uint8_t     unused_[6];
    
    // Options that can be set by the player at the start of a file.
    int16_t     hp_multiplier_;
    int16_t     atk_multiplier_;
    uint32_t    options_;   // Bitfield of Options_Flags.
    
    // State for debugging features.
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