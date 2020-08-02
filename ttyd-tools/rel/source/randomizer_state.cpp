#include "randomizer_state.h"

#include <gc/OSTime.h>
#include <ttyd/mario_pouch.h>
#include <ttyd/mariost.h>

#include <cstdint>
#include <cstring>

namespace mod::pit_randomizer {
    
namespace {

const char* GetSavefileName() {
    return ttyd::mariost::g_MarioSt->saveFileName;
}

RandomizerState* GetSavedStateLocation() {
    // Store randomizer state in stored items space, since this won't be used,
    // and the first byte of any possible stored item produces a "version" of 0.
    // Starts at index 1 to align to 4-byte boundary.
    return reinterpret_cast<RandomizerState*>(
        &ttyd::mario_pouch::pouchGetPtr()->stored_items[1]);
}

}

bool RandomizerState::Load(bool new_save) {
    if (!new_save) {
        RandomizerState* saved_state = GetSavedStateLocation();
        
        if (saved_state->version_ == 1) {
            memcpy(this, saved_state, sizeof(RandomizerState));
            return true;
        }
        // TODO: Failing gracefully w/fallthrough for testing purposes;
        // failing to load should cause a Game Over in the actual game.
    }
    version_ = 1;
    for (int32_t i = 0; i < 7; ++i) partner_upgrades_[i] = 0;
    floor_ = 0;
    reward_flags_ = 0x00000000;
    for (int32_t i = 0; i < 6; ++i) charlieton_items_[i] = 0;
    debug_[0] = 2;
    
    // Seed the rng based on the filename.
    // If the filename is a few variants of "random" or a single 'star',
    // pick a random replacement filename.
    const char* filename = GetSavefileName();
    if (!strcmp(filename, "random") || !strcmp(filename, "Random") ||
        !strcmp(filename, "RANDOM") || !strcmp(filename, "\xde")) { 
        char filenameChars[9];
        rng_state_ = static_cast<uint32_t>(gc::OSTime::OSGetTime());
        for (int32_t i = 0; i < 8; ++i) {
            int32_t ch = Rand(52);
            if (ch < 26) {
                filenameChars[i] = ch + 'a';
            } else {
                filenameChars[i] = (ch - 26) + 'A';
            }
        }
        filenameChars[8] = 0;
        // Copy generated filename to MarioSt.
        strcpy(const_cast<char*>(filename), filenameChars);
    }
    SeedRng(filename);
    return true;
}

void RandomizerState::Save() {
    RandomizerState* saved_state = GetSavedStateLocation();
    memcpy(saved_state, this, sizeof(RandomizerState));
}

void RandomizerState::SeedRng(const char* str) {
    uint32_t hash = 0;
    for (const char* c = str; *c != 0; ++c) {
        hash = 37 * hash + *c;
    }
    rng_state_ = hash;
}

uint32_t RandomizerState::Rand(uint32_t range) {
    rng_state_ = rng_state_ * 0x41c64e6d + 12345;
    return ((rng_state_ >> 16) & 0x7fff) % range;
}

}