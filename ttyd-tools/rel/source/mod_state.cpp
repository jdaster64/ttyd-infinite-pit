#include "mod_state.h"

#include "common_functions.h"
#include "common_types.h"
#include "mod.h"
#include "patch.h"

#include <gc/OSTime.h>
#include <third_party/fasthash.h>
#include <ttyd/item_data.h>
#include <ttyd/mario_pouch.h>
#include <ttyd/mariost.h>
#include <ttyd/system.h>

#include <cinttypes>
#include <cstdio>
#include <cstring>

namespace mod::infinite_pit {
    
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

void* _GetSavedStateLocation() {
    // Store mod's state in stored items space, since this won't be used,
    // and the first byte of any possible stored item produces a "version" of 0.
    // Starts at index 1 to align to 4-byte boundary.
    return &ttyd::mario_pouch::pouchGetPtr()->stored_items[1];
}

bool _LoadFromPreviousVersion(StateManager* state) {
    void* saved_state = _GetSavedStateLocation();
    uint8_t version = *reinterpret_cast<uint8_t*>(saved_state);
    if (version < 1) {
        // Version is 0 or incompatible with the current version; fail to load.
        return false;
    }
    
    // Version is compatible; load, making any adjustments necessary.
    if (version == 3) {
        patch::writePatch(state, saved_state, sizeof(StateManager));
    } else if (version == 2) {
        patch::writePatch(state, saved_state, sizeof(StateManager));
        state->options_v1_40_ = 0;
        for (int32_t i = 0; i < 5; ++i) state->unused_[i] = 0;
    } else if (version == 1) {
        // Note: loading from version 1 will break seeding, but since it was
        // never publically distributed, it's useful for me to keep around.
        patch::writePatch(state, saved_state, sizeof(StateManager));
        state->hp_multiplier_ = 100;
        state->atk_multiplier_ = 100;
        state->options_ = 2;
        state->options_v1_40_ = 0;
        for (int32_t i = 0; i < 5; ++i) state->unused_[i] = 0;
        for (int32_t i = 0; i < 20; ++i) state->play_stats_[i] = 0;
    }
    
    state->version_ = 3;
    InitPartyMaxHpTable(state->partner_upgrades_);
    return true;
}

}

bool StateManager::Load(bool new_save) {
    if (!new_save) return _LoadFromPreviousVersion(this);
    
    version_ = 3;
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
    options_v1_40_ = 0;
    
    // Fill with 0's for future-proofing.
    for (int32_t i = 0; i < 5; ++i) unused_[i] = 0;
    
    // Seed the rng based on the filename.
    // If the filename is a few variants of "random" or a single 'star',
    // pick a random replacement filename.
    const char* filename = GetSavefileName();
    for (const char* ch = filename; *ch; ++ch) {
        // If the filename contains a 'heart' character, start w/FX badges.
        if (*ch == '\xd0') options_ |= StateManager::START_WITH_FX;
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

void StateManager::Save() {
    void* saved_state = _GetSavedStateLocation();
    patch::writePatch(saved_state, this, sizeof(StateManager));
}

void StateManager::SeedRng(const char* str) {
    uint32_t hash = 0;
    for (const char* c = str; *c != 0; ++c) {
        hash = 37 * hash + *c;
    }
    rng_state_ = hash;
}

uint32_t StateManager::Rand(uint32_t range) {
    rng_state_ = rng_state_ * 0x41c64e6d + 12345;
    return ((rng_state_ >> 16) & 0x7fff) % range;
}

void StateManager::ChangeOption(int32_t option, int32_t change) {
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

int32_t StateManager::GetOptionValue(int32_t option) const {
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

void StateManager::GetOptionStrings(
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

int32_t StateManager::StarPowersObtained() const {
    return CountSetBits(GetBitMask(5, 12) & reward_flags_);
}

bool StateManager::StarPowerEnabled() const {
    return GetOptionValue(ALWAYS_ENABLE_AUDIENCE) || 
           (CountSetBits(GetBitMask(5, 12) & reward_flags_) > 0);
}

void StateManager::IncrementPlayStat(PlayStats stat, int32_t amount) {
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

int32_t StateManager::GetPlayStat(PlayStats stat) const {
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

const char* StateManager::GetEncodedOptions() const {
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

bool StateManager::GetPlayStatsString(char* out_buf) const {
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

void StateManager::SaveCurrentTime(bool pit_start) {
    uint64_t current_time = gc::OSTime::OSGetTime();
    // Use the otherwise unused Happy Lucky Lottery timestamps for saving
    // the last time you entered the Pit, and the last time you saved the game.
    if (pit_start) {
        ttyd::mariost::g_MarioSt->hllSignLastReadTime = current_time;
    }
    ttyd::mariost::g_MarioSt->hllPickLastReceivedTime = current_time;
}

const char* StateManager::GetCurrentTimeString() {
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

// TODO: Delete all StateManager v1 code and merge the two anonymous namespaces
// once all old code is ported to use StateManager_v2.
namespace {
    
void* GetSavedStateLocation() {
    // Store state_v2 in otherwise unused space at the end of the GSWF array.
    // This affords 0x120 bytes of space, and should be aligned to 8 bytes.
    return &ttyd::mariost::g_MarioSt->gswf[0x2e0];
}

bool LoadFromPreviousVersion(StateManager_v2* state) {
    void* saved_state = GetSavedStateLocation();
    uint8_t version = *reinterpret_cast<uint8_t*>(saved_state);
    if (version != 4) {
        // Version is incompatible with the current version; fail to load.
        return false;
    }
    
    // Since the only compatible version is current, a direct copy will suffice.
    patch::writePatch(state, saved_state, sizeof(StateManager_v2));
    // Force version to current.
    state->version_ = 4;
    InitPartyMaxHpTable(state->partner_upgrades_);
    return true;
}

// Gets the individual parts from an Options_v2 enum value.
void GetOptionParts(uint32_t v, int32_t* x, int32_t* y, int32_t* w, int32_t* z) {
    *x = GetShiftedBitMask(v, 20, 27);
    *y = GetShiftedBitMask(v, 16, 19);
    *w = GetShiftedBitMask(v, 12, 15);
    *z = v & 0xfff;
}

// Gets the base64-encoded character for the encoded options string.
char GetBase64EncodingChar(int32_t sextet) {
    if (sextet < 26) {
        return 'A' + sextet;
    } else if (sextet < 52) {
        return 'a' + sextet - 26;
    } else if (sextet < 62) {
        return '0' + sextet - 52;
    } else if (sextet < 63) {
        return '!';
    } else {
        return '?';
    }
}

}

bool StateManager_v2::Load(bool new_save) {
    if (!new_save) return LoadFromPreviousVersion(this);
    
    // Carry cosmetic / cheats state over between files.
    uint32_t cosmetic_options = option_flags_[3];
    
    memset(this, 0, sizeof(StateManager_v2));
    version_ = 4;
    SetDefaultOptions();
    option_flags_[3] = cosmetic_options;
    InitPartyMaxHpTable(partner_upgrades_);
    
    // Start new files with Lv. 1 Sweet Treat and Earth Tremor.
    star_power_levels_ = 0b00'00'00'00'00'00'01'01;
    
    // Seed the RNG based on the filename.
    // If the filename is a few variants of "random" or a single 'star',
    // pick a random replacement filename.
    const char* filename = GetSavefileName();
    for (const char* ch = filename; *ch; ++ch) {
        // If the filename contains a 'heart' character, start w/FX badges.
        if (*ch == '\xd0') SetOption(OPT_START_WITH_FX);
    }
    if (!strcmp(filename, "random") || !strcmp(filename, "Random") ||
        !strcmp(filename, "RANDOM") || !strcmp(filename, "\xde") ||
        !strcmp(filename, "random\xd0") || !strcmp(filename, "Random\xd0") ||
        !strcmp(filename, "RANDOM\xd0") || !strcmp(filename, "\xde\xd0")) { 
        char filenameChars[9];
        // Temporarily set the filename seed based on the current time.
        filename_seed_ = static_cast<uint32_t>(gc::OSTime::OSGetTime());
        for (int32_t i = 0; i < 8; ++i) {
            // Pick uppercase / lowercase characters randomly (excluding I / l).
            int32_t ch = Rand(50, RNG_FILENAME);
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
        if (GetOptionNumericValue(OPT_START_WITH_FX)) {
            filenameChars[7] = '\xd0';
        }
        filenameChars[8] = 0;
        // Copy generated filename to MarioSt.
        strcpy(const_cast<char*>(filename), filenameChars);
    }
    
    // Save the filename's hash as a seed for part of the RNG function.
    uint32_t hash = 0;
    for (const char* c = filename; *c != 0; ++c) {
        hash = 37 * hash + *c;
    }
    filename_seed_ = hash;
    return true;
}

void StateManager_v2::Save() {
    void* saved_state = _GetSavedStateLocation();
    patch::writePatch(saved_state, this, sizeof(StateManager_v2));
}

int32_t StateManager_v2::GetStarPowerLevel(int32_t star_power_type) const {
    if (star_power_type < 0 || star_power_type > 7) return 0;
    return GetShiftedBitMask(
        star_power_levels_, star_power_type*2, star_power_type*2 + 1);
}

uint32_t StateManager_v2::Rand(uint32_t range, int32_t sequence) {
    if (sequence > RNG_VANILLA && sequence < RNG_SEQUENCE_MAX) {
        uint32_t data[2] = { 0, 0 };
        uint16_t* seq_val = rng_sequences_ + sequence;
        data[0] = (*seq_val)++;
        switch (sequence) {
            case RNG_CHEST: {
                data[0] |= GetOptionNumericValue(OPT_CHEST_REWARDS);
                data[1] = floor_;
                break;
            }
            case RNG_ENEMY:
            case RNG_ITEM:
            case RNG_CONDITION:
            case RNG_CONDITION_ITEM: {
                data[1] = floor_;
                break;
            }
            case RNG_INVENTORY_UPGRADE:
            case RNG_CHEST_BADGE_FIXED:
            case RNG_PARTNER:
            case RNG_STAR_POWER:
            case RNG_KISS_THIEF:
            case RNG_CHEST_BADGE_RANDOM:
            case RNG_AUDIENCE_ITEM:
            case RNG_FILENAME:
                break;
        }
        return third_party::fasthash64(
            data, sizeof(data), filename_seed_) % range;
    }
    return ttyd::system::irand(range);
}

void StateManager_v2::ClearAllOptions() {
    memset(option_flags_, 0, sizeof(option_flags_));
    memset(option_bytes_, 0, sizeof(option_bytes_));
    memset(play_stats_,   0, sizeof(play_stats_));
}

void StateManager_v2::SetDefaultOptions() {
    memset(option_flags_, 0, 12);
    memset(option_bytes_, 0, 6);
    // Set non-zero default values to their proper values.
    SetOption(OPT_CHEST_REWARDS, 3);
    SetOption(OPTVAL_STARTER_ITEMS_NORMAL);
    SetOption(OPTNUM_ENEMY_HP, 100);
    SetOption(OPTNUM_ENEMY_ATK, 100);
}

void StateManager_v2::ChangeOption(int32_t option, int32_t change) {
    const int32_t type = option >> 28;
    const int32_t max = option & 0xfff;
    int32_t value = GetOptionValue(option);

    if (type == 1) {
        // For flag values, wrap around, and advance if change is 0.
        if (change == 0) change = 1;
        value = (value + change) % max;
        if (value < 0) value += max;
        SetOption(option, value);
    } else if (type == 3 || type == 4) {
        // Saturate range, don't change on 0; range checking is done in Set.
        SetOption(option, value + change);
    }
}

void StateManager_v2::SetOption(int32_t option, int32_t value) {
    int32_t x, y, w, z;
    GetOptionParts(option, &x, &y, &w, &z);
    w &= 7;  // Mask out "changes seeding" bit.
    int32_t type = option >> 28;
    
    if (type == 1 || type == 2) {
        if (type == 2) {
            // OPTVAL, we know the value and that it must be valid.
            value = z;
        } else if (value < 0 || value >= z) {
            // Value out of range for this flag, can't do anything meaningful.
            return;
        }
        // Set the value.
        uint32_t* ptr = option_flags_ + (x >> 5);
        const uint32_t start_bit = x & 31;
        const uint32_t mask = GetBitMask(start_bit, start_bit + y - 1);
        *ptr = (*ptr & ~mask) | (value << start_bit);
    } else if (type == 3 || type == 4) {
        // Clamp value to the appropriate min and max range.
        if (w < 2) {
            // Clamp to [W, Z].
            if (value < w) value = w;
            if (value > z) value = z;
        } else if ((w == 2 || w == 3) && z > 0 && z < 10) {
            // Clamp to Z digits, either [0, 999...] or [-999..., 999...].
            static const constexpr int32_t powers_of_10[] = {
                1, 10, 100, 1000, 10'000, 100'000, 1'000'000, 10'000'000,
                100'000'000, 1'000'000'000
            };
            if (value >= powers_of_10[z]) value = powers_of_10[z] - 1;
            if (w == 2) {
                if (value < 0) value = 0;
            } else {
                if (value <= -powers_of_10[z]) value = -powers_of_10[z] + 1;
            }
        }
        // Set the value.
        uint8_t* ptr = (type == 3 ? option_bytes_ : play_stats_) + x;
        uint32_t uint_val = static_cast<uint32_t>(value);
        for (int32_t i = y - 1; i >= 0; --i) {
            ptr[i] = uint_val & 0xff;
            uint_val >>= 8;
        }
    }
}

int32_t StateManager_v2::GetOptionValue(int32_t option) const {
    int32_t type = option >> 28;
    if (type == 1) return GetOptionFlagValue(option);
    return GetOptionNumericValue(option);
}

int32_t StateManager_v2::GetOptionFlagValue(int32_t option) const {
    int32_t x, y, w, z;
    GetOptionParts(option, &x, &y, &w, &z);
    int32_t type = option >> 28;
    
    if (type == 1) {
        uint32_t word = option_flags_[x >> 5];
        const uint32_t start_bit = x & 31;
        int32_t value = GetShiftedBitMask(word, start_bit, start_bit + y - 1);
        // Return as OPTVAL_... type.
        return 0x20000000 | (option & 0x0ffff000) | value;
    }
    return -1;
}

int32_t StateManager_v2::GetOptionNumericValue(int32_t option) const {
    int32_t x, y, w, z;
    GetOptionParts(option, &x, &y, &w, &z);
    int32_t type = option >> 28;
    
    if (type == 1) {
        uint32_t word = option_flags_[x >> 5];
        const uint32_t start_bit = x & 31;
        return GetShiftedBitMask(word, start_bit, start_bit + y - 1);
    } else if (type == 3 || type == 4) {
        const uint8_t* ptr = (type == 3 ? option_bytes_ : play_stats_) + x;
        uint32_t uint_val = 0;
        if ((w & 7) == 3 && *ptr & 0x80) {
            // If option supports negative values and currently is negative,
            // fill the buffer with all 1-bits to properly represent it.
            uint_val = ~0;
        }
        for (int32_t i = 0; i < y; ++i) {
            uint_val = (uint_val << 8) + *ptr++;
        }
        return static_cast<int32_t>(uint_val);
    }
    return -1;
}

bool StateManager_v2::CheckOptionValue(int32_t option_value) const {
    if (option_value >> 28 != 2) return false;
    
    int32_t x, y, w, z;
    GetOptionParts(option_value, &x, &y, &w, &z);
    
    uint32_t word = option_flags_[x >> 5];
    const uint32_t start_bit = x & 31;
    const int32_t value = GetShiftedBitMask(word, start_bit, start_bit + y - 1);
    return value == z;
}

void StateManager_v2::GetOptionStrings(
    int32_t option, char* name_buf, char* value_buf,
    bool* is_default, bool* affects_seeding) const {
    const int32_t value = GetOptionValue(option);
    const int32_t num_value = GetOptionNumericValue(option);
    // Get option parts.
    int32_t x, y, w, z;
    GetOptionParts(option, &x, &y, &w, &z);
    
    // Set name.
    switch (option) {
        case OPT_CHEST_REWARDS: {
            strcpy(name_buf, "Rewards per chest:");         break;
        }
        case OPT_NO_EXP_MODE: {
            strcpy(name_buf, "No-EXP mode:");               break;
        }
        case OPT_BATTLE_REWARD_MODE: {
            strcpy(name_buf, "Battle drops:");              break;
        }
        case OPT_PARTNERS_OBTAINED: {
            strcpy(name_buf, "Partners obtained:");         break;
        }
        case OPT_PARTNER_RANK: {
            strcpy(name_buf, "Partner starting rank:");     break;
        }
        case OPT_BADGE_MOVE_LEVEL: {
            strcpy(name_buf, "Max badge move level:");      break;
        }
        case OPT_STARTER_ITEMS: {
            strcpy(name_buf, "Rewards per chest:");         break;
        }
        case OPT_FLOOR_100_HP_SCALE: {
            strcpy(name_buf, "Post-floor 100 HP gain:");    break;
        }
        case OPT_FLOOR_100_ATK_SCALE: {
            strcpy(name_buf, "Post-floor 100 ATK gain:");   break;
        }
        case OPT_MERLEE_CURSE: {
            strcpy(name_buf, "Infinite Merlee curses:");    break;
        }
        case OPT_STAGE_RANK: {
            strcpy(name_buf, "Stage rank-up after:");       break;
        }
        case OPT_PERCENT_BASED_DANGER: {
            strcpy(name_buf, "Danger/Peril thresholds:");   break;
        }
        case OPT_WEAKER_RUSH_BADGES: {
            strcpy(name_buf, "Power/Mega Rush power:");     break;
        }
        case OPT_EVASION_BADGES_CAP: {
            strcpy(name_buf, "Cap badge evasion at 80%:");  break;
        }
        case OPT_64_STYLE_HP_FP_DRAIN: {
            strcpy(name_buf, "PM64-style Drain badges:");   break;
        }
        case OPT_STAGE_HAZARDS: {
            strcpy(name_buf, "Stage hazard frequency:");    break;
        }
        case OPT_RANDOM_DAMAGE: {
            strcpy(name_buf, "Damage randomization:");      break;
        }
        case OPT_AUDIENCE_RANDOM_THROWS: {
            strcpy(name_buf, "Random audience items:");     break;
        }
        case OPT_CHET_RIPPO_APPEARANCE: {
            strcpy(name_buf, "Chet Rippo appearance:");     break;
        }
        case OPTNUM_ENEMY_HP: {
            strcpy(name_buf, "Enemy HP multiplier:");       break;
        }
        case OPTNUM_ENEMY_ATK: {
            strcpy(name_buf, "Enemy ATK multiplier:");      break;
        }
        case OPTNUM_SUPERGUARD_SP_COST: {
            strcpy(name_buf, "Superguard cost:");           break;
        }
        case OPTNUM_SWITCH_PARTY_FP_COST: {
            strcpy(name_buf, "Partner switch cost:");       break;
        }
    }
    
    // Check if option is default.
    if (option == OPTNUM_ENEMY_ATK || option == OPTNUM_ENEMY_HP) {
        *is_default = value == 100;
    } else if (option == OPT_CHEST_REWARDS) {
        *is_default = num_value == 3;
    } else if (option == OPT_STARTER_ITEMS) {
        *is_default = value == OPTVAL_STARTER_ITEMS_NORMAL;
    } else {
        *is_default = num_value == 0;
    }
    
    // Check top bit of 'W' nybble from option to see if it affects seeding.
    *affects_seeding = (w & 8) != 0;
    
    // Set value.
    
    // Options with special text.
    switch (value) {
        case OPTVAL_CHEST_REWARDS_RANDOM: {
            strcpy(value_buf, "Varies");                return;
        }
        case OPTVAL_NO_EXP_MODE_ON: {
            strcpy(value_buf, "On (99 BP)");            return;
        }
        case OPTVAL_NO_EXP_MODE_INFINITE: {
            strcpy(value_buf, "On (Infinite BP)");      return;
        }
        case OPTVAL_DROP_STANDARD: {
            strcpy(value_buf, "One held + bonus");      return;
        }
        case OPTVAL_DROP_HELD_FROM_BONUS: {
            strcpy(value_buf, "Held gated by bonus");   return;
        }
        case OPTVAL_DROP_NO_HELD_W_BONUS: {
            strcpy(value_buf, "No held, bonus only");   return;
        }
        case OPTVAL_DROP_ALL_HELD: {
            strcpy(value_buf, "All held + bonus");      return;
        }
        case OPTVAL_PARTNERS_ALL_REWARDS: {
            strcpy(value_buf, "All rewards");           return;
        }
        case OPTVAL_PARTNERS_ALL_START: {
            strcpy(value_buf, "Start with all");        return;
        }
        case OPTVAL_PARTNERS_ONE_START: {
            strcpy(value_buf, "Start with one");        return;
        }
        case OPTVAL_PARTNERS_NEVER: {
            strcpy(value_buf, "Never");                 return;
        }
        case OPTVAL_PARTNER_RANK_NORMAL: {
            strcpy(value_buf, "Normal");                return;
        }
        case OPTVAL_PARTNER_RANK_SUPER: {
            strcpy(value_buf, "Super");                 return;
        }
        case OPTVAL_PARTNER_RANK_ULTRA: {
            strcpy(value_buf, "Ultra");                 return;
        }
        case OPTVAL_BADGE_MOVE_1X: {
            strcpy(value_buf, "1 per copy");            return;
        }
        case OPTVAL_BADGE_MOVE_2X: {
            strcpy(value_buf, "2 per copy");            return;
        }
        case OPTVAL_BADGE_MOVE_RANK: {
            strcpy(value_buf, "Rank-based");            return;
        }
        case OPTVAL_BADGE_MOVE_INFINITE: {
            strcpy(value_buf, "Always 99");             return;
        }
        case OPTVAL_STARTER_ITEMS_NORMAL: {
            strcpy(value_buf, "On");                    return;
        }
        case OPTVAL_STAGE_RANK_30_FLOORS: {
            strcpy(value_buf, "Floor 30/60/90");        return;
        }
        case OPTVAL_STAGE_RANK_ALWAYSMAX: {
            strcpy(value_buf, "Always Superstar");      return;
        }
        case OPTVAL_STAGE_HAZARDS_NORMAL: {
            strcpy(value_buf, "Default");               return;
        }
        case OPTVAL_STAGE_HAZARDS_HIGH: {
            strcpy(value_buf, "High");                  return;
        }
        case OPTVAL_STAGE_HAZARDS_LOW: {
            strcpy(value_buf, "Low");                   return;
        }
        case OPTVAL_STAGE_HAZARDS_NO_FOG: {
            strcpy(value_buf, "No Fog");                return;
        }
        case OPTVAL_STAGE_HAZARDS_OFF: {
            strcpy(value_buf, "Off");                   return;
        }
        case OPTVAL_RANDOM_DAMAGE_25: {
            strcpy(value_buf, "+/-25%");                return;
        }
        case OPTVAL_RANDOM_DAMAGE_50: {
            strcpy(value_buf, "+/-50%");                return;
        }
        case OPTVAL_CHET_RIPPO_50_ONWARD: {
            strcpy(value_buf, "Floor 50+");             return;
        }
        case OPTVAL_CHET_RIPPO_10_ONWARD: {
            strcpy(value_buf, "Floor 10+");              return;
        }
    }
    // Options with special formatting.
    switch (option) {
        case OPT_CHEST_REWARDS: {
            sprintf(value_buf, "%" PRId32, num_value);
            return;
        }
        case OPT_FLOOR_100_HP_SCALE: 
        case OPT_FLOOR_100_ATK_SCALE: {
            strcpy(value_buf, num_value ? "+10% / set" : "+5% / set");
            return;
        }
        case OPT_PERCENT_BASED_DANGER: {
            strcpy(value_buf, num_value ? "%-based" : "Fixed");
            return;
        }
        case OPT_WEAKER_RUSH_BADGES: {
            strcpy(value_buf, num_value ? "+1/+2" : "+2/+5");
            return;
        }
        case OPTNUM_ENEMY_HP:
        case OPTNUM_ENEMY_ATK: {
            sprintf(value_buf, "%" PRId32 "%s", num_value, "%");
            return;
        }
        case OPTNUM_SUPERGUARD_SP_COST: {
            sprintf(value_buf, "%.2f SP", num_value / 100.0f);
            return;
        }
        case OPTNUM_SWITCH_PARTY_FP_COST: {
            sprintf(value_buf, "%" PRId32 " FP", num_value);
            return;
        }
    }
    // Default to using "On"/"Off" for nonzero/zero values.
    strcpy(value_buf, num_value ? "On" : "Off");
}

const char* StateManager_v2::GetEncodedOptions() const {
    static char enc_options[16];

    uint64_t numeric_options = GetOptionValue(OPTNUM_ENEMY_HP);
    numeric_options <<= 12;
    numeric_options += GetOptionValue(OPTNUM_ENEMY_ATK);
    numeric_options <<= 12;
    numeric_options += GetOptionValue(OPTNUM_SUPERGUARD_SP_COST);
    numeric_options <<= 8;
    numeric_options += GetOptionValue(OPTNUM_SWITCH_PARTY_FP_COST);
    // Flip "starting items" flag off to make default options look simpler.
    uint32_t flag_options = option_flags_[0] ^ 0x4000;
    
    // Convert to a base-64 scheme using A-Z, a-z, 0-9, !, ?.
    // Format: 6+.6+.1 (FLAGS.NUMERIC.VERSION)
    for (int32_t i = 5; i >= 0; --i) {
        const int32_t sextet = flag_options & 63;
        enc_options[i] = GetBase64EncodingChar(sextet);
        flag_options >>= 6;
    }
    for (int32_t i = 12; i >= 7; --i) {
        const int32_t sextet = numeric_options & 63;
        enc_options[i] = GetBase64EncodingChar(sextet);
        numeric_options >>= 6;
    }
    enc_options[6]  = '.';
    enc_options[13] = '.';
    enc_options[14] = GetBase64EncodingChar(version_);
    enc_options[15] = '\0';
    return enc_options;
}

bool StateManager_v2::GetPlayStatsString(char* out_buf) const {
    const auto* mariost = ttyd::mariost::g_MarioSt;
    const uint64_t current_time = gc::OSTime::OSGetTime();
    const int64_t last_save_diff = current_time - last_save_time_;
    // If the start time was never initialized or the current time is before
    // the last save's time (indicating clock tampering), don't show any stats.
    if (last_save_diff < 0 || !pit_start_time_) {
        return false;
    }
    
    const int64_t start_diff    = current_time - pit_start_time_;
    const int32_t turn_total    = GetOptionValue(STAT_TURNS_SPENT);
    const int32_t battles_won   = (floor_ + 1) * 91 / 100;
    const int64_t battle_time   = 
        mariost->animationTimeIncludingBattle - mariost->animationTimeNoBattle;
    
    // Page 1: Seed, floor, options & total play time.
    out_buf += sprintf(
        out_buf,
        "<kanban>\n"
        "Seed: <col 0000ffff>%s</col>, Floor: <col ff0000ff>%" PRId32 "\n</col>"
        "Options: <col 0000ffff>%s\n</col>"
        "RTA Time: <col ff0000ff>",
        GetSavefileName(), floor_ + 1, GetEncodedOptions());
    out_buf += DurationTicksToFmtString(start_diff, out_buf);
    
    // Page 2: Battle time, Average time / battle, Average turns / battle.
    out_buf += sprintf(out_buf, "\n</col><k><p>In-battle time: ");
    out_buf += DurationTicksToFmtString(battle_time, out_buf);
    out_buf += sprintf(out_buf, "\nAvg. battle time: ");
    out_buf += DurationTicksToFmtString(battle_time / battles_won, out_buf);
    out_buf += sprintf(
        out_buf, "\nAvg. battle turns: %.2f", turn_total * 1.0f / battles_won);
    
    // Page 3: Total turn count, max turns, run away count.
    out_buf += sprintf(out_buf, "\n<k><p>Total turns: ");
    out_buf += IntegerToFmtString(turn_total, out_buf);
    out_buf += sprintf(
        out_buf, "\nMax turns: %d (Floor %d)", 
        GetOptionValue(STAT_MOST_TURNS_RECORD),
        GetOptionValue(STAT_MOST_TURNS_FLOOR));
    out_buf += sprintf(out_buf, "\nTimes ran away: ");
    out_buf += IntegerToFmtString(GetOptionValue(STAT_TIMES_RAN_AWAY), out_buf);
    
    // Page 4: Damage dealt / taken, Superguards hit.
    out_buf += sprintf(out_buf, "\n<k><p>Enemy damage taken: ");
    out_buf += IntegerToFmtString(GetOptionValue(STAT_ENEMY_DAMAGE), out_buf);
    out_buf += sprintf(out_buf, "\nPlayer damage taken: ");
    out_buf += IntegerToFmtString(GetOptionValue(STAT_PLAYER_DAMAGE), out_buf);
    out_buf += sprintf(out_buf, "\nSuperguards hit: ");
    out_buf += IntegerToFmtString(GetOptionValue(STAT_SUPERGUARDS), out_buf);
    
    // Page 5: FP spent, SP spent, Items used.
    out_buf += sprintf(out_buf, "\n<k><p>FP spent: ");
    out_buf += IntegerToFmtString(GetOptionValue(STAT_FP_SPENT), out_buf);
    out_buf += sprintf(out_buf, "\nSP spent: ");
    out_buf += IntegerToFmtString(GetOptionValue(STAT_SP_SPENT), out_buf);
    out_buf += sprintf(out_buf, "\nItems used: ");
    out_buf += IntegerToFmtString(GetOptionValue(STAT_ITEMS_USED), out_buf);
    
    // Page 6: Coins earned / spent, Shine Sprites used.
    out_buf += sprintf(out_buf, "\n<k><p>Coins earned: ");
    out_buf += IntegerToFmtString(GetOptionValue(STAT_COINS_EARNED), out_buf);
    out_buf += sprintf(out_buf, "\nCoins spent: ");
    out_buf += IntegerToFmtString(GetOptionValue(STAT_COINS_SPENT), out_buf);
    out_buf += sprintf(out_buf, "\nShine Sprites used: ");
    out_buf += IntegerToFmtString(GetOptionValue(STAT_SHINE_SPRITES), out_buf);
    
    // TODO: Add page for Items, badges, level-ups sold?
    out_buf += sprintf(out_buf, "\n<k>");
    
    return true;
}

void StateManager_v2::SaveCurrentTime(bool pit_start) {
    uint64_t current_time = gc::OSTime::OSGetTime();
    if (pit_start) pit_start_time_ = current_time;
    last_save_time_ = current_time;
}

const char* StateManager_v2::GetCurrentTimeString() {
    static char buf[16];
    const uint64_t current_time = gc::OSTime::OSGetTime();
    const int64_t start_diff = current_time - pit_start_time_;
    const int64_t last_save_diff = current_time - last_save_time_;
    // If the time since the last save is negative or the time was never set,
    // return the empty string and clear the timebases previously set.
    if (last_save_diff < 0 || !pit_start_time_) {
        pit_start_time_ = 0;
        last_save_time_ = 0;
        return "";
    }
    // Otherwise, return the time since start as a string.
    DurationTicksToFmtString(start_diff, buf);
    return buf;
}

}