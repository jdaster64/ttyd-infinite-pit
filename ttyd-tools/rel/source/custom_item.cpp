#include "custom_item.h"

#include "mod.h"
#include "mod_state.h"

#include <ttyd/item_data.h>
#include <ttyd/system.h>

#include <cstdint>

namespace mod::infinite_pit {

namespace {

namespace ItemType = ::ttyd::item_data::ItemType;

// Bitfields of whether each item is included in its respective pool or not;
// item X is enabled if kItemPool[X / 16 - offset] & (1 << (X % 16)) != 0.
static constexpr const uint16_t kNormalItems[] = {
    0xffff, 0xffff, 0x000f, 0x0006
};
static constexpr const uint16_t kRecipeItems[] = {
    0x2020, 0xffb8, 0x3edf, 0x97ef, 0x0bff
};
static constexpr const uint16_t kStackableBadges[] = {
    0x3fff, 0xffff, 0x0fff, 0xfff7, 0x018f, 0x0030, 0x0006
};
static constexpr const uint16_t kStackableBadgesNoP[] = {
    0x3fff, 0x5555, 0x0b55, 0xaad7, 0x0186, 0x0030, 0x0002
};

bool CheckBitfieldBit(const uint16_t* bitfield, int32_t idx) {
    return bitfield[idx >> 4] & (1 << (idx & 0xf));
}

}

int32_t PickRandomItem(
    int32_t sequence, int32_t normal_item_weight, int32_t recipe_item_weight,
    int32_t badge_weight, int32_t no_item_weight) {
    StateManager_v2& state = g_Mod->state_;
    
    int32_t total_weight =
        normal_item_weight + recipe_item_weight + badge_weight + no_item_weight;
    int32_t result; 
    result = state.Rand(total_weight, sequence);
    const uint16_t* bitfield;
    int32_t len_bitfield;
    int32_t offset;
    
    int32_t current_weight = normal_item_weight;
    if (result < current_weight) {
        bitfield = kNormalItems;
        len_bitfield = sizeof(kNormalItems) / sizeof(uint16_t);
        offset = 0x80;
    } else {
        current_weight += recipe_item_weight;
        if (result < current_weight) {
            bitfield = kRecipeItems;
            len_bitfield = sizeof(kRecipeItems) / sizeof(uint16_t);
            offset = 0xa0;
        } else {
            current_weight += badge_weight;
            if (result < current_weight) {
                bool partners_enabled =
                    state.GetOptionNumericValue(OPT_ENABLE_P_BADGES);
                bitfield =
                    partners_enabled ? kStackableBadges : kStackableBadgesNoP;
                len_bitfield = sizeof(kStackableBadges) / sizeof(uint16_t);
                offset = 0xf0;
            } else {
                // "No item" chosen; make sure # of rand calls is consistent.
                state.Rand(1, sequence);
                return 0;
            }
        }
    }
    
    int32_t num_items_seen = 0;
    for (int32_t i = 0; i < len_bitfield; ++i) {
        for (int32_t bit = 0; bit < 16; ++bit) {
            if (bitfield[i] & (1U << bit)) ++num_items_seen;
        }
    }
    result = state.Rand(num_items_seen, sequence);
    num_items_seen = 0;
    for (int32_t i = 0; i < len_bitfield; ++i) {
        for (int32_t bit = 0; bit < 16; ++bit) {
            if (bitfield[i] & (1U << bit)) {
                if (result == num_items_seen) return offset + i * 16 + bit;
                ++num_items_seen;
            }
        }
    }
    // Should not be reached, as that would mean the random function returned
    // a larger index than there are bits in the bitfield.
    return -1;
}

bool IsStackableMarioBadge(int32_t item_type) {
    if (item_type < ItemType::POWER_JUMP || item_type >= ItemType::MAX_ITEM_TYPE)
        return false;
    // Partner badges are always one index ahead of their Mario equivalent.
    return IsStackablePartnerBadge(item_type + 1);
}

bool IsStackablePartnerBadge(int32_t item_type) {
    if (item_type < ItemType::POWER_JUMP || item_type >= ItemType::MAX_ITEM_TYPE)
        return false;
    return CheckBitfieldBit(kStackableBadges, item_type - ItemType::POWER_JUMP)
        && !CheckBitfieldBit(kStackableBadgesNoP, item_type - ItemType::POWER_JUMP);
}

}