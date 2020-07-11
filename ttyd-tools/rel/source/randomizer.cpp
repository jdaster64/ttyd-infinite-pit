#include "randomizer.h"

#include "common_functions.h"
#include "common_ui.h"

#include <ttyd/dispdrv.h>
#include <ttyd/seqdrv.h>
#include <ttyd/seq_title.h>

#include <cstdint>

namespace mod::pit_randomizer {
    
namespace  {

using ::ttyd::dispdrv::CameraId;

void DrawTitleScreenInfo() {
    const char* kTitleInfo =
        "Pit of Infinite Trials v0.00 by jdaster64\nPUT GITHUB LINK HERE";
    DrawCenteredTextWindow(
        kTitleInfo, 0, -50, 0xFFu, 0xFFFFFFFFu, 0.75f, 0x000000E5u, 15, 10);
}

}
    
Randomizer::Randomizer() {}

void Randomizer::Init() {}

void Randomizer::Update() {}

void Randomizer::Draw() {
    if (CheckSeq(ttyd::seqdrv::SeqIndex::kTitle)) {
        const uint32_t curtain_state = *reinterpret_cast<uint32_t*>(
            reinterpret_cast<uintptr_t>(ttyd::seq_title::seqTitleWorkPointer2)
            + 0x8);
        if (curtain_state >= 2 && curtain_state < 12) {
            // Curtain is not fully down; draw title screen info.
            RegisterDrawCallback(DrawTitleScreenInfo, CameraId::k2d);
        }
    }
    (void)InMainGameModes();
}

}