#include "common_functions.h"

#include <ttyd/seqdrv.h>
#include <ttyd/seq_mapchange.h>
#include <ttyd/seq_title.h>
#include <ttyd/system.h>

#include <cstdint>
#include <cstring>

namespace mod {

// TODO: #ifdef switch for multiple regions.
const uint32_t kTik06RightBeroEntryOffset = 0x1f2f4;
const uint32_t kPitBattleSetupTblOffset = 0x1d460;
const uint32_t kPitEnemy100Offset = 0xef90;
const uint32_t kPitTreasureTableOffset = 0x11320;
const uint32_t kPitRewardFloorReturnBeroEntryOffset = 0x12520;
const uint32_t kPitBossFloorReturnBeroEntryOffset = 0x12448;
const uint32_t kPitMoverLastSpawnFloorOffset = 0x119a0;
const uint32_t kPitCharlietonSpawnChanceOffset = 0x11ea4;
const uint32_t kPitEvtOpenBoxOffset = 0x11348;
const uint32_t kPitFloorIncrementEvtOffset = 0x123c4;
const uint32_t kPitEnemySetupEvtOffset = 0x102a4;
const uint32_t kPitOpenPipeEvtOffset = 0x120d0;
const uint32_t kPitBonetailFirstEvtOffset = 0x14570;
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
        // USER_FUNC, RUN_EVT, RUN_EVT_ID, RUN_CHILD_EVT
        if ((op & 0xff) >= 91 && (op & 0xff) <= 94) {
            if (static_cast<uint32_t>(*evt) < 0x80000000U) {
                *evt = static_cast<int32_t>(
                    static_cast<uint32_t>(*evt) - ((0x40U + module_id) << 24) +
                    reinterpret_cast<uint32_t>(module_ptr));
            }
        }
        // Skip forward by the number of args the operation has.
        evt += (op >> 16);
    } while (op != 1);
}

void UnlinkCustomEvt(ModuleId::e module_id, void* module_ptr, int32_t* evt) {
    int32_t op;
    do {
        op = *evt++;
        // USER_FUNC, RUN_EVT, RUN_EVT_ID, RUN_CHILD_EVT
        if ((op & 0xff) >= 91 && (op & 0xff) <= 94) {
            if (static_cast<uint32_t>(*evt) >= 
                reinterpret_cast<uint32_t>(module_ptr)) {
                *evt = static_cast<int32_t>(
                    static_cast<uint32_t>(*evt) + ((0x40U + module_id) << 24) -
                    reinterpret_cast<uint32_t>(module_ptr));
            }
        }
        // Skip forward by the number of args the operation has.
        evt += (op >> 16);
    } while (op != 1);
}

}