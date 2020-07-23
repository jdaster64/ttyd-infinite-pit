#pragma once

#include <cstdint>

namespace ttyd::battle_unit {
struct BattleWorkUnit;
}

namespace ttyd::battle {

extern "C" {

// .text
// BattleConsumeReserveItem
// BattleStatusWindowCheck
// BattleStatusWindowSystemOff
// BattleStatusWindowEventOff
// BattleStatusWindowSystemOn
// BattleStatusWindowEventOn
// BattleStatusWindowAPRecoveryOff
// BattleStatusWindowAPRecoveryOn
// BattleMajinaiEndCheck
// BattleMajinaiDone
// BattleMajinaiCheck
// battleDisableHResetCheck
// BattleAfterReactionMain
// BattleAfterReactionRelease
// BattleAfterReactionEntry
// BattleAfterReactionQueueInit
// BattleCheckUnitBroken
// BattleGetFloorHeight
// BattleGetStockExp
// BattleStoreExp
// BattleStoreCoin
// BattlePartyInfoWorkInit
// _EquipItem
// BtlUnit_EquipItem
// BattleTransPartyIdToUnitKind
// BattleTransPartyId
// BattleChangeParty
// BattlePartyAnimeLoad
// BattleGetPartnerPtr
// BattleGetPartyPtr
// BattleGetMarioPtr
// BattleGetSystemPtr
// BattleGetUnitPartsPtr
// BattleSetUnitPtr
battle_unit::BattleWorkUnit* BattleGetUnitPtr(void* battleWork, int32_t idx);
// BattleFree
// BattleAlloc
// BattleIncSeq
// BattleGetSeq
// BattleSetSeq
// BattleSetMarioParamToFieldBattle
// Btl_UnitSetup
// BattleEnd
// BattleMain
// BattleInit
// battleSeqEndCheck
// battleMain

 // .data
extern void* g_BattleWork; 

}

}