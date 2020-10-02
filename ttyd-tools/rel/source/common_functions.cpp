#include "common_functions.h"

#include "evt_cmd.h"

#include <ttyd/mariost.h>
#include <ttyd/seqdrv.h>
#include <ttyd/seq_mapchange.h>
#include <ttyd/seq_title.h>
#include <ttyd/system.h>

#include <cstdint>
#include <cstring>

namespace mod {

// TODO: #ifdef switch for multiple regions.
const uint32_t kTik06PitBeroEntryOffset = 0x1f240;
const uint32_t kTik06RightBeroEntryOffset = 0x1f2f4;
const uint32_t kPitBattleSetupTblOffset = 0x1d460;
const uint32_t kPitEnemy100Offset = 0xef90;
const uint32_t kPitTreasureTableOffset = 0x11320;
const uint32_t kPitRewardFloorReturnBeroEntryOffset = 0x12520;
const uint32_t kPitBossFloorEntryBeroEntryOffset = 0x1240c;
const uint32_t kPitBossFloorReturnBeroEntryOffset = 0x12448;
const uint32_t kPitBossFloorSetupEvtOffset = 0x124c0;
const uint32_t kPitMoverLastSpawnFloorOffset = 0x119a0;
const uint32_t kPitCharlietonSpawnChanceOffset = 0x11ea4;
const uint32_t kPitCharlietonTalkEvtOffset = 0x11a1c;
const uint32_t kPitCharlietonTalkMinItemForBadgeDialogOffset = 0x11b1c;
const uint32_t kPitCharlietonTalkNoInvSpaceBranchOffset = 0x11c7c;
const uint32_t kPitEvtOpenBoxOffset = 0x11348;
const uint32_t kPitFloorIncrementEvtOffset = 0x123c4;
const uint32_t kPitEnemySetupEvtOffset = 0x102a4;
const uint32_t kPitOpenPipeEvtOffset = 0x120d0;
const uint32_t kPitBonetailFirstEvtOffset = 0x14570;
const uint32_t kPitReturnSignEvtOffset = 0x12374;
const uint32_t kPitChainChompSetHomePosFuncOffset = 0x2d0;
const uint32_t kPitSetupNpcExtraParametersFuncOffset = 0x388;
const uint32_t kPitSetKillFlagFuncOffset = 0x3e8;


bool CheckSeq(ttyd::seqdrv::SeqIndex sequence) {
    const ttyd::seqdrv::SeqIndex next_seq = ttyd::seqdrv::seqGetNextSeq();
    const ttyd::seqdrv::SeqIndex cur_seq = ttyd::seqdrv::seqGetSeq();
    return cur_seq == next_seq && next_seq == sequence;
}

bool InMainGameModes() {
    int32_t sequence = static_cast<int32_t>(ttyd::seqdrv::seqGetNextSeq());
    int32_t game = static_cast<int32_t>(ttyd::seqdrv::SeqIndex::kGame);
    int32_t game_over = static_cast<int32_t>(ttyd::seqdrv::SeqIndex::kGameOver);

    bool next_map_demo = !strcmp(ttyd::seq_mapchange::NextMap, "dmo_00");
    bool next_map_title = !strcmp(ttyd::seq_mapchange::NextMap, "title");
    
    return (sequence >= game) && (sequence <= game_over) && 
           !next_map_demo && !next_map_title;
}

const char* GetCurrentArea() {
    return ttyd::mariost::g_MarioSt->currentAreaName;
}

const char* GetCurrentMap() {
    return ttyd::mariost::g_MarioSt->currentMapName;
}

const char* GetNextMap() {
    return ttyd::seq_mapchange::NextMap;
}

const char* ModuleNameFromId(ModuleId::e module_id) {
    static const char* kModuleNames[] = {
        nullptr, "aaa", "aji", "bom", "dmo", "dou", "eki", "end",
        "gon", "gor", "gra", "hei", "hom", "jin", "jon", "kpa",
        "las", "moo", "mri", "muj", "nok", "pik", "rsh", "sys",
        "tik", "tou", "tou2", "usu", "win", "yuu"
    };
    return kModuleNames[module_id];
}
    
void LinkCustomEvt(ModuleId::e module_id, void* module_ptr, int32_t* evt) {
    int32_t op;
    do {
        op = *evt++;
        // Check each of the operation's arguments.
        for (int32_t* next_op = evt + (op >> 16); evt < next_op; ++evt) {
            if (static_cast<uint32_t>(*evt) >= 0x4000'0000U &&
                static_cast<uint32_t>(*evt) < 0x8000'0000U) {
                *evt = static_cast<int32_t>(
                    static_cast<uint32_t>(*evt) - ((0x40U + module_id) << 24) +
                    reinterpret_cast<uint32_t>(module_ptr));
            }
        }
    } while (op != 1);
}

void UnlinkCustomEvt(ModuleId::e module_id, void* module_ptr, int32_t* evt) {
    int32_t op;
    do {
        op = *evt++;
        // Check each of the operation's arguments.
        for (int32_t* next_op = evt + (op >> 16); evt < next_op; ++evt) {
            if (static_cast<uint32_t>(*evt) >=
                reinterpret_cast<uint32_t>(module_ptr) &&
                static_cast<uint32_t>(*evt) < 
                static_cast<uint32_t>(EVT_HELPER_POINTER_BASE)) {
                *evt = static_cast<int32_t>(
                    static_cast<uint32_t>(*evt) + ((0x40U + module_id) << 24) -
                    reinterpret_cast<uint32_t>(module_ptr));
            }
        }
    } while (op != 1);
}

int32_t CountSetBits(uint32_t x) {
    // PowerPC has no built-in pop_cnt instruction, apparently
    x = x - ((x >> 1) & 0x55555555);
    x = (x & 0x33333333) + ((x >> 2) & 0x33333333);
    return (((x + (x >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
}

uint32_t GetBitMask(uint32_t start_bit, uint32_t end_bit) {
    return (~0U >> (31-end_bit)) - (1U << start_bit) + 1;
}

}