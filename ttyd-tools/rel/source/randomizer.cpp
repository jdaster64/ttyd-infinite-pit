#include "randomizer.h"

#include "common_ui.h"

#include <ttyd/dispdrv.h>
#include <ttyd/seqdrv.h>
#include <ttyd/system.h>

#include <cstdint>
#include <cstring>

namespace mod::pit_randomizer {
    
namespace  {
    
using ::ttyd::dispdrv::CameraId;
using ::ttyd::system::keyGetButtonTrg;

// TODO: For testing library functions only; delete when done.
const char* kNextMap = reinterpret_cast<const char*>(0x8041CF20 + 0x16A8);
int32_t scale = 100;
int32_t alignment = 0;
int32_t string_idx = 0;
const char* kStrings[] = {
    "A",
    "Test",
    "Test Test",
    "Test\nTest",
    "Test\n\nTest",
    "Test Test\nTest Test",
    "Test\nTest Test"
};

bool InMainGameModes() {
    int32_t sequence = ttyd::seqdrv::seqGetNextSeq();
    int32_t game = static_cast<int32_t>(ttyd::seqdrv::SeqIndex::kGame);
    int32_t game_over = static_cast<int32_t>(ttyd::seqdrv::SeqIndex::kGameOver);

    bool next_map_demo = !strcmp(kNextMap, "dmo_00");
    bool next_map_title = !strcmp(kNextMap, "title");
    
    return (sequence >= game) && (sequence <= game_over) && 
           !next_map_demo && !next_map_title;
}

void TestDraw() {
    // If given buttons are pressed, alter the text parameters.
    if (keyGetButtonTrg(0) & 0x1) {
        string_idx = string_idx == 0 ? 6 : string_idx - 1;
    }
    if (keyGetButtonTrg(0) & 0x2) {
        string_idx = (string_idx + 1) % 7;
    }
    if (keyGetButtonTrg(0) & 0x4) {
        alignment = (alignment + 1) % 10;
    }
    if (keyGetButtonTrg(0) & 0x8) {
        alignment = alignment == 0 ? 9 : alignment - 1;
    }
    if (keyGetButtonTrg(0) & 0x10) {
        scale = 100;
        alignment = 9;
        string_idx = 0;
    }
    if (keyGetButtonTrg(0) & 0x20) {
        scale = scale <= 10 ? 10 : scale - 5;
    }
    if (keyGetButtonTrg(0) & 0x40) {
        scale += 5;
    }
    
    // Centered window for reference.
    DrawWindow(0xdd, -50, 50, 100, 100, 10);
    if (alignment != 9) {
        // Draw text at the origin with the desired alignment and scale.
        DrawText(kStrings[string_idx], 0, 0, ~0, ~0, scale * 0.01f, alignment);
    } else {
        // Draw a text window at the origin.
        DrawCenteredTextWindow(
            kStrings[string_idx], 0, 0, ~0, ~0, scale * 0.01f, ~0, 15, 10);
    }
}

}
    
Randomizer::Randomizer() {}

void Randomizer::Init() {}

void Randomizer::Update() {}

void Randomizer::Draw() {
    if (InMainGameModes()) RegisterDrawCallback(TestDraw, CameraId::k2d);
}

}