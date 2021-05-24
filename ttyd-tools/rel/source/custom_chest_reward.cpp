#include "custom_chest_reward.h"

#include "common_functions.h"
#include "custom_item.h"
#include "mod.h"
#include "mod_state.h"

#include <ttyd/item_data.h>
#include <ttyd/mario_pouch.h>

#include <cstdint>

namespace mod::infinite_pit {

namespace {

using ::ttyd::mario_pouch::PouchData;

namespace ItemType = ::ttyd::item_data::ItemType;

}

int16_t PickChestReward() {
    static constexpr const int16_t kRewards[] = {
        // Mario / inventory upgrades (items 0 - 4).
        ItemType::STRANGE_SACK, ItemType::SUPER_BOOTS, ItemType::ULTRA_BOOTS,
        ItemType::SUPER_HAMMER, ItemType::ULTRA_HAMMER,
        // Star Powers (items 5 - 12).
        ItemType::MAGICAL_MAP, ItemType::DIAMOND_STAR, ItemType::EMERALD_STAR,
        ItemType::GOLD_STAR, ItemType::RUBY_STAR, ItemType::SAPPHIRE_STAR,
        ItemType::GARNET_STAR, ItemType::CRYSTAL_STAR,
        // Unique badges (items 13 - 24).
        ItemType::CHILL_OUT, ItemType::DOUBLE_DIP, ItemType::DOUBLE_DIP,
        ItemType::DOUBLE_DIP_P, ItemType::DOUBLE_DIP_P, ItemType::FEELING_FINE,
        ItemType::FEELING_FINE_P, ItemType::LUCKY_START, ItemType::QUICK_CHANGE,
        ItemType::RETURN_POSTAGE, ItemType::ZAP_TAP, ItemType::SPIKE_SHIELD,
        // Partners (represented by dummy values); (items 25 - 31).
        -1, -2, -3, -4, -5, -6, -7
    };
    static_assert(sizeof(kRewards) == 32 * sizeof(int16_t));
    
    uint8_t weights[34];
    for (int32_t i = 0; i < 32; ++i) weights[i] = 10;
    
    StateManager& state = g_Mod->state_;
    const PouchData& pouch = *ttyd::mario_pouch::pouchGetPtr();
    // Modify the chance of getting a partner based on the current floor
    // and the number of partners currently obtained.
    int32_t num_partners = 0;
    for (int32_t i = 0; i < 8; ++i) {
        num_partners += pouch.party_data[i].flags & 1;
    }
    
    if (state.floor_ == 29 && num_partners == 0) {
        // Floor 30; force a partner if you don't already have one.
        for (int32_t i = 0; i < 32; ++i) {
            if (kRewards[i] >= 0) weights[i] = 0;
        }
        weights[32] = 0;
        weights[33] = 0;
    } else {
        // Set Strange Sack's weight to be decently high.
        weights[0] = 40;
        // Set Mario's upgrade weights to be moderately likely.
        for (int32_t i = 1; i < 4; ++i) weights[i] = 15;
        
        // Determine the weight for a partner based on how many you have
        // currently and how deep in the Pit you are.
        int32_t partner_weight = 0;
        if (num_partners < 7) {
            partner_weight = 10;
            if (state.floor_ > 30) {
                // Decrease average weight, but increase total considerably 
                // if behind expected count (2 by floor 50, 3 by floor 70, etc.)
                partner_weight = 5;
                int32_t expected_partners = (state.floor_ - 9) / 20;
                if (num_partners < expected_partners) {
                    partner_weight += 
                        30 * (expected_partners - num_partners) 
                           / (7 - num_partners);
                }
                if (partner_weight > 100) partner_weight = 100;
            }
        }
        
        // Determine the weight for Crystal Stars based on how many are yet to
        // be obtained; the total weight should be 10x unobtained count, split
        // among all the ones that can be afforded.
        int32_t sp_weight = 0;
        const int32_t number_star_powers = state.StarPowersObtained();
        switch (8 - number_star_powers) {
            case 8: sp_weight = 80 / 1; break;
            case 7: sp_weight = 70 / 2; break;
            case 6: sp_weight = 60 / 2; break;
            case 5: sp_weight = 50 / 4; break;
            case 4: sp_weight = 40 / 3; break;
            default:    sp_weight = 10; break;
        }
        for (int32_t i = 5; i <= 12; ++i) weights[i] = sp_weight;
        
        // The unique badges should take up about as much weight as Star Powers;
        // this makes them less likely than the average reward early on but
        // more likely the fewer there are remaining.
        int32_t num_unique_badges_remaining =
            12 - CountSetBits(GetBitMask(13, 24) & state.reward_flags_);
        int32_t unique_badge_weight = 15 + 5 * num_unique_badges_remaining;
        if (num_unique_badges_remaining > 0) {
            for (int32_t i = 13; i <= 24; ++i) {
                weights[i] = unique_badge_weight / num_unique_badges_remaining;
            }
            // Make Spike Shield specifically a bit more likely.
            weights[24] *= 3;
        }
        
        // Disable rewards that shouldn't be received out of order or that
        // have already been claimed, and assign partner weight to partners.
        for (int32_t i = 0; i < 32; ++i) {
            if (state.reward_flags_ & (1 << i)) {
                weights[i] = 0;
                continue;
            }
            switch (kRewards[i]) {
                case ItemType::ULTRA_BOOTS:
                    if (pouch.jump_level < 2) {
                        weights[i] = 0;
                        weights[i-1] += 10;     // Make Super a bit more likely.
                    }
                    break;
                case ItemType::ULTRA_HAMMER:
                    if (pouch.hammer_level < 2) {
                        weights[i] = 0;
                        weights[i-1] += 10;     // Make Super a bit more likely.
                    }
                    break;
                case ItemType::DIAMOND_STAR:
                case ItemType::EMERALD_STAR:
                    if (number_star_powers < 1) weights[i] = 0;
                    break;
                case ItemType::GOLD_STAR:
                    if (number_star_powers < 2) weights[i] = 0;
                    break;
                case ItemType::RUBY_STAR:
                case ItemType::SAPPHIRE_STAR:
                case ItemType::GARNET_STAR:
                    if (number_star_powers < 3) weights[i] = 0;
                    break;
                case ItemType::CRYSTAL_STAR:
                    if (number_star_powers < 5) weights[i] = 0;
                    break;
                case -1:
                case -2:
                case -3:
                case -4:
                case -5:
                case -6:
                case -7:
                    weights[i] = partner_weight;
                default:
                    break;
            }
        }
        
        // Set weights for a Shine Sprite (32) or random pool badge (33).
        weights[32] = state.floor_ > 30 ? (state.floor_ < 100 ? 20 : 10) : 0;
        weights[33] = state.floor_ < 100 ? 10 : 20;
    }
    
    int32_t sum_weights = 0;
    for (int32_t i = 0; i < 34; ++i) sum_weights += weights[i];
    
    int32_t weight = state.Rand(sum_weights);
    int32_t reward_idx = 0;
    for (; (weight -= weights[reward_idx]) >= 0; ++reward_idx);
    
    int16_t reward;
    if (reward_idx < 32) {
        // Assign the selected reward and mark it as collected.
        reward = kRewards[reward_idx];
        state.reward_flags_ |= (1 << reward_idx);
    } else if (reward_idx == 32) {
        // Shine Sprite item.
        reward = ItemType::GOLD_BAR_X3;
    } else {
        // Pick a random pool badge.
        reward = PickRandomItem(/* seeded = */ true, 0, 0, 1, 0);
    }
    
    // If partners were unlocked from beginning and a partner was selected,
    // replace the reward with an extra Shine Sprite.
    if (reward < 0 && g_Mod->state_.GetOptionValue(
        StateManager::START_WITH_PARTNERS)) {
        reward = ItemType::GOLD_BAR_X3;
    }
    
    // If reward was Clock Out or Power Lift, swap them if the option is set.
    if (g_Mod->state_.GetOptionValue(
        StateManager::SWAP_CO_PL_SP_COST)) {
        if (reward == ItemType::EMERALD_STAR) {
            reward = ItemType::GOLD_STAR;
        } else if (reward == ItemType::GOLD_STAR) {
            reward = ItemType::EMERALD_STAR;
        }
    }
    
    return reward;
}

}