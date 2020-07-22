#include "randomizer_patches.h"

#include "common_functions.h"
#include "common_types.h"
#include "evt_cmd.h"
#include "patch.h"
#include "randomizer_data.h"

#include <gc/OSLink.h>
#include <ttyd/battle.h>
#include <ttyd/evt_bero.h>
#include <ttyd/evt_cam.h>
#include <ttyd/evt_eff.h>
#include <ttyd/evt_item.h>
#include <ttyd/evt_mario.h>
#include <ttyd/evt_mobj.h>
#include <ttyd/evt_msg.h>
#include <ttyd/evt_npc.h>
#include <ttyd/evt_party.h>
#include <ttyd/evt_pouch.h>
#include <ttyd/evt_snd.h>
#include <ttyd/evtmgr.h>
#include <ttyd/evtmgr_cmd.h>
#include <ttyd/filemgr.h>
#include <ttyd/item_data.h>
#include <ttyd/mario.h>
#include <ttyd/mario_party.h>
#include <ttyd/mario_pouch.h>
#include <ttyd/mariost.h>
#include <ttyd/memory.h>
#include <ttyd/npcdrv.h>
#include <ttyd/seq_mapchange.h>
#include <ttyd/seqdrv.h>
#include <ttyd/system.h>
#include <ttyd/win_main.h>
#include <ttyd/win_party.h>

#include <cinttypes>
#include <cstdio>
#include <cstring>

// Assembly patch functions, and code referenced in them.
extern "C" {
    // battle_condition_patches.s
    void StartRuleDisp();
    void BranchBackRuleDisp();
    // map_change_patches.s
    void StartMapLoad();
    void BranchBackMapLoad();
    void StartOnMapUnload();
    void BranchBackOnMapUnload();
    // win_item_patches.s
    void StartFixItemWinPartyDispOrder();
    void BranchBackFixItemWinPartyDispOrder();
    void StartFixItemWinPartySelectOrder();
    void BranchBackFixItemWinPartySelectOrder();
    void StartUsePartyRankup();
    void BranchBackUsePartyRankup();
    
    void getBattleConditionString(char* out_buf) {
        mod::pit_randomizer::GetBattleConditionString(out_buf);
    }
    
    int32_t mapLoad() { return mod::pit_randomizer::LoadMap(); }
    void onMapUnload() { mod::pit_randomizer::OnMapUnloaded(); }
    
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
using ::ttyd::system::getMarioStDvdRoot;

namespace ItemType = ::ttyd::item_data::ItemType;
namespace ItemUseLocation = ::ttyd::item_data::ItemUseLocation_Flags;

// Global variables and constants.
alignas(0x10) char g_AdditionalRelBss[0x3d4];
const char* g_AdditionalModuleToLoad = nullptr;
uintptr_t g_PitModulePtr = 0;
// TODO: Move all references of this to g_Randomizer->state_.floor_.
int32_t g_PitFloor       = -1;

const char* kPitNpcName = "\x93\x47";  // "enemy"
const char* kPiderName = "\x83\x70\x83\x43\x83\x5f\x81\x5b\x83\x58";
const char* kArantulaName = 
    "\x83\x60\x83\x85\x83\x89\x83\x93\x83\x5e\x83\x89\x81\x5b";
const char* kChainChompName = "\x83\x8f\x83\x93\x83\x8f\x83\x93";
const char* kRedChompName = 
    "\x83\x6f\x81\x5b\x83\x58\x83\x67\x83\x8f\x83\x93\x83\x8f\x83\x93";
const char* kBonetailName = "\x83\x5d\x83\x93\x83\x6f\x83\x6f";

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

// Event that sets up a Pit enemy NPC, and opens a pipe when it is defeated.
// TODO: Disable on reward floors.
EVT_BEGIN(EnemyNpcSetupEvt)
SET(LW(0), GSW(1321))
ADD(LW(0), 1)
IF_NOT_EQUAL(LW(0), 100)
    MOD(LW(0), 10)
    IF_EQUAL(LW(0), 0)
        RETURN()
    END_IF()
END_IF()
USER_FUNC(GetEnemyNpcInfo, LW(0), LW(1), LW(2), LW(3), LW(4), LW(5), LW(6))
USER_FUNC(ttyd::evt_npc::evt_npc_entry, PTR(kPitNpcName), LW(0))
USER_FUNC(ttyd::evt_npc::evt_npc_set_tribe, PTR(kPitNpcName), LW(1))
USER_FUNC(ttyd::evt_npc::evt_npc_setup, LW(2))
USER_FUNC(ttyd::evt_npc::evt_npc_set_position, 
    PTR(kPitNpcName), LW(4), LW(5), LW(6))
IF_STR_EQUAL(LW(1), PTR(kPiderName))
    USER_FUNC(ttyd::evt_npc::evt_npc_set_home_position,
        PTR(kPitNpcName), LW(4), LW(5), LW(6))
END_IF()
IF_STR_EQUAL(LW(1), PTR(kArantulaName))
    USER_FUNC(ttyd::evt_npc::evt_npc_set_home_position,
        PTR(kPitNpcName), LW(4), LW(5), LW(6))
END_IF()
IF_STR_EQUAL(LW(1), PTR(kChainChompName))
    UNCHECKED_USER_FUNC(
        REL_PTR(ModuleId::JON, kPitChainChompSetHomePosFuncOffset),
        LW(4), LW(5), LW(6))
END_IF()
IF_STR_EQUAL(LW(1), PTR(kRedChompName))
    UNCHECKED_USER_FUNC(
        REL_PTR(ModuleId::JON, kPitChainChompSetHomePosFuncOffset),
        LW(4), LW(5), LW(6))
END_IF()
IF_STR_EQUAL(LW(1), PTR(kBonetailName))
    USER_FUNC(ttyd::evt_npc::evt_npc_set_position, PTR(kPitNpcName), 0, 0, 0)
    USER_FUNC(ttyd::evt_npc::evt_npc_set_anim, PTR(kPitNpcName), PTR("GNB_H_3"))
    USER_FUNC(ttyd::evt_npc::evt_npc_flag_onoff, 1, PTR(kPitNpcName), 0x2000040)
    USER_FUNC(ttyd::evt_npc::evt_npc_pera_onoff, PTR(kPitNpcName), 0)
    USER_FUNC(ttyd::evt_npc::evt_npc_set_ry, PTR(kPitNpcName), 0)
    RUN_EVT(REL_PTR(ModuleId::JON, kPitBonetailFirstEvtOffset))
END_IF()
USER_FUNC(ttyd::evt_npc::evt_npc_set_battle_info, PTR(kPitNpcName), LW(3))
UNCHECKED_USER_FUNC(
    REL_PTR(ModuleId::JON, kPitSetupNpcExtraParametersFuncOffset),
    PTR(kPitNpcName))
UNCHECKED_USER_FUNC(REL_PTR(ModuleId::JON, kPitSetKillFlagFuncOffset))
INLINE_EVT()
LBL(1)
    WAIT_FRM(1)
    USER_FUNC(ttyd::evt_npc::evt_npc_status_check,
        PTR(kPitNpcName), 4, LW(0))
    IF_EQUAL(LW(0), 0)
        GOTO(1)
    END_IF()
    SET(LW(0), GSW(1321))
    ADD(LW(0), 1)
    IF_NOT_EQUAL(LW(0), 100)
        RUN_EVT(REL_PTR(ModuleId::JON, kPitOpenPipeEvtOffset))
    ELSE()
        SET(GSWF(0x13dd), 1)
    END_IF()
END_INLINE()
RETURN()
EVT_END()

// Wrapper for modified enemy-setup event.
EVT_BEGIN(EnemyNpcSetupEvtHook)
RUN_CHILD_EVT(EnemyNpcSetupEvt)
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
           
        // Update the enemy setup event.
        LinkCustomEvt(
            ModuleId::JON, reinterpret_cast<void*>(module_ptr),
            const_cast<int32_t*>(EnemyNpcSetupEvt));
        mod::patch::writePatch(
            reinterpret_cast<void*>(module_ptr + kPitEnemySetupEvtOffset),
            EnemyNpcSetupEvtHook, sizeof(EnemyNpcSetupEvtHook));
        
        g_PitModulePtr = module_ptr;
    } else {
        if (module_id == ModuleId::TIK) {
            // Make tik_06 (Pit room)'s right exit loop back to itself.
            BeroEntry* tik_06_e_bero = reinterpret_cast<BeroEntry*>(
                module_ptr + kTik06RightBeroEntryOffset);
            tik_06_e_bero->target_map = "tik_06";
            tik_06_e_bero->target_bero = tik_06_e_bero->name;
        }
    }
}

int32_t LoadMap() {
    auto* mario_st = ttyd::mariost::g_MarioSt;
    if (g_AdditionalModuleToLoad) {
        const char* area = g_AdditionalModuleToLoad;
        if (ttyd::filemgr::fileAsyncf(
            nullptr, nullptr, "%s/rel/%s.rel", getMarioStDvdRoot(), area)) {
            auto* file = ttyd::filemgr::fileAllocf(
                nullptr, "%s/rel/%s.rel", getMarioStDvdRoot(), area);
            if (file) {
                memcpy(
                    mario_st->pMapAlloc, *file->mpFileData,
                    reinterpret_cast<int32_t>(file->mpFileData[1]));
                ttyd::filemgr::fileFree(file);
            }
            memset(g_AdditionalRelBss, 0, 0x3c4);
                gc::OSLink::OSLink(
                    mario_st->pMapAlloc, g_AdditionalRelBss);

            // Both maps loaded; call the prolog for the Pit and exit.
            reinterpret_cast<void(*)(void)>(mario_st->pRelFileBase->prolog)();
            return 2;
        }
        return 1;
    }
    if (!strcmp(ttyd::seq_mapchange::NextMap, "title")) {
        strcpy(mario_st->unk_14c, mario_st->currentMapName);
        strcpy(mario_st->currentAreaName, "");
        strcpy(mario_st->currentMapName, "");
        ttyd::seqdrv::seqSetSeq(
            ttyd::seqdrv::SeqIndex::kTitle, nullptr, nullptr);
        return 1;
    }
    const char* area = ttyd::seq_mapchange::NextArea;
    if (!strcmp(area, "tou")) {
        if (ttyd::seqdrv::seqGetSeq() == ttyd::seqdrv::SeqIndex::kTitle) {
            area = "tou2";
        } else if (!strcmp(ttyd::seq_mapchange::NextMap, "tou_03")) {
            area = "tou2";
        }
    }
    if (ttyd::filemgr::fileAsyncf(
        nullptr, nullptr, "%s/rel/%s.rel", getMarioStDvdRoot(), area)) {
        auto* file = ttyd::filemgr::fileAllocf(
            nullptr, "%s/rel/%s.rel", getMarioStDvdRoot(), area);
        if (file) {
            if (!strncmp(area, "tst", 3) || !strncmp(area, "jon", 3)) {
                auto* module_info = reinterpret_cast<OSModuleInfo*>(
                    ttyd::memory::_mapAlloc(
                        reinterpret_cast<void*>(0x8041e808),
                        reinterpret_cast<int32_t>(file->mpFileData[1])));
                mario_st->pRelFileBase = module_info;
            } else {
                mario_st->pRelFileBase = mario_st->pMapAlloc;
            }
            memcpy(
                mario_st->pRelFileBase, *file->mpFileData,
                reinterpret_cast<int32_t>(file->mpFileData[1]));
            ttyd::filemgr::fileFree(file);
        }
        if (mario_st->pRelFileBase != nullptr) {
            memset(&ttyd::seq_mapchange::rel_bss, 0, 0x3c4);
            gc::OSLink::OSLink(
                mario_st->pRelFileBase, &ttyd::seq_mapchange::rel_bss);
        }
        ttyd::seq_mapchange::_load(
            mario_st->currentMapName, ttyd::seq_mapchange::NextMap,
            ttyd::seq_mapchange::NextBero);

        // Determine the enemies to spawn on this floor, and load a second
        // relocatable module for support enemies if necessary.
        if (g_PitModulePtr) {
            const char* area = ModuleNameFromId(SelectEnemies(g_PitFloor));
            if (area) {
                g_AdditionalModuleToLoad = area;
                return 1;
            }
        }
        
        // If not the Pit, or no second module needed, call the prolog and exit.
        reinterpret_cast<void(*)(void)>(mario_st->pRelFileBase->prolog)();
        return 2;
    }
    return 1;
}

void OnMapUnloaded() {
    if (g_PitModulePtr) {
        UnlinkCustomEvt(
            ModuleId::JON, reinterpret_cast<void*>(g_PitModulePtr),
            const_cast<int32_t*>(EnemyNpcSetupEvt));
        if (g_AdditionalModuleToLoad) {
            gc::OSLink::OSUnlink(ttyd::mariost::g_MarioSt->pMapAlloc);
            g_AdditionalModuleToLoad = nullptr;
        }
        g_PitModulePtr = 0;
    }
    // Normal unloading logic follows...
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

void CheckBattleCondition() {
    auto* fbat_info =
        *reinterpret_cast<ttyd::npcdrv::FbatBattleInformation**>(
            reinterpret_cast<uintptr_t>(ttyd::battle::g_BattleWork) + 0x2738);
    // If condition is a success and rule is not 0, add a bonus item.
    if (fbat_info->wBtlActRecCondition && fbat_info->wRuleKeepResult == 6) {
        ttyd::npcdrv::NpcBattleInfo* npc_info = fbat_info->wBattleInfo;
        for (int32_t i = 0; i < 8; ++i) {
            if (npc_info->wBackItemIds[i] == 0) {
                // TODO: Determine the bonus item procedurally.
                npc_info->wBackItemIds[i] = ItemType::GOLD_BAR_X3;
                break;
            }
        }
    }
}

void ApplyMiscPatches() {
    // Apply patches to _rule_disp code to print a custom string w/conditions.
    const int32_t kRuleDispBeginHookAddress = 0x8011c384;
    const int32_t kRuleDispEndHookAddress = 0x8011c428;
    mod::patch::writeBranch(
        reinterpret_cast<void*>(kRuleDispBeginHookAddress),
        reinterpret_cast<void*>(StartRuleDisp));
    mod::patch::writeBranch(
        reinterpret_cast<void*>(BranchBackRuleDisp),
        reinterpret_cast<void*>(kRuleDispEndHookAddress));
    // Individual instruction patches to make the string display longer and
    // only be dismissable by the B button.
    const int32_t kLengthenRuleDispTimeOpAddr = 0x8011c5ec;
    const uint32_t kLengthenRuleDispTimeOpcode = 0x3800012c;  // li r0, 300
    mod::patch::writePatch(
        reinterpret_cast<void*>(kLengthenRuleDispTimeOpAddr),
        &kLengthenRuleDispTimeOpcode, sizeof(uint32_t));
    const int32_t kDismissRuleDispButtonOpAddr = 0x8011c62c;
    const uint32_t kDismissRuleDispButtonOpcode = 0x38600200;  // li r3, 200
    mod::patch::writePatch(
        reinterpret_cast<void*>(kDismissRuleDispButtonOpAddr),
        &kDismissRuleDispButtonOpcode, sizeof(uint32_t));

    // Apply patches to seq_mapChangeMain code to run additional logic when
    // loading or unloading a map.
    const int32_t kMapLoadBeginHookAddress = 0x80007ef0;
    const int32_t kMapLoadEndHookAddress = 0x80008148;
    const int32_t kMapUnloadBeginHookAddress = 0x80007e0c;
    const int32_t kMapUnloadEndHookAddress = 0x80007e10;
    mod::patch::writeBranch(
        reinterpret_cast<void*>(kMapLoadBeginHookAddress),
        reinterpret_cast<void*>(StartMapLoad));
    mod::patch::writeBranch(
        reinterpret_cast<void*>(BranchBackMapLoad),
        reinterpret_cast<void*>(kMapLoadEndHookAddress));
    mod::patch::writeBranch(
        reinterpret_cast<void*>(kMapUnloadBeginHookAddress),
        reinterpret_cast<void*>(StartOnMapUnload));
    mod::patch::writeBranch(
        reinterpret_cast<void*>(BranchBackOnMapUnload),
        reinterpret_cast<void*>(kMapUnloadEndHookAddress));
    
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
        
    // Apply patch to item menu code to properly use Shine Sprite items.
    const int32_t kWinItemCheckBackgroundHookAddress = 0x8016cfd0;
    mod::patch::writeBranch(
        reinterpret_cast<void*>(kWinItemCheckBackgroundHookAddress),
        reinterpret_cast<void*>(StartUsePartyRankup));
    mod::patch::writeBranch(
        reinterpret_cast<void*>(BranchBackUsePartyRankup),
        reinterpret_cast<void*>(kWinItemCheckBackgroundHookAddress + 0x4));

    // Prevents the menu from closing if you use an item on the active party.
    const int32_t kItemWindowCloseHookAddr = 0x8016ce40;
    const uint32_t kAlwaysUseItemsInMenuOpcode = 0x4800001c;
    mod::patch::writePatch(
        reinterpret_cast<void*>(kItemWindowCloseHookAddr),
        &kAlwaysUseItemsInMenuOpcode, sizeof(kAlwaysUseItemsInMenuOpcode));
        
    // Enable the crash handler.
    const int32_t kCrashHandlerEnableOpAddr = 0x80009b2c;
    const uint32_t kEnableHandlerOpcode = 0x3800FFFF;  // li r0, -1
    mod::patch::writePatch(
        reinterpret_cast<void*>(kCrashHandlerEnableOpAddr),
        &kEnableHandlerOpcode, sizeof(uint32_t));
        
    // Skip tutorials for boots / hammer upgrades.
    const int32_t kSkipUpgradeCutsceneOpAddr = 0x800abcd8;
    const uint32_t kSkipCutsceneOpcode = 0x48000030;
    mod::patch::writePatch(
        reinterpret_cast<void*>(kSkipUpgradeCutsceneOpAddr),
        &kSkipCutsceneOpcode, sizeof(uint32_t));
        
    // Item patches.
    
    // Turn Gold Bars x3 into "Shine Sprites" that can be used from the menu.
    // TODO: Update descriptions to be more useful?
    memcpy(&itemDataTable[ItemType::GOLD_BAR_X3], 
           &itemDataTable[ItemType::SHINE_SPRITE], sizeof(ItemData));
    itemDataTable[ItemType::GOLD_BAR_X3].usable_locations 
        |= ItemUseLocation::kField;
}

EVT_DEFINE_USER_FUNC(GetEnemyNpcInfo) {
    ttyd::npcdrv::NpcTribeDescription* npc_tribe_description;
    ttyd::npcdrv::NpcSetupInfo* npc_setup_info;
    BuildBattle(
        g_PitModulePtr, g_PitFloor, &npc_tribe_description, &npc_setup_info);
    int8_t* enemy_100 = 
        reinterpret_cast<int8_t*>(g_PitModulePtr + kPitEnemy100Offset);
    int8_t battle_setup_idx = enemy_100[g_PitFloor % 100];
    const int32_t x_sign = ttyd::system::irand(2) ? 1 : -1;
    const int32_t x_pos = ttyd::system::irand(50) + 80;
    const int32_t y_pos = 0;    // TODO: Pick Y-coordinate based on species.
    const int32_t z_pos = ttyd::system::irand(200) - 100;
    
    ttyd::evtmgr_cmd::evtSetValue(
        evt, evt->evtArguments[0], PTR(npc_tribe_description->modelName));
    ttyd::evtmgr_cmd::evtSetValue(
        evt, evt->evtArguments[1], PTR(npc_tribe_description->nameJp));
    ttyd::evtmgr_cmd::evtSetValue(
        evt, evt->evtArguments[2], PTR(npc_setup_info));
    ttyd::evtmgr_cmd::evtSetValue(evt, evt->evtArguments[3], battle_setup_idx);
    ttyd::evtmgr_cmd::evtSetValue(evt, evt->evtArguments[4], x_pos * x_sign);
    ttyd::evtmgr_cmd::evtSetValue(evt, evt->evtArguments[5], y_pos);
    ttyd::evtmgr_cmd::evtSetValue(evt, evt->evtArguments[6], z_pos);
        
    return 2;
}

}