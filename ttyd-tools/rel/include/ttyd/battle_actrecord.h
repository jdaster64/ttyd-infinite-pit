#pragma once

#include <cstdint>

namespace ttyd::battle_actrecord {

extern "C" {

// _check_turn_count_0_end
// _check_turn_count_0_turn
// _check_no_use
// _check_use
void BtlActRec_JudgeRuleKeep();
void BtlActRec_JudgeTurnRuleKeep();
// BtlActRec_AddPoint
// BtlActRec_AddCount    

}

}