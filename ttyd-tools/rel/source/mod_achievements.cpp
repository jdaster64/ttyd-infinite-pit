#include "mod_achievements.h"

#include "common_functions.h"
#include "mod.h"
#include "mod_state.h"

#include <ttyd/battle_database_common.h>
#include <ttyd/item_data.h>
#include <ttyd/mario_pouch.h>
#include <ttyd/swdrv.h>
#include <ttyd/system.h>

#include <cinttypes>
#include <cstdio>
#include <cstring>

namespace mod::infinite_pit {
    
namespace {
    
using ::ttyd::mario_pouch::PouchData;
    
namespace BattleUnitType = ::ttyd::battle_database_common::BattleUnitType;
namespace ItemType = ::ttyd::item_data::ItemType;

}

// Definitions for class constants.
const int32_t AchievementsManager::kChestRewardItem   = 0x52;
const int32_t AchievementsManager::kBadgeLogItem      = 0x53;
const int32_t AchievementsManager::kTattleLogItem     = 0x54;

void AchievementsManager::Update() {
    StateManager_v2& state = g_Mod->state_;
    
    // Remove awarded items if debug mode was used at any point.
    if (state.GetOptionNumericValue(OPT_DEBUG_MODE_USED)) {
        ttyd::mario_pouch::pouchRemoveItem(kChestRewardItem);
        ttyd::mario_pouch::pouchRemoveItem(kBadgeLogItem);
        ttyd::mario_pouch::pouchRemoveItem(kTattleLogItem);
        return;
    }
    
    // Check to see if the player has received all unique chest rewards.
    int32_t num_rewards = CountSetBits(state.reward_flags_);
    bool has_all_rewards = state.star_power_levels_ == 0xffff;
    if (num_rewards < 17) has_all_rewards = false;
    if (num_rewards < 24 && !state.GetOptionValue(OPTVAL_PARTNERS_NEVER)) {
        has_all_rewards = false;
    }
    if (has_all_rewards && !ttyd::mario_pouch::pouchCheckItem(kChestRewardItem)) {
        ttyd::mario_pouch::pouchGetItem(kChestRewardItem);
    }
    
    // Check to see if the player has completed the badge log.
    int32_t num_badges = 0;
    for (int32_t i = ItemType::POWER_JUMP; i < ItemType::MAX_ITEM_TYPE; ++i) {
        // Ignore the Attack FX badges, since those are optional.
        if (i >= ItemType::ATTACK_FX_R && i <= ItemType::ATTACK_FX_P) continue;
        num_badges += ttyd::swdrv::swGet(0x80 + i - ItemType::POWER_JUMP);
    }
    if (num_badges == 80 && !ttyd::mario_pouch::pouchCheckItem(kBadgeLogItem)) {
        ttyd::mario_pouch::pouchGetItem(kBadgeLogItem);
    }
    
    // Check to see if the player has completed the Tattle log.
    int32_t num_tattles = 0;
    for (int32_t i = 0; i <= BattleUnitType::BONETAIL; ++i) {
        num_tattles += ttyd::swdrv::swGet(0x117a + i);
    }
    if (num_tattles == 94 &&
        !ttyd::mario_pouch::pouchCheckItem(kTattleLogItem)) {
        ttyd::mario_pouch::pouchGetItem(kTattleLogItem);
    }
}

}