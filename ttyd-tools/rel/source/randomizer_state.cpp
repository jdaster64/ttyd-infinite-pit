#include "randomizer_state.h"

#include "common_functions.h"
#include "common_types.h"
#include "patch.h"
#include "randomizer.h"

#include <gc/OSTime.h>
#include <ttyd/item_data.h>
#include <ttyd/mario_pouch.h>
#include <ttyd/mariost.h>

#include <cinttypes>
#include <cstdio>
#include <cstring>

namespace mod::pit_randomizer {
    
// Bitfield for options introduced in v1.40 (implementation details).
namespace Options_v1_40 {
    enum e {
        PARTNER_STARTING_RANK   = 0x3,
        DANGER_PERIL_BY_PERCENT = 0x4,
        MAX_BADGE_MOVE_LEVEL    = 0x18,
        RANK_UP_REQUIREMENT     = 0x60,
        // Values for composite options.
        MAX_MOVE_LEVEL_1X       = 0x0,
        MAX_MOVE_LEVEL_2X       = 0x8,
        MAX_MOVE_LEVEL_RANK     = 0x10,
        MAX_MOVE_LEVEL_INFINITE = 0x18,
        RANK_UP_NORMAL          = 0x0,
        RANK_UP_EARLIER         = 0x20,
        RANK_UP_BY_FLOOR        = 0x40,
        RANK_UP_ALWAYS_MAX      = 0x60,
    };
}
    
namespace {
    
using ::ttyd::mario_pouch::PouchData;
namespace ItemType = ::ttyd::item_data::ItemType;

const char* GetSavefileName() {
    return ttyd::mariost::g_MarioSt->saveFileName;
}

void* GetSavedStateLocation() {
    // Store randomizer state in stored items space, since this won't be used,
    // and the first byte of any possible stored item produces a "version" of 0.
    // Starts at index 1 to align to 4-byte boundary.
    return &ttyd::mario_pouch::pouchGetPtr()->stored_items[1];
}

// Updates partners' Ultra Rank max HP based how many times each partner
// has had a Shine Sprite used on them (each after the second adds +5 max HP).
void InitPartyMaxHpTable(uint8_t* partner_upgrades) {
    static constexpr const int16_t kDefaultUltraRankMaxHp[] = {
        30, 25, 40, 30, 35, 30, 25
    };
    int16_t* hp_table = ttyd::mario_pouch::_party_max_hp_table + 4;
    for (int32_t i = 0; i < 7; ++i) {
        int32_t hp = kDefaultUltraRankMaxHp[i] +
            (partner_upgrades[i] > 2 ? (partner_upgrades[i] - 2) * 5 : 0);
        if (hp > 200) hp = 200;
        hp_table[i * 4 + 2] = hp;
    }
}

bool LoadFromPreviousVersion(RandomizerState* state) {
    void* saved_state = GetSavedStateLocation();
    uint8_t version = *reinterpret_cast<uint8_t*>(saved_state);
    if (version < 1) {
        // Version is 0 or incompatible with the current version; fail to load.
        return false;
    }
    
    // Version is compatible; load, making any adjustments necessary.
    if (version == 2) {
        patch::writePatch(state, saved_state, sizeof(RandomizerState));
    } else if (version == 1) {
        patch::writePatch(state, saved_state, sizeof(RandomizerState));
        state->hp_multiplier_ = 100;
        state->atk_multiplier_ = 100;
        state->options_ = 2;
    }
    
    state->version_ = 2;
    InitPartyMaxHpTable(state->partner_upgrades_);
    return true;
}

}

bool RandomizerState::Load(bool new_save) {
    if (!new_save) return LoadFromPreviousVersion(this);
    
    version_ = 2;
    floor_ = 0;
    reward_flags_ = 0x00000000;
    load_from_save_ = false;
    disable_partner_badges_in_shop_ = true;
    for (int32_t i = 0; i < 7; ++i) partner_upgrades_[i] = 0;
    InitPartyMaxHpTable(partner_upgrades_);
    
    // Default options: All optional flags off, 2 chests / floor.
    hp_multiplier_ = 100;
    atk_multiplier_ = 100;
    options_ = 2;
    
    // Seed the rng based on the filename.
    // If the filename is a few variants of "random" or a single 'star',
    // pick a random replacement filename.
    const char* filename = GetSavefileName();
    for (const char* ch = filename; *ch; ++ch) {
        // If the filename contains a 'heart' character, start w/FX badges.
        if (*ch == '\xd0') options_ |= RandomizerState::START_WITH_FX;
    }
    if (!strcmp(filename, "random") || !strcmp(filename, "Random") ||
        !strcmp(filename, "RANDOM") || !strcmp(filename, "\xde") ||
        !strcmp(filename, "random\xd0") || !strcmp(filename, "Random\xd0") ||
        !strcmp(filename, "RANDOM\xd0") || !strcmp(filename, "\xde\xd0")) { 
        char filenameChars[9];
        rng_state_ = static_cast<uint32_t>(gc::OSTime::OSGetTime());
        for (int32_t i = 0; i < 8; ++i) {
            // Pick uppercase / lowercase characters randomly (excluding I / l).
            int32_t ch = Rand(50);
            if (ch < 25) {
                filenameChars[i] = ch + 'a';
                if (filenameChars[i] == 'l') filenameChars[i] = 'z';
            } else {
                filenameChars[i] = (ch - 25) + 'A';
                if (filenameChars[i] == 'I') filenameChars[i] = 'Z';
            }
        }
        // If a heart was in the initial filename, put one at the end of the
        // random one as well.
        if (options_ & START_WITH_FX) filenameChars[7] = '\xd0';
        filenameChars[8] = 0;
        // Copy generated filename to MarioSt.
        strcpy(const_cast<char*>(filename), filenameChars);
    }
    SeedRng(filename);    
    return true;
}

void RandomizerState::Save() {
    void* saved_state = GetSavedStateLocation();
    patch::writePatch(saved_state, this, sizeof(RandomizerState));
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

void RandomizerState::ChangeOption(int32_t option, int32_t change) {
    PouchData& pouch = *ttyd::mario_pouch::pouchGetPtr();
    
    switch (option) {
        case NUM_CHEST_REWARDS: {
            int32_t value = GetOptionValue(option) + change;
            if (value < 0) value = 5;
            if (value > 5) value = 0;
            options_ = (options_ & ~NUM_CHEST_REWARDS) | value;
            break;
        }
        case NO_EXP_MODE: {
            if (change == 0) change = 1;
            int32_t value = GetOptionValue(option) + change * (option / 3);
            if (value < 0) value = NO_EXP_INFINITE_BP;
            if (value > NO_EXP_INFINITE_BP) value = 0;
            options_ = (options_ & ~option) | value;
            break;
        }
        case START_WITH_NO_ITEMS: {
            options_ ^= START_WITH_NO_ITEMS;
            if (options_ & START_WITH_NO_ITEMS) {
                // Delete starter items.
                for (int32_t i = 0; i < 20; ++i) {
                    pouch.items[i] = 0;
                }
            } else {
                // Re-add starter items.
                ttyd::mario_pouch::pouchGetItem(ItemType::THUNDER_BOLT);
                ttyd::mario_pouch::pouchGetItem(ItemType::FIRE_FLOWER);
                ttyd::mario_pouch::pouchGetItem(ItemType::HONEY_SYRUP);
                ttyd::mario_pouch::pouchGetItem(ItemType::MUSHROOM);
            }
            break;
        }
        case MERLEE: {
            options_ ^= MERLEE;
            if (options_ & MERLEE) {
                pouch.merlee_curse_uses_remaining = 99;
                pouch.turns_until_merlee_activation = -1;
            } else {
                pouch.merlee_curse_uses_remaining = 0;
                pouch.turns_until_merlee_activation = 0;
            }
            break;
        }
        case SWITCH_PARTY_COST_FP: {
            int32_t value = GetOptionValue(option) + change;
            if (value < 0) value = 3;
            if (value > 3) value = 0;
            options_ = (options_ & ~SWITCH_PARTY_COST_FP) | 
                       (SWITCH_PARTY_COST_FP / 3 * value);
            break;
        }
        case BATTLE_REWARD_MODE:
        case POST_100_SCALING: {
            if (change == 0) change = 1;
            int32_t value = GetOptionValue(option) + change * (option / 3);
            if (value < 0) value = option;
            if (value > option) value = 0;
            options_ = (options_ & ~option) | value;
            break;
        }
        case STAGE_HAZARD_OPTIONS: {
            if (change == 0) change = 1;
            int32_t value = GetOptionValue(option) + change * (option / 7);
            if (value < 0) value = HAZARD_RATE_OFF;
            if (value > HAZARD_RATE_OFF) value = 0;
            options_ = (options_ & ~option) | value;
            break;
        }
        case DAMAGE_RANGE: {
            if (change == 0) change = 1;
            int32_t value = GetOptionValue(option) + change * (option / 3);
            if (value < 0) value = DAMAGE_RANGE_50;
            if (value > DAMAGE_RANGE_50) value = 0;
            options_ = (options_ & ~option) | value;
            break;
        }
        case HP_MODIFIER: {
            hp_multiplier_ += change;
            if (hp_multiplier_ < 1) hp_multiplier_ = 1;
            if (hp_multiplier_ > 1000) hp_multiplier_ = 1000;
            break;
        }
        case ATK_MODIFIER: {
            atk_multiplier_ += change;
            if (atk_multiplier_ < 1) atk_multiplier_ = 1;
            if (atk_multiplier_ > 1000) atk_multiplier_ = 1000;
            break;
        }
        case PARTNER_STARTING_RANK: {
            if (change == 0) change = 1;
            option = Options_v1_40::PARTNER_STARTING_RANK;
            int32_t max_option = option * 2 / 3;
            int32_t value = options_v1_40_ & option;
            value += change * (option / 3);
            if (value < 0) value = max_option;
            if (value > max_option) value = 0;
            options_v1_40_ = (options_v1_40_ & ~option) | value;
            break;
        }        
        case DANGER_PERIL_BY_PERCENT: {
            options_v1_40_ ^= Options_v1_40::DANGER_PERIL_BY_PERCENT;
            break;
        }        
        case MAX_BADGE_MOVE_LEVEL: {
            if (change == 0) change = 1;
            option = Options_v1_40::MAX_BADGE_MOVE_LEVEL;
            int32_t value = options_v1_40_ & option;
            value += change * (option / 3);
            if (value < 0) value = Options_v1_40::MAX_MOVE_LEVEL_INFINITE;
            if (value > Options_v1_40::MAX_MOVE_LEVEL_INFINITE) value = 0;
            options_v1_40_ = (options_v1_40_ & ~option) | value;
            break;
        }        
        case RANK_UP_REQUIREMENT: {
            if (change == 0) change = 1;
            option = Options_v1_40::RANK_UP_REQUIREMENT;
            int32_t value = options_v1_40_ & option;
            value += change * (option / 3);
            if (value < 0) value = Options_v1_40::RANK_UP_ALWAYS_MAX;
            if (value > Options_v1_40::RANK_UP_ALWAYS_MAX) value = 0;
            options_v1_40_ = (options_v1_40_ & ~option) | value;
            break;
        }        
        default: {
            options_ ^= option;
            break;
        }
    }
}

int32_t RandomizerState::GetOptionValue(int32_t option) const {
    switch (option) {
        case HP_MODIFIER:
            return hp_multiplier_;
        case ATK_MODIFIER:
            return atk_multiplier_;
        case SWITCH_PARTY_COST_FP:
            return (options_ & SWITCH_PARTY_COST_FP) / (SWITCH_PARTY_COST_FP/3);
        case NUM_CHEST_REWARDS:
        case BATTLE_REWARD_MODE:
        case NO_EXP_MODE:
        case POST_100_SCALING:
        case STAGE_HAZARD_OPTIONS:
        case DAMAGE_RANGE:
            return (options_ & option);
        case PARTNER_STARTING_RANK:
            return options_v1_40_ & Options_v1_40::PARTNER_STARTING_RANK;
        case DANGER_PERIL_BY_PERCENT:
            return (options_v1_40_ & Options_v1_40::DANGER_PERIL_BY_PERCENT) != 0;
        case MAX_BADGE_MOVE_LEVEL:
            switch (options_v1_40_ & Options_v1_40::MAX_BADGE_MOVE_LEVEL) {
                case Options_v1_40::MAX_MOVE_LEVEL_1X:
                    return MAX_MOVE_LEVEL_1X;
                case Options_v1_40::MAX_MOVE_LEVEL_2X:
                    return MAX_MOVE_LEVEL_2X;
                case Options_v1_40::MAX_MOVE_LEVEL_RANK:
                    return MAX_MOVE_LEVEL_RANK;
                case Options_v1_40::MAX_MOVE_LEVEL_INFINITE:
                    return MAX_MOVE_LEVEL_INFINITE;
            }
        case RANK_UP_REQUIREMENT:
            switch (options_v1_40_ & Options_v1_40::RANK_UP_REQUIREMENT) {
                case Options_v1_40::RANK_UP_NORMAL:
                    return RANK_UP_NORMAL;
                case Options_v1_40::RANK_UP_EARLIER:
                    return RANK_UP_EARLIER;
                case Options_v1_40::RANK_UP_BY_FLOOR:
                    return RANK_UP_BY_FLOOR;
                case Options_v1_40::RANK_UP_ALWAYS_MAX:
                    return RANK_UP_ALWAYS_MAX;
            }
        default:
            return (options_ & option) != 0;
    }
}

void RandomizerState::GetOptionStrings(
    int32_t option, char* name, char* value, uint32_t* color) const {
    // Set name.
    switch (option) {
        case NUM_CHEST_REWARDS: {
            sprintf(name, "Rewards per chest:");
            break;
        }
        case BATTLE_REWARD_MODE: {
            sprintf(name, "Battle drops:");
            break;
        }
        case START_WITH_PARTNERS: {
            sprintf(name, "Start with all partners:");
            break;
        }
        case START_WITH_SWEET_TREAT: {
            sprintf(name, "Start with Sweet Treat:");
            break;
        }
        case NO_EXP_MODE: {
            sprintf(name, "No-EXP mode:");
            break;
        }
        case START_WITH_NO_ITEMS: {
            sprintf(name, "Starter set of items:");
            break;
        }
        case SHINE_SPRITES_MARIO: {
            sprintf(name, "Use Shines on Mario for +SP:");
            break;
        }
        case ALWAYS_ENABLE_AUDIENCE: {
            sprintf(name, "Audience always present:");
            break;
        }
        case MERLEE: {
            sprintf(name, "Infinite Merlee curses:");
            break;
        }
        case SUPERGUARDS_COST_FP: {
            sprintf(name, "Superguards cost 1 FP:");
            break;
        }
        case SWITCH_PARTY_COST_FP: {
            sprintf(name, "Partner switch cost:");
            break;
        }
        case WEAKER_RUSH_BADGES: {
            sprintf(name, "Power/Mega Rush power:");
            break;
        }
        case HP_MODIFIER: {
            sprintf(name, "Enemy HP multiplier:");
            break;
        }
        case ATK_MODIFIER: {
            sprintf(name, "Enemy ATK multiplier:");
            break;
        }
        case POST_100_HP_SCALING: {
            sprintf(name, "Late-Pit HP scaling:");
            break;
        }
        case POST_100_ATK_SCALING: {
            sprintf(name, "Late-Pit ATK scaling:");
            break;
        }
        case POST_100_SCALING: {
            sprintf(name, "Late-Pit stat scaling:");
            break;
        }
        case CAP_BADGE_EVASION: {
            sprintf(name, "Cap badge evasion at 80%%:");
            break;
        }
        case HP_FP_DRAIN_PER_HIT: {
            sprintf(name, "PM64-style Drain badges:");
            break;
        }
        case SWAP_CO_PL_SP_COST: {
            sprintf(name, "Swap Clock Out & Power Lift:");
            break;
        }
        case STAGE_HAZARD_OPTIONS: {
            sprintf(name, "Stage hazard frequency:");
            break;
        }
        case DAMAGE_RANGE: {
            sprintf(name, "Damage randomization:");
            break;
        }
        case AUDIENCE_ITEMS_RANDOM: {
            sprintf(name, "Random audience items:");
            break;
        }
        case PARTNER_STARTING_RANK: {
            sprintf(name, "Partner starting rank:");
            break;
        }
        case DANGER_PERIL_BY_PERCENT: {
            sprintf(name, "Danger/Peril thresholds:");
            break;
        }
        case MAX_BADGE_MOVE_LEVEL: {
            sprintf(name, "Max badge move level:");
            break;
        }
        case RANK_UP_REQUIREMENT: {
            sprintf(name, "Stage rank-up after:");
            break;
        }
        default: {
            name[0] = '\0';
            break;
        }
    }
    
    // Set value.
    int32_t option_value = GetOptionValue(option);
    *color = 0;
    switch (option) {
        case NO_EXP_MODE: {
            switch (option_value) {
                case 0: {
                    sprintf(value, "Off");
                    *color = 0xe50000ffU;
                    break;
                }
                case NO_EXP_99_BP: {
                    sprintf(value, "On (99 BP)");
                    *color = 0x00c100ffU;
                    break;
                }
                case NO_EXP_INFINITE_BP: {
                    sprintf(value, "On (Infinite BP)");
                    *color = 0x00c100ffU;
                    break;
                }
            }
            break;
        }
        case BATTLE_REWARD_MODE: {
            switch (option_value) {
                case 0: {
                    sprintf(value, "One held + bonus");
                    break;
                }
                case CONDITION_DROPS_HELD: {
                    sprintf(value, "Condition-gated");
                    break;
                }
                case NO_HELD_ITEMS: {
                    sprintf(value, "Conditions only");
                    break;
                }
                case ALL_HELD_ITEMS: {
                    sprintf(value, "All held + bonus");
                    break;
                }
            }
            break;
        }
        case NUM_CHEST_REWARDS: {
            if (option_value == 0) {
                sprintf(value, "Varies");
            } else {
                sprintf(value, "%" PRId32, option_value);
            }
            break;
        }
        case START_WITH_NO_ITEMS: {
            // Reversed options (defaults to "On").
            if (!option_value) {
                sprintf(value, "On");
                *color = 0x00c100ffU;
            } else {
                sprintf(value, "Off");
                *color = 0xe50000ffU;
            }
            break;
        }
        case SWITCH_PARTY_COST_FP: {
            if (option_value) {
                sprintf(value, "%" PRId32 " FP", option_value);
            } else {
                sprintf(value, "Off");
                *color = 0xe50000ffU;
            }
            break;
        }
        case POST_100_SCALING: {
            switch (option_value) {
                case 0: {
                    sprintf(value, "+5%% HP + ATK");
                    break;
                }
                case POST_100_HP_SCALING: {
                    sprintf(value, "+10%% HP, +5%% ATK");
                    break;
                }
                case POST_100_ATK_SCALING: {
                    sprintf(value, "+5%% HP, +10%% ATK");
                    break;
                }
                case POST_100_SCALING: {
                    sprintf(value, "+10%% HP + ATK");
                    break;
                }
            }
            break;
        }
        case HP_MODIFIER: 
        case ATK_MODIFIER: {
            sprintf(value, "%" PRId32 "%s", option_value, "%");
            break;
        }
        case POST_100_HP_SCALING:
        case POST_100_ATK_SCALING: {
            sprintf(value, option_value ? "+10%% / set" : "+5%% / set");
            break;
        }
        case WEAKER_RUSH_BADGES: {
            sprintf(value, option_value ? "+1/+2" : "+2/+5");
            break;
        }
        case STAGE_HAZARD_OPTIONS: {
            switch (option_value) {
                case 0: {
                    sprintf(value, "Default");
                    break;
                }
                case HAZARD_RATE_HIGH: {
                    sprintf(value, "High");
                    break;
                }
                case HAZARD_RATE_LOW: {
                    sprintf(value, "Low");
                    break;
                }
                case HAZARD_RATE_NO_FOG: {
                    sprintf(value, "No Fog");
                    break;
                }
                case HAZARD_RATE_OFF: {
                    sprintf(value, "Off");
                    break;
                }
            }
            break;
        }
        case DAMAGE_RANGE: {
            switch (option_value) {
                case 0: {
                    sprintf(value, "Off");
                    *color = 0xe50000ffU;
                    break;
                }
                case DAMAGE_RANGE_25: {
                    sprintf(value, "+/-25%%");
                    break;
                }
                case DAMAGE_RANGE_50: {
                    sprintf(value, "+/-50%%");
                    break;
                }
            }
            break;
        }
        case PARTNER_STARTING_RANK: {
            switch (option_value) {
                case 0: {
                    sprintf(value, "Normal");
                    break;
                }
                case 1: {
                    sprintf(value, "Super");
                    break;
                }
                case 2: {
                    sprintf(value, "Ultra");
                    break;
                }
            }
            break;
        }
        case DANGER_PERIL_BY_PERCENT: {
            sprintf(value, option_value ? "%%-based" : "Fixed");
            break;
        }
        case MAX_BADGE_MOVE_LEVEL: {
            switch (option_value) {
                case MAX_MOVE_LEVEL_1X: {
                    sprintf(value, "1 per copy");
                    break;
                }
                case MAX_MOVE_LEVEL_2X: {
                    sprintf(value, "2 per copy");
                    break;
                }
                case MAX_MOVE_LEVEL_RANK: {
                    sprintf(value, "Rank-based");
                    break;
                }
                case MAX_MOVE_LEVEL_INFINITE: {
                    sprintf(value, "Always 99");
                    break;
                }
            }
            break;
        }
        case RANK_UP_REQUIREMENT: {
            switch (option_value) {
                case RANK_UP_NORMAL: {
                    sprintf(value, "Lvl. 10/20/30");
                    break;
                }
                case RANK_UP_EARLIER: {
                    sprintf(value, "Lvl. 8/15/25");
                    break;
                }
                case RANK_UP_BY_FLOOR: {
                    sprintf(value, "Floor 30/60/90");
                    break;
                }
                case RANK_UP_ALWAYS_MAX: {
                    sprintf(value, "Always max");
                    break;
                }
            }
            break;
        }
        case INVALID_OPTION: {
            value[0] = '\0';
            break;
        }
        default: {
            if (option_value) {
                sprintf(value, "On");
                *color = 0x00c100ffU;
            } else {
                sprintf(value, "Off");
                *color = 0xe50000ffU;
            }
            break;
        }
    }
}

int32_t RandomizerState::StarPowersObtained() const {
    return CountSetBits(GetBitMask(5, 12) & reward_flags_);
}

bool RandomizerState::StarPowerEnabled() const {
    return GetOptionValue(ALWAYS_ENABLE_AUDIENCE) || 
           (CountSetBits(GetBitMask(5, 12) & reward_flags_) > 0);
}

void RandomizerState::IncrementPlayStat(PlayStats stat, int32_t amount) {
    int32_t offset, length, max;
    switch (stat) {
        case TURNS_SPENT: {
            offset = 0;
            length = 3;
            max = 9'999'999;
            break;
        }
        case TIMES_RAN_AWAY: {
            offset = 3;
            length = 2;
            max = 9999;
            break;
        }
        case ENEMY_DAMAGE: {
            offset = 5;
            length = 3;
            max = 9'999'999;
            break;
        }
        case PLAYER_DAMAGE: {
            offset = 8;
            length = 3;
            max = 9'999'999;
            break;
        }
        case ITEMS_USED: {
            offset = 11;
            length = 2;
            max = 9999;
            break;
        }
        case COINS_EARNED: {
            offset = 13;
            length = 3;
            max = 9'999'999;
            break;
        }
        case COINS_SPENT: {
            offset = 16;
            length = 3;
            max = 9'999'999;
            break;
        }
        default: return;
    }
    int32_t current = 0;
    for (int32_t i = offset; i < offset + length; ++i) {
        current = (current << 8) + play_stats_[i];
    }
    current += amount;
    if (current > max) current = max;
    for (int32_t i = offset + length - 1; i >= offset; --i) {
        play_stats_[i] = current & 0xff;
        current >>= 8;
    }
}

int32_t RandomizerState::GetPlayStat(PlayStats stat) const {
    int32_t offset, length;
    switch (stat) {
        case TURNS_SPENT: {
            offset = 0;
            length = 3;
            break;
        }
        case TIMES_RAN_AWAY: {
            offset = 3;
            length = 2;
            break;
        }
        case ENEMY_DAMAGE: {
            offset = 5;
            length = 3;
            break;
        }
        case PLAYER_DAMAGE: {
            offset = 8;
            length = 3;
            break;
        }
        case ITEMS_USED: {
            offset = 11;
            length = 2;
            break;
        }
        case COINS_EARNED: {
            offset = 13;
            length = 3;
            break;
        }
        case COINS_SPENT: {
            offset = 16;
            length = 3;
            break;
        }
        case SHINE_SPRITES_USED: {
            return ttyd::mario_pouch::pouchGetPtr()->shine_sprites;
        }
        // Cannot be reached with a valid PlayStats type.
        default: return 0;
    }
    int32_t result = 0;
    for (int32_t i = offset; i < offset + length; ++i) {
        result = (result << 8) + play_stats_[i];
    }
    return result;
}

const char* RandomizerState::GetEncodedOptions() const {
    static char enc_options[16];
    
    uint64_t options = options_v1_40_;
    options = (options << 32) + static_cast<uint32_t>(options_);
    // Turn off purely-cosmetic / non-user-selectable options.
    options &= ~START_WITH_FX;
    options &= ~YOSHI_COLOR_SELECT;
    // Shift hp and atk multipliers onto the end.
    options <<= 24;
    options += hp_multiplier_ << 12;
    options += atk_multiplier_;
    
    // Convert to a base-64 scheme using A-Z, a-z, 0-9, !, ?.
    // Add eleventh character only if any bits 60+ are set, for encoding
    // backward compatibility.
    int32_t optional_digits = 0;
    if (options >> 60) optional_digits = 1;
    for (int32_t i = 9 + optional_digits; i >= 0; --i) {
        const int32_t sextet = options & 63;
        char next;
        if (sextet < 26) {
            next = 'A' + sextet;
        } else if (sextet < 52) {
            next = 'a' + sextet - 26;
        } else if (sextet < 62) {
            next = '0' + sextet - 52;
        } else if (sextet < 63) {
            next = '!';
        } else {
            next = '?';
        }
        enc_options[i] = next;
        options >>= 6;
    }
    enc_options[10 + optional_digits] = '\0';
    
    return enc_options;
}

bool RandomizerState::GetPlayStatsString(char* out_buf) const {
    const auto* mariost = ttyd::mariost::g_MarioSt;
    const uint64_t current_time = gc::OSTime::OSGetTime();
    const int64_t last_save_diff = 
        current_time - mariost->hllPickLastReceivedTime;
    // If the time was never initialized, or the current time is before
    // the last save's time (indicating clock tampering), don't show any stats.
    if (last_save_diff < 0 || !mariost->hllSignLastReadTime) {
        return false;
    }
    
    // Page 1: Seed, floor, options & total play time.
    out_buf += sprintf(
        out_buf,
        "<kanban>\n"
        "Seed: <col 0000ffff>%s</col>, Floor: <col ff0000ff>%" PRId32 "\n</col>"
        "Options: <col 0000ffff>%s\n</col>"
        "RTA Time: <col ff0000ff>",
        GetSavefileName(), floor_ + 1, GetEncodedOptions());
    const int64_t start_diff = current_time - mariost->hllSignLastReadTime;
    out_buf += DurationTicksToFmtString(start_diff, out_buf);
    
    // Page 2: Battle time, turn count, run away count.
    out_buf += sprintf(out_buf, "\n</col><k><p>In-battle time: ");
    const int64_t battle_time = 
        mariost->animationTimeIncludingBattle - mariost->animationTimeNoBattle;
    out_buf += DurationTicksToFmtString(battle_time, out_buf);
    out_buf += sprintf(out_buf, "\nTotal turns: ");
    out_buf += IntegerToFmtString(GetPlayStat(TURNS_SPENT), out_buf, 9'999'999);
    out_buf += sprintf(out_buf, "\nTimes ran away: ");
    out_buf += IntegerToFmtString(GetPlayStat(TIMES_RAN_AWAY), out_buf, 9'999);
    
    // Page 3: Damage dealt / taken, items used.
    out_buf += sprintf(out_buf, "\n<k><p>Enemy damage taken: ");
    out_buf += IntegerToFmtString(GetPlayStat(ENEMY_DAMAGE), out_buf, 9'999'999);
    out_buf += sprintf(out_buf, "\nPlayer damage taken: ");
    out_buf += IntegerToFmtString(GetPlayStat(PLAYER_DAMAGE), out_buf, 9'999'999);
    out_buf += sprintf(out_buf, "\nItems used: ");
    out_buf += IntegerToFmtString(GetPlayStat(ITEMS_USED), out_buf, 9'999);
    
    // Page 4: Coins earned / spent, Shine Sprites used.
    out_buf += sprintf(out_buf, "\n<k><p>Coins earned: ");
    out_buf += IntegerToFmtString(GetPlayStat(COINS_EARNED), out_buf, 9'999'999);
    out_buf += sprintf(out_buf, "\nCoins spent: ");
    out_buf += IntegerToFmtString(GetPlayStat(COINS_SPENT), out_buf, 9'999'999);
    out_buf += sprintf(out_buf, "\nShine Sprites used: ");
    out_buf += IntegerToFmtString(GetPlayStat(SHINE_SPRITES_USED), out_buf, 999);
    out_buf += sprintf(out_buf, "\n<k>");
    
    return true;
}

void RandomizerState::SaveCurrentTime(bool pit_start) {
    uint64_t current_time = gc::OSTime::OSGetTime();
    // Use the otherwise unused Happy Lucky Lottery timestamps for saving
    // the last time you entered the Pit, and the last time you saved the game.
    if (pit_start) {
        ttyd::mariost::g_MarioSt->hllSignLastReadTime = current_time;
    }
    ttyd::mariost::g_MarioSt->hllPickLastReceivedTime = current_time;
}

const char* RandomizerState::GetCurrentTimeString() {
    static char buf[16];
    const uint64_t current_time = gc::OSTime::OSGetTime();
    const int64_t start_diff = 
        current_time - ttyd::mariost::g_MarioSt->hllSignLastReadTime;
    const int64_t last_save_diff = 
        current_time - ttyd::mariost::g_MarioSt->hllPickLastReceivedTime;
    // If the time since the last save is negative or the time was never set,
    // return the empty string and clear the timebases previously set.
    if (last_save_diff < 0 || !ttyd::mariost::g_MarioSt->hllSignLastReadTime) {
        ttyd::mariost::g_MarioSt->hllSignLastReadTime = 0;
        ttyd::mariost::g_MarioSt->hllPickLastReceivedTime = 0;
        return "";
    }
    // Otherwise, return the time since start as a string.
    DurationTicksToFmtString(start_diff, buf);
    return buf;
}

}