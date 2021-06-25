#pragma once

#include <cstdint>

namespace mod::infinite_pit {

class AchievementsManager {
public:
    // Checks whether to award key items for completing various tasks.
    static void Update();
    
    // Item types to award for completing achievement tasks.
    static const int32_t kChestRewardItem;
    static const int32_t kBadgeLogItem;
    static const int32_t kTattleLogItem;
};
 
}