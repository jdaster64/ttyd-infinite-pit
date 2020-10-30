#pragma once

#include <cstdint>

namespace mod::pit_randomizer {

struct RandomizerState {
    enum Options_Flags {
        // Options that do not change seeding.
        MERLEE                  = 0x10,     // Infinite Merlee curses
        SUPERGUARDS_COST_FP     = 0x20,     // Superguards cost 1 FP
        NO_EXP_MODE             = 0xC0,     // Start at level 99, gain no EXP
        START_WITH_FX           = 0x100,    // Start with Attack FX badges
        START_WITH_NO_ITEMS     = 0x200,    // Start with a preset set of items
        SWITCH_PARTY_COST_FP    = 0xc00,    // Switching partners costs 1-3 FP
        YOSHI_COLOR_SELECT      = 0x1000,   // Manually selecting Yoshi color
        SHINE_SPRITES_MARIO     = 0x2000,   // Increase max SP w/Shine Sprites
        ALWAYS_ENABLE_AUDIENCE  = 0x4000,   // Always enable SP features
        
        // Options that DO change seeding.
        NUM_CHEST_REWARDS       = 0x7,      // 1 ~ 5 per chest (0 = varies)
        POST_100_SCALING        = 0x30000,  // Sets HP / ATK scale after fl. 100
        START_WITH_PARTNERS     = 0x40000,  // Start with all base-rank partners
        START_WITH_SWEET_TREAT  = 0x80000,  // Start with Sweet Treat
        BATTLE_REWARD_MODE      = 0x300000, // Alternate battle reward schemes
        
        // Bonus options.
        WEAKER_RUSH_BADGES      = 0x8000,       // Rush badges increase ATK less
        CAP_BADGE_EVASION       = 0x400000,     // Max evasion from badge capped
        HP_FP_DRAIN_PER_HIT     = 0x800000,     // Restore 1 HP/FP per hit
        SWAP_CO_PL_SP_COST      = 0x8000000,    // Swap Clock Out and Power Lift
        STAGE_HAZARD_OPTIONS    = 0x7000000,    // Stage hazard frequency, etc.
        DAMAGE_RANGE            = 0x30000000,   // All damage has random ranges
        AUDIENCE_ITEMS_RANDOM   = 0x40000000,   // Random audience item throws
        
        // Individual modes of composite options.
        NO_EXP_99_BP            = 0x40,
        NO_EXP_INFINITE_BP      = 0x80,
        POST_100_HP_SCALING     = 0x10000,
        POST_100_ATK_SCALING    = 0x20000,
        CONDITION_DROPS_HELD    = 0x100000, // Held item drop gated by condition
        NO_HELD_ITEMS           = 0x200000, // Condition items only, none held
        ALL_HELD_ITEMS          = 0x300000, // All held items drop + bonus
        HAZARD_RATE_HIGH        = 0x1000000,
        HAZARD_RATE_LOW         = 0x2000000,
        HAZARD_RATE_NO_FOG      = 0x3000000,
        HAZARD_RATE_OFF         = 0x4000000,
        DAMAGE_RANGE_25         = 0x10000000,   // +/-25%, in increments of 5%
        DAMAGE_RANGE_50         = 0x20000000,   // +/-50%, in increments of 10%
        
        // Options that aren't controlled by flags (used by menu state).
        CHANGE_PAGE             = -100,
        HP_MODIFIER             = -101,     // Multiplier for enemy HP
        ATK_MODIFIER            = -102,     // Multiplier for enemy ATK
        INVALID_OPTION          = -999,
    };
    
    // Enum of stats that get tracked point by point throughout a run.
    enum PlayStats {
        TURNS_SPENT = 0,
        TIMES_RAN_AWAY,
        ENEMY_DAMAGE,
        PLAYER_DAMAGE,
        ITEMS_USED,
        COINS_EARNED,
        COINS_SPENT,
        SHINE_SPRITES_USED,
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
    
    // Holds the value of various stats tracked over the course of a file
    // (total turns in battle, number of items used, etc.)
    uint8_t     play_stats_[20];

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
    
    // Get or update various stats tracked over the course of a file.
    void IncrementPlayStat(PlayStats stat, int32_t amount = 1);
    int32_t GetPlayStat(PlayStats stat) const;
    // Returns a base-64-esque encoding of this file's user-selectable options.
    const char* GetEncodedOptions() const;
    // Gets a string containing the player's play stats on this save file.
    // Returns false if no play stats are present on this save file.
    bool GetPlayStatsString(char* out_buf) const;
    
    // Saves the current clock time (used for calculating RTA time since start).
    void SaveCurrentTime(bool pit_start = false);
    // Get the current RTA play time as a string.
    // Will return empty string if the start time was unset or incompatible.
    const char* GetCurrentTimeString();
} __attribute__((__packed__));

static_assert(sizeof(RandomizerState) <= 0x3c);

}