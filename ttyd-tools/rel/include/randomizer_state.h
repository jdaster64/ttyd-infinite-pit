#pragma once

#include <cstdint>

namespace mod::pit_randomizer {

struct RandomizerState {
    enum Options_Flags {
        // Options that do not change seeding.
        MERLEE                  = 0x10,     // Infinite Merlee curses
        SUPERGUARDS_COST_FP     = 0x20,     // Superguards cost 1 FP
        NO_EXP_MODE             = 0x40,     // Start at level 99 w/99 BP, no EXP
        START_WITH_FX           = 0x100,    // Start with Attack FX badges
        START_WITH_NO_ITEMS     = 0x200,    // Start with a preset set of items
        SWITCH_PARTY_COST_FP    = 0xc00,    // Switching partners costs 1-3 FP
        YOSHI_COLOR_SELECT      = 0x1000,   // Manually selecting Yoshi color
        SHINE_SPRITES_MARIO     = 0x2000,   // Increase max SP w/Shine Sprites
        ALWAYS_ENABLE_AUDIENCE  = 0x4000,   // Always enable SP features
        WEAKER_RUSH_BADGES      = 0x8000,   // Rush badges increase ATK less
        
        // Options that DO change seeding.
        NUM_CHEST_REWARDS       = 0x7,      // 0 ~ 5 (0 = random); changes seeds
        POST_100_SCALING        = 0x30000,  // Sets HP / ATK scale after fl. 100
        START_WITH_PARTNERS     = 0x40000,  // Start with all base-rank partners
        START_WITH_SWEET_TREAT  = 0x80000,  // Start with Sweet Treat
        BATTLE_REWARD_MODE      = 0x300000, // Alternate battle reward schemes
        
        // Individual modes of composite options.
        POST_100_HP_SCALING     = 0x10000,
        POST_100_ATK_SCALING    = 0x20000,
        CONDITION_DROPS_HELD    = 0x100000, // Held item drop gated by condition
        NO_HELD_ITEMS           = 0x200000, // Condition items only, none held
        
        // Options that aren't controlled by flags (used by menu state).
        CHANGE_PAGE             = -100,
        HP_MODIFIER             = -101,     // Multiplier for enemy HP
        ATK_MODIFIER            = -102,     // Multiplier for enemy ATK
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
    
    // Changes the selected menu option; `change` controls how to change it,
    // -1 / +1 for decreasing / increasing, 0 for toggling or advancing.
    // Will also make any other changes necessary for the option to function
    // (e.g. changing Mario's level and BP for No-EXP mode).
    void ChangeOption(int32_t option, int32_t change);
    // Returns the appropriate value for the specified option
    // (e.g. 0 or 1 for boolean options, or the percent for HP/ATK modifiers).
    int32_t GetOptionValue(int32_t option) const;
    // Writes the menu strings for the specified option to the supplied buffers,
    // and may optionally write a special color to use (set to 0 if default).
    void GetOptionStrings(
        int32_t option, char* name, char* value, uint32_t* color) const;
    
    // Returns the number of Star Powers obtained.
    int32_t StarPowersObtained() const;
    // Returns whether Star Power / audience functions should be enabled.
    bool StarPowerEnabled() const;
    
    // Returns a base-64-esque encoding of this file's user-selectable options.
    const char* GetEncodedOptions() const;
    // Gets a string containing the player's play stats on this save file.
    // Returns false if no play stats are present on this save file.
    bool GetPlayStats(char* out_buf) const;
} __attribute__((__packed__));

static_assert(sizeof(RandomizerState) <= 0x38);

}