#include "randomizer_patches.h"

#include "common_functions.h"
#include "common_types.h"
#include "evt_cmd.h"
#include "patch.h"

#include <gc/OSLink.h>
#include <ttyd/evt_bero.h>
#include <ttyd/evt_cam.h>
#include <ttyd/evt_eff.h>
#include <ttyd/evt_item.h>
#include <ttyd/evt_mario.h>
#include <ttyd/evt_mobj.h>
#include <ttyd/evt_msg.h>
#include <ttyd/evt_party.h>
#include <ttyd/evt_pouch.h>
#include <ttyd/evt_snd.h>
#include <ttyd/evtmgr.h>
#include <ttyd/item_data.h>
#include <ttyd/mario.h>
#include <ttyd/mario_party.h>
#include <ttyd/mario_pouch.h>
#include <ttyd/mariost.h>
#include <ttyd/win_main.h>
#include <ttyd/win_party.h>

#include <cinttypes>
#include <cstdio>
#include <cstring>

// Assembly patch functions, and code referenced in them.
extern "C" {
    void StartFixItemWinPartyDispOrder();
    void BranchBackFixItemWinPartyDispOrder();
    void StartFixItemWinPartySelectOrder();
    void BranchBackFixItemWinPartySelectOrder();
    void StartUsePartyRankup();
    void BranchBackUsePartyRankup();
    
    void usePartyRankup() {
        void* winPtr = ttyd::win_main::winGetPtr();
        const int32_t item = reinterpret_cast<int32_t*>(winPtr)[0x2d4 / 4];
        
        // If the item is a Shine Sprite... (otherwise, handle normally)
        if (item == ttyd::item_data::ItemType::GOLD_BAR_X3) {
            int32_t* party_member_target = 
                reinterpret_cast<int32_t*>(winPtr) + (0x2dc / 4);
            // If Mario is selected, target the first party member instead.
            if (*party_member_target == 0) *party_member_target = 1;
            
            ttyd::win_party::WinPartyData* party_data = 
                ttyd::win_party::g_winPartyDt;
            ttyd::mario_pouch::PouchPartyData* pouch_data = 
                ttyd::mario_pouch::pouchGetPtr()->party_data;
            const int32_t party_id = ttyd::mario_party::marioGetParty();
            
            // Determine the order the party members are shown in the menu.
            ttyd::win_party::WinPartyData* party_win_order[7];
            ttyd::win_party::WinPartyData** current_order = party_win_order;
            for (int32_t i = 0; i < 7; ++i) {
                if (party_data[i].partner_id == party_id) {
                    *current_order = party_data + i;
                    ++current_order;
                }
            }
            for (int32_t i = 0; i < 7; ++i) {
                int32_t id = party_data[i].partner_id;
                if ((pouch_data[id].flags & 1) && id != party_id) {
                    *current_order = party_data + i;
                    ++current_order;
                }
            }
            // Rank the selected party member up and fully heal them.
            const int32_t selected_partner_id =
                party_win_order[*party_member_target - 1]->partner_id;
            pouch_data += selected_partner_id;
            int16_t* hp_table = 
                ttyd::mario_pouch::_party_max_hp_table + selected_partner_id * 4;
            if (pouch_data->hp_level < 2) {
                ++pouch_data->hp_level;
                ++pouch_data->attack_level;
                ++pouch_data->tech_level;
            } else {
                // TODO: The number of HP ups will need to be saved somehow
                // if the player is allowed to save.
                if (hp_table[2] < 200) hp_table[2] += 5;
            }
            pouch_data->base_max_hp = hp_table[pouch_data->hp_level];
            pouch_data->current_hp = hp_table[pouch_data->hp_level];
            pouch_data->max_hp = hp_table[pouch_data->hp_level];
            // Include HP Plus P in current / max stats.
            const int32_t hp_plus_p_cnt = 
                ttyd::mario_pouch::pouchEquipCheckBadge(
                    ttyd::item_data::ItemType::HP_PLUS_P);
            pouch_data->current_hp += 5 * hp_plus_p_cnt;
            pouch_data->max_hp += 5 * hp_plus_p_cnt;
        }
    }
}

namespace mod::pit_randomizer {

namespace {

using ::gc::OSLink::OSModuleInfo;
using ::ttyd::evt_bero::BeroEntry;
using ::ttyd::item_data::itemDataTable;
using ::ttyd::item_data::ItemData;
using ::ttyd::mariost::g_MarioSt;

namespace ItemType = ::ttyd::item_data::ItemType;
namespace ItemUseLocation = ::ttyd::item_data::ItemUseLocation_Flags;

// Global variables.
uintptr_t g_PitModulePtr;
uintptr_t g_AdditionalModulePtr;
int32_t g_PitFloor = -1;

// Event that plays "get partner" fanfare.
EVT_BEGIN(PartnerFanfareEvt)
USER_FUNC(ttyd::evt_snd::evt_snd_bgmoff, 0x400)
USER_FUNC(ttyd::evt_snd::evt_snd_bgmon, 1, PTR("BGM_FF_GET_PARTY1"))
WAIT_MSEC(2000)
RETURN()
EVT_END()

// Event that handles a chest being opened.
EVT_BEGIN(ChestOpenEvt)
USER_FUNC(ttyd::evt_mario::evt_mario_key_onoff, 0)
USER_FUNC(ttyd::evt_mobj::evt_mobj_wait_animation_end, PTR("box"))
// TODO: Call a user func to select the "item" to spawn.
// In the event the "item" is actually a partner...
USER_FUNC(ttyd::evt_mario::evt_mario_normalize)
USER_FUNC(ttyd::evt_mario::evt_mario_goodbye_party, 0)
WAIT_MSEC(500)
// TODO: Parameterize which partner to spawn.
USER_FUNC(ttyd::evt_pouch::evt_pouch_party_join, 1)
// TODO: Reposition partner by box? At origin is fine...
USER_FUNC(ttyd::evt_mario::evt_mario_set_party_pos, 0, 1, 0, 0, 0)
RUN_EVT_ID(PartnerFanfareEvt, LW(11))
USER_FUNC(ttyd::evt_eff::evt_eff,
          PTR("sub_bg"), PTR("itemget"), 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
USER_FUNC(ttyd::evt_msg::evt_msg_toge, 1, 0, 0, 0)
// Prints a custom message; the "joined party" messages aren't loaded anyway.
USER_FUNC(ttyd::evt_msg::evt_msg_print, 0, PTR("pit_reward_party_join"), 0, 0)
CHK_EVT(LW(11), LW(12))
IF_EQUAL(LW(12), 1)
    DELETE_EVT(LW(11))
    USER_FUNC(ttyd::evt_snd::evt_snd_bgmoff, 0x201)
END_IF()
USER_FUNC(ttyd::evt_eff::evt_eff_softdelete, PTR("sub_bg"))
USER_FUNC(ttyd::evt_snd::evt_snd_bgmon, 0x120, 0)
USER_FUNC(ttyd::evt_snd::evt_snd_bgmon_f, 0x300, PTR("BGM_STG0_100DN1"), 1500)
WAIT_MSEC(500)
USER_FUNC(ttyd::evt_party::evt_party_run, 0)
USER_FUNC(ttyd::evt_party::evt_party_run, 1)
USER_FUNC(ttyd::evt_mario::evt_mario_key_onoff, 1)
RETURN()
EVT_END()

// Wrapper for modified chest-opening event.
EVT_BEGIN(ChestOpenEvtHook) 
RUN_CHILD_EVT(ChestOpenEvt)
RETURN()
EVT_END()

// Event that states an exit is disabled.
EVT_BEGIN(DisabledBeroEvt)
INLINE_EVT()
USER_FUNC(ttyd::evt_cam::evt_cam_ctrl_onoff, 4, 0)
USER_FUNC(ttyd::evt_mario::evt_mario_key_onoff, 0)
USER_FUNC(ttyd::evt_bero::evt_bero_exec_wait, 0x10000)
WAIT_MSEC(750)
USER_FUNC(ttyd::evt_msg::evt_msg_print, 0, PTR("pit_disabled_return"), 0, 0)
USER_FUNC(ttyd::evt_mario::evt_mario_key_onoff, 1)
USER_FUNC(ttyd::evt_cam::evt_cam_ctrl_onoff, 4, 1)
END_INLINE()
SET(LW(0), 0)
RETURN()
EVT_END()

// Event that increments a separate Pit floor counter, and updates the actual
// floor counter to be between 81-100 as appropriate if the actual floor > 100.
EVT_BEGIN(FloorIncrementEvt)
GET_RAM(LW(0), PTR(&g_PitFloor))
ADD(LW(0), 1)
SET_RAM(LW(0), PTR(&g_PitFloor))
ADD(GSW(1321), 1)
IF_LARGE_EQUAL(LW(0), 100)
    SET(LW(1), LW(0))
    MOD(LW(1), 10)
    IF_EQUAL(LW(1), 0)
        DIV(LW(0), 10)
        MOD(LW(0), 10)
        IF_EQUAL(LW(0), 9)
            SET(GSW(1321), 90)
        ELSE()
            SET(GSW(1321), 80)
        END_IF()
    END_IF()
END_IF()
RETURN()
EVT_END()

// Wrapper for modified floor-incrementing event.
EVT_BEGIN(FloorIncrementEvtHook)
RUN_CHILD_EVT(FloorIncrementEvt)
RETURN()
EVT_END()

}

void OnModuleLoaded(OSModuleInfo* module) {
    if (module == nullptr) return;
    int32_t module_id = module->id;
    uintptr_t module_ptr = reinterpret_cast<uintptr_t>(module);
    
    if (module_id == ModuleId::JON) {
        // Apply custom logic to box opening event to allow spawning partners.
        mod::patch::writePatch(
            reinterpret_cast<void*>(module_ptr + kPitEvtOpenBoxOffset),
            ChestOpenEvtHook, sizeof(ChestOpenEvtHook));
            
        // Disable reward floors' return pipe and display a message if entered.
        BeroEntry* return_bero = reinterpret_cast<BeroEntry*>(
            module_ptr + kPitRewardFloorReturnBeroEntryOffset);
        return_bero->target_map = nullptr;
        return_bero->target_bero = return_bero->name;
        return_bero->entry_evt_code = reinterpret_cast<void*>(
            const_cast<int32_t*>(DisabledBeroEvt));
        
        // Change destination of boss room pipe to be the usual 80+ floor.
        BeroEntry* boss_bero = reinterpret_cast<BeroEntry*>(
            module_ptr + kPitBossFloorReturnBeroEntryOffset);
        boss_bero->target_map = "jon_02";
        boss_bero->target_bero = "dokan_2";
        boss_bero->entry_evt_code = reinterpret_cast<void*>(
            module_ptr + kPitFloorIncrementEvtOffset);
        
        // Update the actual pit floor in tandem with GW(1321).
        // TODO: Remove this hack to initialize the pit floor.
        if (g_PitFloor < 0) {
            g_PitFloor = reinterpret_cast<uint8_t*>(g_MarioSt)[0xaa1];
        }
        mod::patch::writePatch(
            reinterpret_cast<void*>(module_ptr + kPitFloorIncrementEvtOffset),
            FloorIncrementEvtHook, sizeof(FloorIncrementEvtHook));
        
        g_PitModulePtr = module_ptr;
    } else {
        g_AdditionalModulePtr = module_ptr;
        if (module_id == ModuleId::TIK) {
            // Make tik_06 (Pit room)'s right exit loop back to itself.
            BeroEntry* tik_06_e_bero = reinterpret_cast<BeroEntry*>(
                module_ptr + kTik06RightBeroEntryOffset);
            tik_06_e_bero->target_map = "tik_06";
            tik_06_e_bero->target_bero = tik_06_e_bero->name;
        }
    }
}

const char* GetReplacementMessage(const char* msg_key) {
    // Do not use for more than one custom message at a time!
    static char buf[128];
    
    if (!strcmp(msg_key, "pit_reward_party_join")) {
        return "<system>\n<p>\nYou got a new party member!\n<k>";
    } else if (!strcmp(msg_key, "pit_disabled_return")) {
        return "<system>\n<p>\nYou can't leave the Infinite Pit!\n<k>";
    } else if (!strcmp(msg_key, "msg_jon_kanban_1")) {
        sprintf(buf, "<kanban>\n<pos 150 25>\nFloor %" PRId32 "\n<k>", 
                g_PitFloor + 1);
        return buf;
    }
    return nullptr;
}

// TODO: Port constants to other game versions.
void ApplyMiscPatches() {
    // Apply patches to item menu code to display the correct available partners
    // (both functions use identical code).
    const int32_t kWinItemDispPartyTableBeginHookAddress = 0x80169f40;
    const int32_t kWinItemDispPartyTableEndHookAddress = 0x8016a088;
    const int32_t kWinItemSelectPartyTableBeginHookAddress = 0x8016ce88;
    const int32_t kWinItemSelectPartyTableEndHookAddress = 0x8016cfd0;
    mod::patch::writeBranch(
        reinterpret_cast<void*>(kWinItemDispPartyTableBeginHookAddress),
        reinterpret_cast<void*>(StartFixItemWinPartyDispOrder));
    mod::patch::writeBranch(
        reinterpret_cast<void*>(BranchBackFixItemWinPartyDispOrder),
        reinterpret_cast<void*>(kWinItemDispPartyTableEndHookAddress));
    mod::patch::writeBranch(
        reinterpret_cast<void*>(kWinItemSelectPartyTableBeginHookAddress),
        reinterpret_cast<void*>(StartFixItemWinPartySelectOrder));
    mod::patch::writeBranch(
        reinterpret_cast<void*>(BranchBackFixItemWinPartySelectOrder),
        reinterpret_cast<void*>(kWinItemSelectPartyTableEndHookAddress));

    // Prevents the menu from closing if you use an item on the active party.
    const int32_t kAlwaysUseItemsInMenuOpcode = 0x4800001c;
    mod::patch::writePatch(
        reinterpret_cast<void*>(0x8016ce40),
        &kAlwaysUseItemsInMenuOpcode, sizeof(kAlwaysUseItemsInMenuOpcode));
        
    // Apply patch to item menu code to properly use Shine Sprite items.
    const int32_t kWinItemCheckBackgroundHookAddress = 0x8016cfd0;
    mod::patch::writeBranch(
        reinterpret_cast<void*>(kWinItemCheckBackgroundHookAddress),
        reinterpret_cast<void*>(StartUsePartyRankup));
    mod::patch::writeBranch(
        reinterpret_cast<void*>(BranchBackUsePartyRankup),
        reinterpret_cast<void*>(kWinItemCheckBackgroundHookAddress + 0x4));
        
    // Item patches.
    
    // Turn Gold Bars x3 into "Shine Sprites" that can be used from the menu.
    // TODO: Update descriptions to be more useful?
    memcpy(&itemDataTable[ItemType::GOLD_BAR_X3], 
           &itemDataTable[ItemType::SHINE_SPRITE], sizeof(ItemData));
    itemDataTable[ItemType::GOLD_BAR_X3].usable_locations 
        |= ItemUseLocation::kField;
}

}