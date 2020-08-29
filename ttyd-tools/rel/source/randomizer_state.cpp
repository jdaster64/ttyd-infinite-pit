#include "randomizer_state.h"

#include "patch.h"
#include "randomizer.h"

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

void* GetSavedStateLocation() {
    // Store randomizer state in stored items space, since this won't be used,
    // and the first byte of any possible stored item produces a "version" of 0.
    // Starts at index 1 to align to 4-byte boundary.
    return &ttyd::mario_pouch::pouchGetPtr()->stored_items[1];
}

// Updates partners' Ultra Rank max HP based how many times each partner
// has had a Shine Sprite used on them (each after the second adds +5 max HP).
void InitPartyMaxHpTable(uint8_t* partner_upgrades) {
    static constexpr const int16_t kDefaultUltraRankMaxHp[] = {
        30, 25, 40, 30, 35, 30, 25
    };
    int16_t* hp_table = ttyd::mario_pouch::_party_max_hp_table + 4;
    for (int32_t i = 0; i < 7; ++i) {
        const int32_t addl_hp = partner_upgrades[i] > 2 ? 
            (partner_upgrades[i] - 2) * 5 : 0;
        hp_table[i * 4 + 2] = kDefaultUltraRankMaxHp[i] + addl_hp;
    }
}

bool LoadFromPreviousVersion(RandomizerState* state) {
    void* saved_state = GetSavedStateLocation();
    uint8_t version = *reinterpret_cast<uint8_t*>(saved_state);
    if (version < 1) {
        // Version is 0 or incompatible with the current version; fail to load.
        return false;
    }
    
    // Version is compatible; load, making any adjustments necessary.
    if (version == 2) {
        patch::writePatch(state, saved_state, sizeof(RandomizerState));
    } else if (version == 1) {
        patch::writePatch(state, saved_state, sizeof(RandomizerState));
        state->hp_multiplier_ = 100;
        state->atk_multiplier_ = 100;
        state->options_ = 2;
    }
    
    state->version_ = 2;
    InitPartyMaxHpTable(state->partner_upgrades_);
    return true;
}

}

bool RandomizerState::Load(bool new_save) {
    if (!new_save) return LoadFromPreviousVersion(this);
    
    version_ = 2;
    floor_ = 0;
    reward_flags_ = 0x00000000;
    load_from_save_ = false;
    disable_partner_badges_in_shop_ = true;
    for (int32_t i = 0; i < 7; ++i) partner_upgrades_[i] = 0;
    InitPartyMaxHpTable(partner_upgrades_);
    
    // Default options: All optional flags off, 2 chests / floor.
    hp_multiplier_ = 100;
    atk_multiplier_ = 100;
    options_ = 2;
    
    // Seed the rng based on the filename.
    // If the filename is a few variants of "random" or a single 'star',
    // pick a random replacement filename.
    const char* filename = GetSavefileName();
    for (const char* ch = filename; *ch; ++ch) {
        // If the filename contains a 'heart' character, start w/FX badges.
        if (*ch == '\xd0') options_ |= RandomizerState::START_WITH_FX;
    }
    if (!strcmp(filename, "random") || !strcmp(filename, "Random") ||
        !strcmp(filename, "RANDOM") || !strcmp(filename, "\xde") ||
        !strcmp(filename, "random\xd0") || !strcmp(filename, "Random\xd0") ||
        !strcmp(filename, "RANDOM\xd0") || !strcmp(filename, "\xde\xd0")) { 
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
    void* saved_state = GetSavedStateLocation();
    patch::writePatch(saved_state, this, sizeof(RandomizerState));
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