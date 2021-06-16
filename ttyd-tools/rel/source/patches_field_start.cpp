#include "patches_field_start.h"

#include "common_types.h"
#include "evt_cmd.h"
#include "mod.h"
#include "mod_state.h"
#include "patch.h"
#include "patches_options.h"
#include "patches_partner.h"

#include <ttyd/evt_bero.h>
#include <ttyd/evt_mario.h>
#include <ttyd/evt_pouch.h>
#include <ttyd/item_data.h>
#include <ttyd/mario_pouch.h>

#include <cstdint>

namespace mod::infinite_pit {

namespace {

using ::ttyd::evt_bero::BeroEntry;

namespace ItemType = ::ttyd::item_data::ItemType;

}

// Function hooks.
// Patch addresses.
extern const int32_t g_tik_06_PitBeroEntryOffset;
extern const int32_t g_tik_06_RightBeroEntryOffset;

namespace field_start {
    
namespace {
    
// Declarations for USER_FUNCs.
EVT_DECLARE_USER_FUNC(InitOptionsOnPitEntry, 5)
EVT_DECLARE_USER_FUNC(IncrementYoshiColor, 0)

// Event that runs when taking the pipe to enter the Pit.
EVT_BEGIN(PitStartPipeEvt)
// Parameters are dummy values, and get overwritten during execution.
USER_FUNC(InitOptionsOnPitEntry, 0, 0, 0, 0, 0)
RETURN()
EVT_END()

// Event that runs when taking the right loading zone in the pre-Pit room.
EVT_BEGIN(PrePitRoomLoopEvt)
USER_FUNC(IncrementYoshiColor)
RETURN()
EVT_END()

// Initializes all selected options on initially entering the Pit.
EVT_DEFINE_USER_FUNC(InitOptionsOnPitEntry) {
    // Initialize number of upgrades per partner.
    const int32_t starting_rank = g_Mod->state_.GetOptionValue(
        StateManager::PARTNER_STARTING_RANK);
    for (int32_t i = 0; i < 7; ++i) {
        g_Mod->state_.partner_upgrades_[i] = starting_rank;
    }
    if (g_Mod->state_.GetOptionValue(
        StateManager::START_WITH_PARTNERS)) {
        // Enable and initialize all partners.
        for (int32_t i = 1; i <= 7; ++i) {
            evt->evtArguments[0] = i;
            ttyd::evt_pouch::evt_pouch_party_join(evt, isFirstCall);
            partner::InitializePartyMember(evt, isFirstCall);
        }
        // Put Goombella on the field.
        evt->evtArguments[0] = 0;
        evt->evtArguments[1] = 1;
        evt->evtArguments[2] = 0;
        evt->evtArguments[3] = 0;
        evt->evtArguments[4] = 0;
        ttyd::evt_mario::evt_mario_set_party_pos(evt, isFirstCall);
        // Do NOT disable partners from the reward pool; they will be
        // replaced with extra Shine Sprites.
    }
    if (g_Mod->state_.GetOptionValue(
        StateManager::START_WITH_SWEET_TREAT)) {
        // Give the player Sweet Treat + 1.00 SP.
        ttyd::mario_pouch::pouchGetItem(ItemType::MAGICAL_MAP);
        auto& pouch = *ttyd::mario_pouch::pouchGetPtr();
        pouch.max_sp += 100;
        pouch.current_sp = pouch.max_sp;
        pouch.star_powers_obtained |= 1;
        // Disable Sweet Treat from the rewards pool.
        g_Mod->state_.reward_flags_ |= (1 << 5);
    }
    if (g_Mod->state_.GetOptionValue(StateManager::NO_EXP_MODE)) {
        auto& pouch = *ttyd::mario_pouch::pouchGetPtr();
        pouch.level = 99;
        pouch.unallocated_bp += 90;
        pouch.total_bp += 90;
        if (g_Mod->state_.GetOptionValue(
                StateManager::RANK_UP_REQUIREMENT) != 
                StateManager::RANK_UP_BY_FLOOR) {
            // Start at Superstar rank, unless rank-ups are by floor number.
            pouch.rank = 3;
        }
    }
    if (g_Mod->state_.GetOptionValue(
            StateManager::RANK_UP_REQUIREMENT) == 
            StateManager::RANK_UP_ALWAYS_MAX) {
        ttyd::mario_pouch::pouchGetPtr()->rank = 3;
    }
    
    // TODO: Set up starting inventory.
    // TODO: Set up Merlee curse.
    
    options::ApplySettingBasedPatches();
    // Save the timestamp you entered the Pit.
    g_Mod->state_.SaveCurrentTime(/* pit_start = */ true);
    // All other options are handled immediately on setting them,
    // or are checked explicitly every time they are relevant.
    return 2;
}

// Increments the randomly selected Yoshi color & marks it as manually changed.
EVT_DEFINE_USER_FUNC(IncrementYoshiColor) {
    g_Mod->state_.options_ |= StateManager::YOSHI_COLOR_SELECT;
    int32_t color = ttyd::mario_pouch::pouchGetPartyColor(4);
    ttyd::mario_pouch::pouchSetPartyColor(4, (color + 1) % 7);
    return 2;
}

}

void ApplyFixedPatches() {}

void ApplyModuleLevelPatches(void* module_ptr, ModuleId::e module_id) {
    if (module_id != ModuleId::TIK || !module_ptr) return;
    const uint32_t module_start = reinterpret_cast<uint32_t>(module_ptr);
    
    // Run custom event code when entering the Pit pipe.
    BeroEntry* tik_06_pipe_bero = reinterpret_cast<BeroEntry*>(
        module_start + g_tik_06_PitBeroEntryOffset);
    tik_06_pipe_bero->out_evt_code = reinterpret_cast<void*>(
        const_cast<int32_t*>(PitStartPipeEvt));
    
    // Make tik_06 (pre-Pit room)'s right exit loop back to itself.
    BeroEntry* tik_06_e_bero = reinterpret_cast<BeroEntry*>(
        module_start + g_tik_06_RightBeroEntryOffset);
    tik_06_e_bero->target_map = "tik_06";
    tik_06_e_bero->target_bero = tik_06_e_bero->name;
    tik_06_e_bero->out_evt_code = reinterpret_cast<void*>(
        const_cast<int32_t*>(PrePitRoomLoopEvt));
}

}  // namespace field_start
}  // namespace mod::infinite_pit