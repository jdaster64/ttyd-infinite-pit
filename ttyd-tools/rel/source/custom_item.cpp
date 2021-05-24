#include "custom_item.h"

#include "mod.h"
#include "mod_state.h"

#include <ttyd/mario_pouch.h>
#include <ttyd/system.h>

#include <cstdint>

namespace mod::infinite_pit {

namespace {

using ::ttyd::mario_pouch::PouchData;

}

int32_t PickRandomItem(
    bool seeded, int32_t normal_item_weight, int32_t recipe_item_weight,
    int32_t badge_weight, int32_t no_item_weight, bool force_no_partner) {
    // Bitfields of whether each item is included in the pool or not;
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
    
    int32_t total_weight =
        normal_item_weight + recipe_item_weight + badge_weight + no_item_weight;
    int32_t result; 
    if (seeded) {
        result = g_Mod->state_.Rand(total_weight);
    } else {
        result = ttyd::system::irand(total_weight);
    }
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
                // Count available partners.
                const PouchData& pouch = *ttyd::mario_pouch::pouchGetPtr();
                int32_t num_partners = 0;
                for (int32_t i = 0; i < 8; ++i) {
                    num_partners += pouch.party_data[i].flags & 1;
                }
                // Exclude 'P' badges if no partners unlocked yet.
                bitfield = (num_partners && !force_no_partner)
                    ? kStackableBadges : kStackableBadgesNoP;
                len_bitfield = sizeof(kStackableBadges) / sizeof(uint16_t);
                offset = 0xf0;
            } else {
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
    if (seeded) {
        result = g_Mod->state_.Rand(num_items_seen);
    } else {
        result = ttyd::system::irand(num_items_seen);
    }
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

}