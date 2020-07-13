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
const uint32_t kPitTreasureTableOffset = 0x11320;
const uint32_t kPitEvtOpenBoxOffset = 0x11348;
const uint32_t kPitFloorIncrementEvtOffset = 0x123c4;
const uint32_t kPitRewardFloorReturnBeroEntryOffset = 0x12520;
const uint32_t kPitBossFloorReturnBeroEntryOffset = 0x12448;

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

}