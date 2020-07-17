#pragma once

#include "common_types.h"

#include <ttyd/seqdrv.h>

#include <cstdint>

namespace mod {

extern const uint32_t kTik06RightBeroEntryOffset;
extern const uint32_t kPitBattleSetupTblOffset;
extern const uint32_t kPitEnemy100Offset;
extern const uint32_t kPitTreasureTableOffset;
extern const uint32_t kPitRewardFloorReturnBeroEntryOffset;
extern const uint32_t kPitBossFloorReturnBeroEntryOffset;
extern const uint32_t kPitEvtOpenBoxOffset;
extern const uint32_t kPitFloorIncrementEvtOffset;
extern const uint32_t kPitEnemySetupEvtOffset;
extern const uint32_t kPitOpenPipeEvtOffset;
extern const uint32_t kPitBonetailFirstEvtOffset;
extern const uint32_t kPitChainChompSetHomePosFuncOffset;
extern const uint32_t kPitSetupNpcExtraParametersFuncOffset;
extern const uint32_t kPitSetKillFlagFuncOffset;

// Returns true if the current and next game sequence matches the given one.
bool CheckSeq(ttyd::seqdrv::SeqIndex sequence);
// Returns true if in normal gameplay (not in title, game over, etc. sequence)
bool InMainGameModes();

// Returns the string name of a relocatable module from its id.
const char* ModuleNameFromId(ModuleId::e module_id);

// For custom event support; allows calling subroutines / user_funcs in
// relocatable modules.
# define REL_PTR(module_id, offset) \
    (static_cast<int32_t>( \
        ((0x40 + static_cast<int32_t>(module_id)) << 24) + offset))

// Replaces REL_PTR function addresses for USER_FUNC and RUN_EVT-variants with
// their actual addresses, and vice versa, with the following caveats:
// - Only replaces the addresses of evt code or user_funcs, not any params.
// - Naively assumes that any address after `module_ptr` is part of the module.
// - Supports module_ids < 0x40 only.
void LinkCustomEvt(ModuleId::e module_id, void* module_ptr, int32_t* evt);
void UnlinkCustomEvt(ModuleId::e module_id, void* module_ptr, int32_t* evt);

}