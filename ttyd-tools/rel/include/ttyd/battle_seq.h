#pragma once

#include <cstdint>

namespace ttyd::battle_seq {

extern "C" {

// _set_haikei_entry_scale
// _mapobj_data_touch_scale
// BattleCheckConcluded
// BattleWaitAllActiveEvtEnd_NoBgSetEndWait
// BattleWaitAllActiveEvtEnd
// battleMakePhaseEvtTable
// battleSortPhaseMoveTable
// btlseqPhaseFirstProcess
// btlseqTurnFirstProcess
// btlseqAct
// BattlePhaseEndCheck
// btlseqMove
// btlseqPhase
void _rule_disp();
// btlseqTurn
// _set_effect_luck
// btlseqFirstAct
// _GetFirstAttackWeapon
// btlseqInit
// BattleCheckAllPinchStatus
// BattleSequenceManager

}

}