#pragma once

#include <cstdint>

namespace mod::infinite_pit {

class AchievementsManager {
public:
    // Checks whether to award key items for completing various tasks.
    static void Update();
    // Code that runs drawing-related code every frame.
    static void Draw();
    
    // Item types to award for completing achievement tasks.
    static const int32_t kChestRewardItem;
    static const int32_t kBadgeLogItem;
    static const int32_t kTattleLogItem;
};
 
}