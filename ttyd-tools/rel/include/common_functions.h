#pragma once

#include <ttyd/seqdrv.h>

#include <cstdint>

namespace mod {

extern const uint32_t kPitTreasureTableOffset;
extern const uint32_t kPitEvtOpenBoxOffset;

// Returns true if the current and next game sequence matches the given one.
bool CheckSeq(ttyd::seqdrv::SeqIndex sequence);
// Returns true if in normal gameplay (not in title, game over, etc. sequence)
bool InMainGameModes();

}