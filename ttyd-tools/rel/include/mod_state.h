#pragma once

#include <cstdint>

namespace mod::infinite_pit {

class StateManager {
public:
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
        
        // Options added from v1.40 onward (can't fit in original flag field).
        PARTNER_STARTING_RANK   = -14010,
        DANGER_PERIL_BY_PERCENT = -14020,
        MAX_BADGE_MOVE_LEVEL    = -14030,
        MAX_MOVE_LEVEL_1X       = -14031,
        MAX_MOVE_LEVEL_2X       = -14032,
        MAX_MOVE_LEVEL_RANK     = -14033,
        MAX_MOVE_LEVEL_INFINITE = -14034,
        RANK_UP_REQUIREMENT     = -14040,
        RANK_UP_NORMAL          = -14041,
        RANK_UP_EARLIER         = -14042,
        RANK_UP_BY_FLOOR        = -14043,
        RANK_UP_ALWAYS_MAX      = -14044,
        
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
    // Current version = 3, compatible versions = 1 ~ 3.
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
    // Reserved / used for options introduced in v1.40.
    uint8_t     unused_[5];
    uint8_t     options_v1_40_;
    
    // Options that can be set by the player at the start of a file.
    int16_t     hp_multiplier_;
    int16_t     atk_multiplier_;
    uint32_t    options_;   // Bitfield of Options_Flags.
    
    // Holds the value of various stats tracked over the course of a file
    // (total turns in battle, number of items used, etc.)
    uint8_t     play_stats_[20];

    // Initializes the mod's state based on the current save file.
    // Returns whether the mod was successfully initialized.
    bool Load(bool new_save);
    // Copies the mod's state to g_MarioSt so it can be saved.
    void Save();
    
    // Seeds the mod's RNG state with an input string.
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

static_assert(sizeof(StateManager) <= 0x3c);

class StateManager_v2 {
public:
    // Save file revision; makes it possible to add fields while maintaining
    // backwards compatibility, and detect when a vanilla file is loaded.
    // Current version = 4, compatible versions = 4.
    uint8_t     version_;
    
    // Game state / progression.
    uint8_t     partner_upgrades_[7];   // Includes Super- and Ultra- rank.
    int32_t     floor_;
    uint16_t    star_power_levels_;
    uint8_t     partners_unlocked_;
    uint8_t     reserved_0x0f_;         // Padding byte.
    // RNG state.
    uint32_t    filename_seed_;
    uint16_t    rng_sequences_[22];
    // Game timers.
    uint64_t    pit_start_time_;
    uint64_t    last_save_time_;
    // Option values & gameplay stats; all accessed by new Options enum.
    uint32_t    option_flags_[4];       // Last 4 bytes reserved for cosmetics.
    uint8_t     option_bytes_[32];
    uint8_t     play_stats_  [64];

    // Initializes the mod's state based on the current save file.
    // Returns whether the mod was successfully initialized.
    bool Load(bool new_save);
    // Copies the mod's state to g_MarioSt so it can be saved.
    void Save();
    
    // Gets the maximum level of a particular Star Power.
    int32_t GetStarPowerLevel(int32_t star_power_type) const;
    
    // Fetches a random value from the desired sequence (using the RngSequence
    // enum), returning a value in the range [0, range). If `sequence` is not
    // a valid enum value, returns a random value using ttyd::system::irand().
    uint32_t Rand(uint32_t range, int32_t sequence = -1);
    
    // Set all options / play stats to 0.
    void ClearAllOptions();
    // Set all non-cosmetic options to their default values.
    void SetDefaultOptions();
    // Changes the selected option/stat by the given amount.
    // Flag options will wrap by default, and increment if `change` is 0;
    // numeric options will saturate and will not increment if `change` is 0.
    void ChangeOption(int32_t option, int32_t change = 1);
    // Sets the selected option/stat to a particular value.
    // If option is set to an OPTVAL_xxx, 'value' is ignored.
    void SetOption(int32_t option, int32_t value = 0);
    // Returns the appropriate flag / numeric value for the specified option.
    // Returns -1 if the option doesn't exist or if the wrong type is requested.
    int32_t GetOptionValue(int32_t option) const;
    int32_t GetOptionFlagValue(int32_t option) const;
    int32_t GetOptionNumericValue(int32_t option) const;
    // Checks whether a flag option has a particular value.
    bool CheckOptionValue(int32_t option_value) const;
    // Returns strings for the given menu option and its current value,
    // and whether the value is default / affects seeding in any way.
    void GetOptionStrings(
        int32_t option, char* name_buf, char* value_buf,
        bool* is_default, bool* affects_seeding) const;

    // Returns a string representing all of this file's user-selectable options.
    const char* GetEncodedOptions() const;
    // Gets a string containing all the saved gameplay stats on this save file.
    // Returns false if no play stats are present on this save file.
    bool GetPlayStatsString(char* out_buf) const;
    
    // Saves the current clock time (used for calculating RTA time since start).
    void SaveCurrentTime(bool pit_start = false);
    // Get the current RTA play time as a string.
    // Will return the empty string if the start time was unset or incompatible.
    const char* GetCurrentTimeString();
} __attribute__((__packed__));

static_assert(sizeof(StateManager_v2) <= 0x120);

// An enumeration of all flag options, flag option values, numeric options,
// and gameplay stats used in StateManager_v2.
//
// Types of options / encoding:
//  - OPT_BLAH:     Flag option type    (0x1 XX Y W ZZZ);
//      Represents the range of bits [XX, XX+Y) and having ZZZ possible values.
//      Bits used for a single option should not cross a word boundary.
//  - OPTVAL_BLAH:  Flag option value   (0x1 XX Y W ZZZ);
//      Represents the range of bits [XX, XX+Y) being set to the value ZZZ.
//  - OPTNUM_BLAH:  Bytes option value  (0x3 XX Y W ZZZ);
//      Represents the range of option_bytes_ from offset [XX, XX+Y).
//  - STAT_BLAH:    Play stats value    (0x4 XX Y W ZZZ);
//      Represents the range of play_stats_ from offset [XX, XX+Y).
//  - Negative values: Not valid for flags/options/stats, may be used for
//    other purposes.
//
//  Use of "W" bits:
//      If (W & 7) == 0 or 1: 
//          Numeric options / play stats are clamped to the range [0 or 1, ZZZ].
//      If (W & 7) == 2:
//          Numeric options / play stats are clamped to Z digits (from 1-9).
//      If (W & 7) == 3:
//          Numeric options / play stats are clamped to +/- Z digits (from 1-9).
//      If W & 8 != 0, the option affects seeding.
// 
// Intentionally placed in global namespace for convenience.
enum Options_v2 {
    // Non-cosmetic (menu-visible) flag-based options.
    // How many rewards are obtained per chest (1 to 9, or 0 = random).
    OPT_CHEST_REWARDS           = 0x1'00'4'8'009,
    OPTVAL_CHEST_REWARDS_RANDOM = 0x2'00'4'8'000,
    // If enabled, Mario starts at level 99 with base HP/FP and 99/Infinite BP.
    OPT_NO_EXP_MODE             = 0x1'04'2'0'003,
    OPTVAL_NO_EXP_MODE_OFF      = 0x2'04'2'0'000,
    OPTVAL_NO_EXP_MODE_ON       = 0x2'04'2'0'001,
    OPTVAL_NO_EXP_MODE_INFINITE = 0x2'04'2'0'002,
    // How enemies hold / drop items, and whether there are challenges.
    OPT_BATTLE_REWARD_MODE      = 0x1'06'2'8'004,
    OPTVAL_DROP_STANDARD        = 0x2'06'2'8'000,   // 1 drop + bonus chance
    OPTVAL_DROP_HELD_FROM_BONUS = 0x2'06'2'8'001,   // held drop from bonus
    OPTVAL_DROP_NO_HELD_W_BONUS = 0x2'06'2'8'002,   // no held, only bonus
    OPTVAL_DROP_ALL_HELD        = 0x2'06'2'8'003,   // all drop + bonus chance
    // How partners are obtained.
    OPT_PARTNERS_OBTAINED       = 0x1'08'2'8'004,
    OPTVAL_PARTNERS_ALL_REWARDS = 0x2'08'2'8'000,   // all from chest rewards
    OPTVAL_PARTNERS_ALL_START   = 0x2'08'2'8'001,   // all at start
    OPTVAL_PARTNERS_ONE_START   = 0x2'08'2'8'002,   // one at start, rest later
    OPTVAL_PARTNERS_NEVER       = 0x2'08'2'8'003,   // no partners at all
    // Which rank partners start at.
    OPT_PARTNER_RANK            = 0x1'0a'2'0'003,
    OPTVAL_PARTNER_RANK_NORMAL  = 0x2'0a'2'0'000,
    OPTVAL_PARTNER_RANK_SUPER   = 0x2'0a'2'0'001,
    OPTVAL_PARTNER_RANK_ULTRA   = 0x2'0a'2'0'002,
    // How high a level of each badge-based move can be used.
    OPT_BADGE_MOVE_LEVEL        = 0x1'0c'2'0'004,
    OPTVAL_BADGE_MOVE_1X        = 0x2'0c'2'0'000,   // up to 1x badges equipped
    OPTVAL_BADGE_MOVE_2X        = 0x2'0c'2'0'001,   // up to 2x badges equipped
    OPTVAL_BADGE_MOVE_RANK      = 0x2'0c'2'0'002,   // up to Mario's rank + 1
    OPTVAL_BADGE_MOVE_INFINITE  = 0x2'0c'2'0'003,   // up to lv. 99 always
    // Which items the player starts the pit with.
    OPT_STARTER_ITEMS           = 0x1'0e'2'0'002,
    OPTVAL_STARTER_ITEMS_OFF    = 0x2'0e'2'0'000,
    OPTVAL_STARTER_ITEMS_NORMAL = 0x2'0e'2'0'001,
    OPTVAL_STARTER_ITEMS_SET_2  = 0x2'0e'2'0'002,   // Reserved for future use?
    OPTVAL_STARTER_ITEMS_SET_3  = 0x2'0e'2'0'003,   // Reserved for future use?
    // Whether to use faster enemy stat scaling after floor 100.
    OPT_FLOOR_100_HP_SCALE      = 0x1'10'1'0'002,
    OPT_FLOOR_100_ATK_SCALE     = 0x1'11'1'0'002,
    // Merlee's curse enabled.
    OPT_MERLEE_CURSE            = 0x1'12'1'0'002,
    // How the stage's rank increases.
    OPT_STAGE_RANK              = 0x1'13'1'0'002,
    OPTVAL_STAGE_RANK_30_FLOORS = 0x2'13'1'0'000,
    OPTVAL_STAGE_RANK_ALWAYSMAX = 0x2'13'1'0'001,
    // Alternate balance options.
    OPT_PERCENT_BASED_DANGER    = 0x1'14'1'0'002,
    OPT_WEAKER_RUSH_BADGES      = 0x1'15'1'0'002,
    OPT_EVASION_BADGES_CAP      = 0x1'16'1'0'002,
    OPT_64_STYLE_HP_FP_DRAIN    = 0x1'17'1'0'002,
    // Changes to stage hazard rates.
    OPT_STAGE_HAZARDS           = 0x1'18'3'0'005,
    OPTVAL_STAGE_HAZARDS_NORMAL = 0x2'18'3'0'000,
    OPTVAL_STAGE_HAZARDS_HIGH   = 0x2'18'3'0'001,
    OPTVAL_STAGE_HAZARDS_LOW    = 0x2'18'3'0'002,
    OPTVAL_STAGE_HAZARDS_NO_FOG = 0x2'18'3'0'003,
    OPTVAL_STAGE_HAZARDS_OFF    = 0x2'18'3'0'004,
    // Whether to enable variance on all sources of variable damage.
    OPT_RANDOM_DAMAGE           = 0x1'1b'2'0'003,
    OPTVAL_RANDOM_DAMAGE_NONE   = 0x2'1b'2'0'000,
    OPTVAL_RANDOM_DAMAGE_25     = 0x2'1b'2'0'001,
    OPTVAL_RANDOM_DAMAGE_50     = 0x2'1b'2'0'002,
    // Random items from thrown audience items.
    OPT_AUDIENCE_RANDOM_THROWS  = 0x1'1d'1'0'002,
    // Whether Chet Rippo should appear early.
    OPT_CHET_RIPPO_APPEARANCE   = 0x1'1e'1'0'002,
    OPTVAL_CHET_RIPPO_50_ONWARD = 0x2'1e'1'0'000,
    OPTVAL_CHET_RIPPO_10_ONWARD = 0x2'1e'1'0'001,
    
    // Cosmetic / internal-only flag-based options.
    OPT_RTA_TIMER               = 0x1'60'1'0'002,
    OPT_YOSHI_COLOR_SELECT      = 0x1'61'1'0'002,
    OPT_START_WITH_FX           = 0x1'62'1'0'002,
    OPT_BGM_ENABLED             = 0x1'63'1'0'002,
    OPT_ENABLE_P_BADGES         = 0x1'64'1'0'002,
    OPT_ENABLE_PARTNER_REWARD   = 0x1'65'1'0'002,
    
    // Numeric options.
    // Global HP and ATK scaling (in percentage).
    OPTNUM_ENEMY_HP             = 0x3'00'2'1'3e8,
    OPTNUM_ENEMY_ATK            = 0x3'02'2'1'3e8,
    // SP cost for Superguarding (in increments of 0.01 SP).
    OPTNUM_SUPERGUARD_SP_COST   = 0x3'04'1'0'096,
    // FP cost for switching partners.
    OPTNUM_SWITCH_PARTY_FP_COST = 0x3'05'1'0'00a,
    
    // Gameplay stats.
    STAT_TURNS_SPENT            = 0x4'00'3'2'007,
    STAT_MOST_TURNS_RECORD      = 0x4'03'2'2'004,
    STAT_MOST_TURNS_CURRENT     = 0x4'05'2'2'004,
    STAT_MOST_TURNS_FLOOR       = 0x4'07'4'7'fff,
    STAT_TIMES_RAN_AWAY         = 0x4'0b'2'2'004,
    STAT_ENEMY_DAMAGE           = 0x4'0d'3'2'007,
    STAT_PLAYER_DAMAGE          = 0x4'10'3'2'007,
    STAT_ITEMS_USED             = 0x4'13'3'2'007,
    STAT_COINS_EARNED           = 0x4'16'3'2'007,
    STAT_COINS_SPENT            = 0x4'19'3'2'007,
    STAT_FP_SPENT               = 0x4'1c'3'2'007,
    STAT_SP_SPENT               = 0x4'1f'3'2'007,
    STAT_SUPERGUARDS            = 0x4'22'3'2'007,
    STAT_ITEMS_SOLD             = 0x4'25'3'2'007,
    STAT_BADGES_SOLD            = 0x4'28'3'2'007,
    STAT_LEVELS_SOLD            = 0x4'2b'2'2'004,
    STAT_SHINE_SPRITES          = 0x4'2d'2'2'003,
};

// An enumeration of all of the RNG sequences used by StateManager_v2.
// All RNG calls (except RNG_VANILLA) mangle the results with the filename seed,
// so results should differ between seeds, but be consistent on the same seed.
enum RngSequence {
    // "Totally random"; not reproducible run-to-run.
    RNG_VANILLA             = -1,    // Calls ttyd::system::irand().
    
    // Mangled w/floor number and OPT_CHEST_REWARDS; will change dramatically
    // if using options that change seeding, but will be consistent otherwise.
    RNG_CHEST               = 0,    // Chest top-level weighting.
    
    // Mangled w/floor number; may stay reasonably consistent even if using
    // options that change seeding for unrelated features.
    RNG_ENEMY               = 1,    // Enemy loadout generation.
    RNG_ITEM                = 2,    // Items (for enemy loadouts or shops).
    RNG_CONDITION           = 3,    // Bonus challenge condition.
    RNG_CONDITION_ITEM      = 4,    // Bonus challenge reward.
    
    // Not mangled w/ floor number; order should stay completely consistent,
    // independent of all seeding options (unless partners are disabled).
    RNG_CHEST_BADGE_FIXED   = 5,    // Chest one-time badges' order.
    RNG_PARTNER             = 6,    // Partners' obtained order.
    RNG_STAR_POWER          = 7,    // Star powers' obtained order.
    RNG_KISS_THIEF          = 8,    // Kiss Thief reward order.
    
    // Not mangled w/ floor number; will stay more or less consistent depending
    // on when partners are first unlocked, but independent of other options.
    RNG_CHEST_BADGE_RANDOM  = 9,    // Chest random badge order.
    RNG_AUDIENCE_ITEM       = 10,   // Random audience items (if using option).
    
    // Only used for picking random filenames.
    RNG_FILENAME            = 11,
    
    RNG_SEQUENCE_MAX,
};

}