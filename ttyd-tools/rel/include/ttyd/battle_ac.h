#pragma once

#include <cstdint>

namespace ttyd::battle_database_common {
struct BattleWeapon;
}
namespace ttyd::battle_unit {
struct BattleWorkUnit;
}

namespace ttyd::battle_ac {

extern "C" {

// BattleAcGaugeSeDelete
// BattleAcGaugeSeUpdate
// BattleAcGaugeSeInit
// BattleAcDrawGauge
// BattleACGetButtonIcon
// BattleActionCommandGetPrizeLv
// BattleActionCommandSetDifficulty
// BattleActionCommandGetDifficulty
// BattleActionCommandResetDefenceResult
// BattleActionCommandGetDefenceResult
// BattleACPadCheckRecordTrigger
int32_t BattleActionCommandCheckDefence(
    battle_unit::BattleWorkUnit* unit,
    battle_database_common::BattleWeapon* weapon);
// BattleActionCommandStop
// BattleActionCommandStart
// BattleActionCommandSetup
// BattleActionCommandDeclareACResult
// BattleActionCommandResult
// BattleActionCommandManager
// BattleActionCommandManagerInit

}

}