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
        const int32_t addl_hp = partner_upgrades[i] > 2 ? 
            (partner_upgrades[i] - 2) * 5 : 0;
        hp_table[i * 4 + 2] = kDefaultUltraRankMaxHp[i] + addl_hp;
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
            int32_t ch = Rand(52);
            if (ch < 26) {
                filenameChars[i] = ch + 'a';
            } else {
                filenameChars[i] = (ch - 26) + 'A';
            }
        }
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
            options_ ^= NO_EXP_MODE;
            if (options_ & NO_EXP_MODE) {
                pouch.rank = 3;
                pouch.level = 99;
                pouch.unallocated_bp += 90;
                pouch.total_bp += 90;
            } else {
                pouch.rank = 0;
                pouch.level = 1;
                pouch.unallocated_bp -= 90;
                pouch.total_bp -= 90;
            }
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
        case SUPERGUARDS_COST_FP: {
            options_ ^= SUPERGUARDS_COST_FP;
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
    }
}

int32_t RandomizerState::GetOptionValue(int32_t option) const {
    switch (option) {
        case NUM_CHEST_REWARDS:
            return (options_ & NUM_CHEST_REWARDS);
        case HP_MODIFIER:
            return hp_multiplier_;
        case ATK_MODIFIER:
            return atk_multiplier_;
        case SWITCH_PARTY_COST_FP:
            return (options_ & SWITCH_PARTY_COST_FP) / (SWITCH_PARTY_COST_FP/3);
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
        case NO_EXP_MODE: {
            sprintf(name, "No EXP, Max BP mode:");
            break;
        }
        case START_WITH_NO_ITEMS: {
            sprintf(name, "Starter set of items:");
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
        case HP_MODIFIER: {
            sprintf(name, "Enemy HP multiplier:");
            break;
        }
        case ATK_MODIFIER: {
            sprintf(name, "Enemy ATK multiplier:");
            break;
        }
    }
    
    // Set value.
    int32_t option_value = GetOptionValue(option);
    *color = 0;
    switch (option) {
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
        case HP_MODIFIER: 
        case ATK_MODIFIER: {
            sprintf(value, "%" PRId32 "%s", option_value, "%");
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
    return CountSetBits(GetBitMask(5, 12) & reward_flags_) > 0;
}

}