#include "randomizer_patches.h"

#include "common_functions.h"
#include "common_types.h"
#include "common_ui.h"
#include "evt_cmd.h"
#include "patch.h"
#include "randomizer.h"
#include "randomizer_data.h"

#include <gc/OSLink.h>
#include <gc/mtx.h>
#include <gc/types.h>
#include <ttyd/battle.h>
#include <ttyd/battle_damage.h>
#include <ttyd/battle_database_common.h>
#include <ttyd/battle_enemy_item.h>
#include <ttyd/battle_event_cmd.h>
#include <ttyd/battle_item_data.h>
#include <ttyd/battle_mario.h>
#include <ttyd/battle_menu_disp.h>
#include <ttyd/battle_sub.h>
#include <ttyd/battle_unit.h>
#include <ttyd/battle_weapon_power.h>
#include <ttyd/evt_badgeshop.h>
#include <ttyd/evt_bero.h>
#include <ttyd/evt_cam.h>
#include <ttyd/evt_eff.h>
#include <ttyd/evt_item.h>
#include <ttyd/evt_johoya.h>
#include <ttyd/evt_mario.h>
#include <ttyd/evt_mobj.h>
#include <ttyd/evt_msg.h>
#include <ttyd/evt_npc.h>
#include <ttyd/evt_party.h>
#include <ttyd/evt_pouch.h>
#include <ttyd/evt_snd.h>
#include <ttyd/evt_yuugijou.h>
#include <ttyd/evtmgr.h>
#include <ttyd/evtmgr_cmd.h>
#include <ttyd/filemgr.h>
#include <ttyd/icondrv.h>
#include <ttyd/item_data.h>
#include <ttyd/mario.h>
#include <ttyd/mario_party.h>
#include <ttyd/mario_pouch.h>
#include <ttyd/mariost.h>
#include <ttyd/memory.h>
#include <ttyd/npcdrv.h>
#include <ttyd/seq_mapchange.h>
#include <ttyd/seqdrv.h>
#include <ttyd/sound.h>
#include <ttyd/statuswindow.h>
#include <ttyd/swdrv.h>
#include <ttyd/system.h>
#include <ttyd/unit_bomzou.h>
#include <ttyd/unit_party_christine.h>
#include <ttyd/unit_party_chuchurina.h>
#include <ttyd/unit_party_nokotarou.h>
#include <ttyd/unit_party_sanders.h>
#include <ttyd/unit_party_vivian.h>
#include <ttyd/unit_party_yoshi.h>
#include <ttyd/win_main.h>
#include <ttyd/win_party.h>

#include <cinttypes>
#include <cstdio>
#include <cstring>

// Assembly patch functions, and code referenced in them.
extern "C" {
    // charlieton_patches.s (patched directly, not branched to)
    void CharlietonPitPriceListPatchStart();
    void CharlietonPitPriceListPatchEnd();
    void CharlietonPitPriceItemPatchStart();
    void CharlietonPitPriceItemPatchEnd();
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
    
    int32_t mapLoad() { return mod::pit_randomizer::LoadMap(); }
    void onMapUnload() { mod::pit_randomizer::OnMapUnloaded(); } 
    void usePartyRankup() { mod::pit_randomizer::UseShineSprite(); }
}

namespace mod::pit_randomizer {

namespace {

using ::gc::OSLink::OSModuleInfo;
using ::ttyd::battle::BattleWorkCommandCursor;
using ::ttyd::battle::BattleWorkCommandOperation;
using ::ttyd::battle::BattleWorkCommandWeapon;
using ::ttyd::battle_database_common::BattleGroupSetup;
using ::ttyd::battle_database_common::BattleWeapon;
using ::ttyd::battle_database_common::PointDropData;
using ::ttyd::battle_unit::BattleWorkUnit;
using ::ttyd::evt_bero::BeroEntry;
using ::ttyd::evtmgr::EvtEntry;
using ::ttyd::evtmgr_cmd::evtGetValue;
using ::ttyd::evtmgr_cmd::evtSetValue;
using ::ttyd::item_data::itemDataTable;
using ::ttyd::item_data::ItemData;
using ::ttyd::mario_pouch::PouchData;
using ::ttyd::mariost::g_MarioSt;
using ::ttyd::npcdrv::FbatBattleInformation;
using ::ttyd::npcdrv::NpcBattleInfo;
using ::ttyd::npcdrv::NpcEntry;
using ::ttyd::system::getMarioStDvdRoot;
using ::ttyd::win_party::WinPartyData;

namespace BattleUnitType = ::ttyd::battle_database_common::BattleUnitType;
namespace ItemType = ::ttyd::item_data::ItemType;
namespace ItemUseLocation = ::ttyd::item_data::ItemUseLocation_Flags;

// Trampoline hooks for patching in custom logic to existing TTYD C functions.
int32_t (*g_pouchEquipCheckBadge_trampoline)(int16_t) = nullptr;
int32_t (*g_BtlUnit_GetWeaponCost_trampoline)(
    BattleWorkUnit*, BattleWeapon*) = nullptr;
void (*g_DrawOperationWin_trampoline)() = nullptr;
void (*g_DrawWeaponWin_trampoline)() = nullptr;
void (*g__getSickStatusParam_trampoline)(
    BattleWorkUnit*, BattleWeapon*, int32_t, int8_t*, int8_t*) = nullptr;
int32_t(*g__make_madowase_weapon_trampoline)(EvtEntry*, bool) = nullptr;

// Global variables and constants.
alignas(0x10) char  g_AdditionalRelBss[0x3d4];
const char*         g_AdditionalModuleToLoad = nullptr;
uintptr_t           g_PitModulePtr = 0;
bool                g_PromptSave = false;
bool                g_InBattle = false;
int8_t              g_MaxMoveBadgeCounts[18];
int8_t              g_CurMoveBadgeCounts[18];
char                g_MoveBadgeTextBuffers[18][24];
const char*         kMoveBadgeAbbreviations[18] = {
    "Power J.", "Multib.", "Power B.", "Tor. J.", "Shrink S.",
    "Sleep S.", "Soft S.", "Power S.", "Quake H.", "H. Throw",
    "Pier. B.", "H. Rattle", "Fire Drive", "Ice Smash",
    "Charge", "Charge", "Tough. Up", "Tough. Up"
};

const char kPitNpcName[] = "\x93\x47";  // "enemy"
const char kPiderName[] = "\x83\x70\x83\x43\x83\x5f\x81\x5b\x83\x58";
const char kArantulaName[] = 
    "\x83\x60\x83\x85\x83\x89\x83\x93\x83\x5e\x83\x89\x81\x5b";
const char kChainChompName[] = "\x83\x8f\x83\x93\x83\x8f\x83\x93";
const char kRedChompName[] = 
    "\x83\x6f\x81\x5b\x83\x58\x83\x67\x83\x8f\x83\x93\x83\x8f\x83\x93";
const char kBonetailName[] = "\x83\x5d\x83\x93\x83\x6f\x83\x6f";

// Event that plays "get partner" fanfare.
EVT_BEGIN(PartnerFanfareEvt)
USER_FUNC(ttyd::evt_snd::evt_snd_bgmoff, 0x400)
USER_FUNC(ttyd::evt_snd::evt_snd_bgmon, 1, PTR("BGM_FF_GET_PARTY1"))
WAIT_MSEC(2000)
RETURN()
EVT_END()

// Event that handles a chest being opened.
// Rewards the player with an item/partner, or two rewards on boss floors.
EVT_BEGIN(ChestOpenEvt)
USER_FUNC(ttyd::evt_mario::evt_mario_key_onoff, 0)
USER_FUNC(GetNumChestRewards, LW(13))
DO(0)
    SUB(LW(13), 1)
    IF_SMALL(LW(13), 0)
        DO_BREAK()
    END_IF()
    USER_FUNC(GetChestReward, LW(1))
    // If reward < 0, then reward a partner (-1 to -7 = partners 1 to 7).
    IF_SMALL(LW(1), 0)
        MUL(LW(1), -1)
        WAIT_MSEC(100)  // If the second reward
        USER_FUNC(ttyd::evt_mobj::evt_mobj_wait_animation_end, PTR("box"))
        USER_FUNC(ttyd::evt_mario::evt_mario_normalize)
        USER_FUNC(ttyd::evt_mario::evt_mario_goodbye_party, 0)
        WAIT_MSEC(500)
        USER_FUNC(ttyd::evt_pouch::evt_pouch_party_join, LW(1))
        // TODO: Reposition partner by box? At origin is fine...
        USER_FUNC(ttyd::evt_mario::evt_mario_set_party_pos, 0, LW(1), 0, 0, 0)
        RUN_EVT_ID(PartnerFanfareEvt, LW(11))
        USER_FUNC(
            ttyd::evt_eff::evt_eff,
            PTR("sub_bg"), PTR("itemget"), 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
        USER_FUNC(ttyd::evt_msg::evt_msg_toge, 1, 0, 0, 0)
        USER_FUNC(
            ttyd::evt_msg::evt_msg_print, 0, PTR("pit_reward_party_join"), 0, 0)
        CHK_EVT(LW(11), LW(12))
        IF_EQUAL(LW(12), 1)
            DELETE_EVT(LW(11))
            USER_FUNC(ttyd::evt_snd::evt_snd_bgmoff, 0x201)
        END_IF()
        USER_FUNC(ttyd::evt_eff::evt_eff_softdelete, PTR("sub_bg"))
        USER_FUNC(ttyd::evt_snd::evt_snd_bgmon, 0x120, 0)
        USER_FUNC(
            ttyd::evt_snd::evt_snd_bgmon_f, 0x300, PTR("BGM_STG0_100DN1"), 1500)
        WAIT_MSEC(500)
        USER_FUNC(ttyd::evt_party::evt_party_run, 0)
        USER_FUNC(ttyd::evt_party::evt_party_run, 1)
    ELSE()
        // Reward is an item; spawn it item normally.
        USER_FUNC(
            ttyd::evt_mobj::evt_mobj_get_position,
            PTR("box"), LW(10), LW(11), LW(12))
        USER_FUNC(
            ttyd::evt_item::evt_item_entry,
            PTR("item"), LW(1), LW(10), LW(11), LW(12), 17, -1, 0)
        WAIT_MSEC(300)  // If the second reward
        USER_FUNC(ttyd::evt_mobj::evt_mobj_wait_animation_end, PTR("box"))
        USER_FUNC(ttyd::evt_item::evt_item_get_item, PTR("item"))
        // If the item was a Crystal Star / Magical Map, unlock its Star Power.
        // TODO: The pause menu does not properly display SP moves out of order.
        USER_FUNC(AddItemStarPower, LW(1))
    END_IF()
WHILE()
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

// Event that runs before advancing to the next floor.
// On reward floors, checks to see if the player has claimed their reward, then
// prompts the player to save.  If conditions are met to continue, increments
// the floor counter in the randomizer's state, as well as GSW(1321).
EVT_BEGIN(FloorIncrementEvt)
SET(LW(0), GSW(1321))
ADD(LW(0), 1)
MOD(LW(0), 10)
IF_EQUAL(LW(0), 0)
    USER_FUNC(CheckRewardClaimed, LW(0))
    IF_EQUAL(LW(0), 0)
        // If reward not claimed, disable player from continuing.
        SET(LW(0), 1)
        USER_FUNC(ttyd::evt_mario::evt_mario_key_onoff, 0)
        WAIT_MSEC(50)
        USER_FUNC(
            ttyd::evt_msg::evt_msg_print, 0, PTR("pit_chest_unclaimed"), 0, 0)
        WAIT_MSEC(50)
        USER_FUNC(ttyd::evt_mario::evt_mario_key_onoff, 1)
    ELSE()
        USER_FUNC(CheckPromptSave, LW(0))
        IF_EQUAL(LW(0), 1)
            // If prompting the user to save, disable player from continuing.
            RUN_CHILD_EVT(ttyd::evt_mobj::mobj_save_blk_sysevt)
            SET(LW(0), 1)
        ELSE()    
            SET(LW(0), 0)
            USER_FUNC(IncrementInfinitePitFloor)
        END_IF()
    END_IF()
ELSE()
    // Set LW(0) back to 0 to allow going through pipe.
    SET(LW(0), 0)
    USER_FUNC(IncrementInfinitePitFloor)
END_IF()
RETURN()
EVT_END()

// Wrapper for modified floor-incrementing event.
EVT_BEGIN(FloorIncrementEvtHook)
RUN_CHILD_EVT(FloorIncrementEvt)
RETURN()
EVT_END()

// Event that sets up a Pit enemy NPC, and opens a pipe when it is defeated.
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
IF_STR_EQUAL(LW(1), PTR(kBonetailName))
    USER_FUNC(ttyd::evt_npc::evt_npc_set_position, PTR(kPitNpcName), 0, 0, 0)
    USER_FUNC(ttyd::evt_npc::evt_npc_set_anim, PTR(kPitNpcName), PTR("GNB_H_3"))
    USER_FUNC(ttyd::evt_npc::evt_npc_flag_onoff, 1, PTR(kPitNpcName), 0x2000040)
    USER_FUNC(ttyd::evt_npc::evt_npc_pera_onoff, PTR(kPitNpcName), 0)
    USER_FUNC(ttyd::evt_npc::evt_npc_set_ry, PTR(kPitNpcName), 0)
    RUN_EVT(REL_PTR(ModuleId::JON, kPitBonetailFirstEvtOffset))
END_IF()
USER_FUNC(SetEnemyNpcBattleInfo, PTR(kPitNpcName), LW(3))
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

// Patch over the end of the existing Trade Off item script so it actually
// calls the part of the code associated with applying its status.
EVT_BEGIN(TradeOffPatch)
SET(LW(12), PTR(&ttyd::battle_item_data::ItemWeaponData_Teki_Kyouka))
USER_FUNC(ttyd::battle_event_cmd::btlevtcmd_WeaponAftereffect, LW(12))
// Run the end of ItemEvent_Support_NoEffect's evt.
RUN_CHILD_EVT(static_cast<int32_t>(0x803652b8U))
RETURN()
EVT_END()

// Patch over the end of subsetevt_blow_dead event to disable getting
// coins and EXP from Gale Force.
EVT_BEGIN(GaleForceKillPatch)
USER_FUNC(ttyd::battle_event_cmd::btlevtcmd_KillUnit, -2, 0)
RETURN()
EVT_END()

// A fragment of an event to patch over Hammer/Boomerang/Fire Bros.' HP checks.
const int32_t HammerBrosHpCheck[] = {
    USER_FUNC(GetPercentOfMaxHP, -2, LW(0))
    IF_SMALL(LW(0), 50)
};

}

void InitializeOnNewFile() {
    // TODO: Make this not re-allocate the pouch if an alloc already exists.
    ttyd::mario_pouch::pouchInit();
    PouchData& pouch = *ttyd::mario_pouch::pouchGetPtr();
    // Initialize other systems / data.
    ttyd::evt_badgeshop::badgeShop_init();
    ttyd::evt_yuugijou::yuugijou_init();
    ttyd::evt_johoya::johoya_init();
    
    ttyd::mario::marioSetCharMode(0);
    ttyd::statuswindow::statusWinForceUpdate();
    ttyd::mariost::g_MarioSt->lastFrameRetraceLocalTime = 0ULL;
    ttyd::mariost::g_MarioSt->flags &= ~1U;
    
    // Initializes the randomizer's state and copies it to the pouch.
    g_Randomizer->state_.Load(/* new_save = */ true);
    g_Randomizer->state_.Save();
    
    // Update any stats / equipment / flags as necessary.
    ttyd::mario_pouch::pouchGetItem(ItemType::BOOTS);
    ttyd::mario_pouch::pouchGetItem(ItemType::HAMMER);
    ttyd::mario_pouch::pouchSetCoin(0);
    ttyd::mario_pouch::pouchGetItem(ItemType::PEEKABOO);
    ttyd::mario_pouch::pouchEquipBadgeID(ItemType::PEEKABOO);
    ttyd::mario_pouch::pouchGetItem(ItemType::L_EMBLEM);
    ttyd::mario_pouch::pouchGetItem(ItemType::W_EMBLEM);
    ttyd::mario_pouch::pouchGetItem(ItemType::FP_PLUS);
    ttyd::mario_pouch::pouchGetItem(ItemType::HP_PLUS);
    pouch.unallocated_bp = 6;
    pouch.total_bp = 6;
    ttyd::mario_pouch::pouchReviseMarioParam();
    // Assign Yoshi a random color.
    ttyd::mario_pouch::pouchSetPartyColor(4, g_Randomizer->state_.Rand(7));
    
    // Set story progress / some tutorial flags.
    ttyd::swdrv::swInit();
    ttyd::swdrv::swByteSet(0, 405);     // post-game story progress
    ttyd::swdrv::swSet(0xe9);           // Save Block tutorial
    ttyd::swdrv::swSet(0xea);           // Heart Block tutorial
    ttyd::swdrv::swSet(0xeb);           // Item tutorial
    ttyd::swdrv::swSet(0xec);           // Save Block tutorial-related
    ttyd::swdrv::swSet(0x15d9);         // Star piece in Pit room collected
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
        return_bero->out_evt_code = reinterpret_cast<void*>(
            const_cast<int32_t*>(DisabledBeroEvt));
        
        // Change destination of boss room pipe to be the usual 80+ floor.
        BeroEntry* boss_bero = reinterpret_cast<BeroEntry*>(
            module_ptr + kPitBossFloorReturnBeroEntryOffset);
        boss_bero->target_map = "jon_02";
        boss_bero->target_bero = "dokan_2";
        boss_bero->out_evt_code = reinterpret_cast<void*>(
            module_ptr + kPitFloorIncrementEvtOffset);
        
        // Update the actual Pit floor alongside GSW(1321); also, check for
        // unclaimed rewards and prompt the player to save on X0 floors.
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
            
        // Make Charlieton always spawn, and Movers never spawn.
        *reinterpret_cast<int32_t*>(
            module_ptr + kPitCharlietonSpawnChanceOffset) = 1000;
        *reinterpret_cast<int32_t*>(
            module_ptr + kPitMoverLastSpawnFloorOffset) = 0;
        
        // If not reward floor, reset Pit-related flags, and reset save prompt.
        if (g_Randomizer->state_.floor_ % 10 != 9) {
            for (uint32_t i = 0x13d3; i <= 0x13dd; ++i) {
                ttyd::swdrv::swClear(i);
            }
            for (int32_t i = 0; i < 6; ++i) {
                g_Randomizer->state_.charlieton_items_[i] = 0;
            }
            g_PromptSave = true;
        }
        // Otherwise, modify Charlieton's stock, using items from the randomizer
        // state if continuing from an existing save file.
        else {
            ReplaceCharlietonStock();
        }
        
        g_PitModulePtr = module_ptr;
    } else {
        if (module_id == ModuleId::TIK) {
            // Make tik_06 (pre-Pit room)'s right exit loop back to itself.
            BeroEntry* tik_06_e_bero = reinterpret_cast<BeroEntry*>(
                module_ptr + kTik06RightBeroEntryOffset);
            tik_06_e_bero->target_map = "tik_06";
            tik_06_e_bero->target_bero = tik_06_e_bero->name;
            
            // Patch over Hammer Bros. HP check.
            mod::patch::writePatch(
                reinterpret_cast<void*>(module_ptr + 0x323a4),
                HammerBrosHpCheck, sizeof(HammerBrosHpCheck));
        } else if (module_id == ModuleId::TOU2) {
            // Patch over Hammer, Boomerang, and Fire Bros.' HP checks.
            mod::patch::writePatch(
                reinterpret_cast<void*>(module_ptr + 0x373ec),
                HammerBrosHpCheck, sizeof(HammerBrosHpCheck));
            mod::patch::writePatch(
                reinterpret_cast<void*>(module_ptr + 0x2b644),
                HammerBrosHpCheck, sizeof(HammerBrosHpCheck));
            mod::patch::writePatch(
                reinterpret_cast<void*>(module_ptr + 0x31aa4),
                HammerBrosHpCheck, sizeof(HammerBrosHpCheck));
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
            const char* area = 
                ModuleNameFromId(SelectEnemies(g_Randomizer->state_.floor_));
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

void OnEnterExitBattle(bool is_start) {
    if (is_start) {
        int8_t badge_count;
        for (int32_t i = 0; i < 14; ++i) {
            badge_count = ttyd::mario_pouch::pouchEquipCheckBadge(
                ItemType::POWER_JUMP + i);
            g_MaxMoveBadgeCounts[i] = badge_count;
            g_CurMoveBadgeCounts[i] = badge_count;
        }
        for (int32_t i = 0; i < 2; ++i) {
            badge_count = ttyd::mario_pouch::pouchEquipCheckBadge(
                ItemType::CHARGE + i);
            g_MaxMoveBadgeCounts[14 + i] = badge_count;
            g_CurMoveBadgeCounts[14 + i] = badge_count;
            badge_count = ttyd::mario_pouch::pouchEquipCheckBadge(
                ItemType::SUPER_CHARGE + i);
            g_MaxMoveBadgeCounts[16 + i] = badge_count;
            g_CurMoveBadgeCounts[16 + i] = badge_count;
        }
        g_InBattle = true;
    } else {
        g_InBattle = false;
    }
}

int32_t GetWeaponLevelSelectionIndex(int16_t badge_id) {
    if (badge_id >= ItemType::POWER_JUMP && badge_id <= ItemType::ICE_SMASH) {
        return badge_id - ItemType::POWER_JUMP;
    }
    switch (badge_id) {
        case ItemType::CHARGE: return 14;
        case ItemType::CHARGE_P: return 15;
        case ItemType::SUPER_CHARGE: return 16;
        case ItemType::SUPER_CHARGE_P: return 17;
    }
    return -1;
}

int32_t GetSelectedLevelWeaponCost(BattleWorkUnit* unit, BattleWeapon* weapon) {
    if (!weapon || !g_InBattle) return -1;
    if (int32_t idx = GetWeaponLevelSelectionIndex(weapon->item_id); idx >= 0) {
        const int32_t fp_cost =
            weapon->base_fp_cost * g_CurMoveBadgeCounts[idx] -
            unit->badges_equipped.flower_saver;
        return fp_cost < 1 ? 1 : fp_cost;
    }
    return -1;
}

void CheckForSelectingWeaponLevel(bool is_strategies_menu) {
    const uint16_t buttons = ttyd::system::keyGetButtonTrg(0);
    const bool left_press = buttons & (ButtonId::L | ButtonId::DPAD_LEFT);
    const bool right_press = buttons & (ButtonId::R | ButtonId::DPAD_RIGHT);
    
    void** win_data = reinterpret_cast<void**>(
        ttyd::battle::g_BattleWork->command_work.window_work);
    if (!win_data || !win_data[0]) return;
    
    auto* cursor = reinterpret_cast<BattleWorkCommandCursor*>(win_data[0]);
    if (is_strategies_menu) {
        auto* battleWork = ttyd::battle::g_BattleWork;
        auto* strats = battleWork->command_work.operation_table;
        BattleWorkUnit* unit =
            battleWork->battle_units[battleWork->active_unit_idx];
        BattleWeapon* weapon = nullptr;
        int32_t idx = 0;
        for (int32_t i = 0; i < cursor->num_options; ++i) {
            // Not selecting Charge or Super Charge.
            if (strats[i].type < 1 || strats[i].type > 2) continue;
            
            if (strats[i].type == 1) {
                if (unit->current_kind == BattleUnitType::MARIO) {
                    weapon = &ttyd::battle_mario::badgeWeapon_Charge;
                } else {
                    weapon = &ttyd::battle_mario::badgeWeapon_ChargeP;
                }
            } else {
                if (unit->current_kind == BattleUnitType::MARIO) {
                    weapon = &ttyd::battle_mario::badgeWeapon_SuperCharge;
                } else {
                    weapon = &ttyd::battle_mario::badgeWeapon_SuperChargeP;
                }
            }
            idx = GetWeaponLevelSelectionIndex(weapon->item_id);
            
            // If current selection, and L/R pressed, change power level.
            if (i == cursor->abs_position) {
                if (left_press && g_CurMoveBadgeCounts[idx] > 1) {
                    --g_CurMoveBadgeCounts[idx];
                    ttyd::sound::SoundEfxPlayEx(0x478, 0, 0x64, 0x40);
                } else if (
                    right_press && 
                    g_CurMoveBadgeCounts[idx] < g_MaxMoveBadgeCounts[idx]) {
                    ++g_CurMoveBadgeCounts[idx];
                    ttyd::sound::SoundEfxPlayEx(0x478, 0, 0x64, 0x40);
                }
            }
            
            sprintf(
                g_MoveBadgeTextBuffers[idx], "%s Lv. %" PRId8,
                kMoveBadgeAbbreviations[idx], g_CurMoveBadgeCounts[idx]);
            strats[i].name = g_MoveBadgeTextBuffers[idx];
            strats[i].cost = GetSelectedLevelWeaponCost(unit, weapon);
            strats[i].enabled =
                strats[i].cost <= ttyd::battle_unit::BtlUnit_GetFp(unit);
            strats[i].unk_08 = !strats[i].enabled;  // 1 if disabled: "no FP" msg
        }
    } else {
        auto* weapons = reinterpret_cast<BattleWorkCommandWeapon*>(win_data[2]);
        if (!weapons) return;
        for (int32_t i = 0; i < cursor->num_options; ++i) {
            if (!weapons[i].weapon) continue;
            const int32_t idx = GetWeaponLevelSelectionIndex(
                weapons[i].weapon->item_id);
            if (idx < 0 || g_MaxMoveBadgeCounts[idx] <= 1) continue;
            
            // If current selection, and L/R pressed, change power level.
            if (i == cursor->abs_position) {
                if (left_press && g_CurMoveBadgeCounts[idx] > 1) {
                    --g_CurMoveBadgeCounts[idx];
                    ttyd::sound::SoundEfxPlayEx(0x478, 0, 0x64, 0x40);
                } else if (
                    right_press && 
                    g_CurMoveBadgeCounts[idx] < g_MaxMoveBadgeCounts[idx]) {
                    ++g_CurMoveBadgeCounts[idx];
                    ttyd::sound::SoundEfxPlayEx(0x478, 0, 0x64, 0x40);
                }
            }
            
            // Overwrite default text based on current power level.
            sprintf(
                g_MoveBadgeTextBuffers[idx], "%s Lv. %" PRId8,
                kMoveBadgeAbbreviations[idx], g_CurMoveBadgeCounts[idx]);
            weapons[i].name = g_MoveBadgeTextBuffers[idx];
        }
    }
}

void UseShineSprite() {
    void* winPtr = ttyd::win_main::winGetPtr();
    const int32_t item = reinterpret_cast<int32_t*>(winPtr)[0x2d4 / 4];
    
    // If the item is a Shine Sprite... (otherwise, handle normally)
    if (item == ItemType::GOLD_BAR_X3) {
        int32_t* party_member_target = 
            reinterpret_cast<int32_t*>(winPtr) + (0x2dc / 4);
        // If Mario is selected, target the first party member instead.
        if (*party_member_target == 0) *party_member_target = 1;
        
        WinPartyData* party_data = ttyd::win_party::g_winPartyDt;
        ttyd::mario_pouch::PouchPartyData* pouch_data = 
            ttyd::mario_pouch::pouchGetPtr()->party_data;
        const int32_t party_id = ttyd::mario_party::marioGetParty();
        
        // Determine the order the party members are shown in the menu.
        // TODO: Try to reuse the same code currently in win_item_patches.s.
        WinPartyData* party_win_order[7];
        WinPartyData** current_order = party_win_order;
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
            // TODO: Save the number of HP ups to the randomizer_state.
            if (hp_table[2] < 200) hp_table[2] += 5;
        }
        pouch_data->base_max_hp = hp_table[pouch_data->hp_level];
        pouch_data->current_hp = hp_table[pouch_data->hp_level];
        pouch_data->max_hp = hp_table[pouch_data->hp_level];
        // Include HP Plus P in current / max stats.
        const int32_t hp_plus_p_cnt =
            ttyd::mario_pouch::pouchEquipCheckBadge(ItemType::HP_PLUS_P);
        pouch_data->current_hp += 5 * hp_plus_p_cnt;
        pouch_data->max_hp += 5 * hp_plus_p_cnt;
    }
}

void CheckBattleCondition() {
    auto* fbat_info = ttyd::battle::g_BattleWork->fbat_info;
    const int32_t item_reward =
        fbat_info->wBattleInfo->pConfiguration->random_item_weight;
    // Did not win the fight.
    if (fbat_info->wResult != 1) return;
    // If condition is a success and rule is not 0, add a bonus item.
    if (fbat_info->wBtlActRecCondition && fbat_info->wRuleKeepResult == 6) {
        NpcBattleInfo* npc_info = fbat_info->wBattleInfo;
        for (int32_t i = 0; i < 8; ++i) {
            // Only return a bonus item if the enemies were all defeated.
            if (npc_info->wStolenItems[i] != 0) return;
        }
        for (int32_t i = 0; i < 8; ++i) {
            if (npc_info->wBackItemIds[i] == 0) {
                npc_info->wBackItemIds[i] = item_reward;
                break;
            }
        }
    }
}

void DisplayBattleCondition() {
    char buf[128];
    GetBattleConditionString(buf);
    DrawCenteredTextWindow(
        buf, 0, 60, 0xFFu, false, 0x000000FFu, 0.75f, 0xFFFFFFE5u, 15, 10);
}

void GetDropMaterials(FbatBattleInformation* fbat_info) {
    NpcBattleInfo* battle_info = fbat_info->wBattleInfo;
    const BattleGroupSetup* party_setup = battle_info->pConfiguration;
    const PointDropData* hp_drop = party_setup->hp_drop_table;
    const PointDropData* fp_drop = party_setup->fp_drop_table;
    
    // Get natural heart and flower drops based on Mario's health, as usual.
    auto* battleWork = ttyd::battle::g_BattleWork;
    const BattleWorkUnit* mario = ttyd::battle::BattleGetMarioPtr(battleWork);
    
    int32_t mario_hp_pct = mario->current_hp * 100 / mario->max_hp;
    for (; true; ++hp_drop) {
        if (mario_hp_pct <= hp_drop->max_stat_percent) {
            if (static_cast<int32_t>(ttyd::system::irand(100))
                    < hp_drop->overall_drop_rate) {
                for (int32_t i = 0; i < hp_drop->drop_count; ++i) {
                    if (static_cast<int32_t>(ttyd::system::irand(100))
                            < hp_drop->individual_drop_rate) {
                        ++battle_info->wHeartsDroppedBaseCount;
                    }
                }
            }
            break;
        }
    }
    int32_t mario_fp_pct = mario->current_fp * 100 / mario->max_fp;
    for (; true; ++fp_drop) {
        if (mario_fp_pct <= fp_drop->max_stat_percent) {
            if (static_cast<int32_t>(ttyd::system::irand(100))
                    < fp_drop->overall_drop_rate) {
                for (int32_t i = 0; i < fp_drop->drop_count; ++i) {
                    if (static_cast<int32_t>(ttyd::system::irand(100))
                            < fp_drop->individual_drop_rate) {
                        ++battle_info->wFlowersDroppedBaseCount;
                    }
                }
            }
            break;
        }
    }
    
    // Get item drop, based on the pre-set enemy held item index.
    battle_info->wItemDropped = 
        battle_info->wHeldItems[party_setup->held_item_weight];
}

// Global variable for the last type of item consumed;
// this is necessary to allow enemies to use cooked items.
int32_t g_EnemyItem = 0;

void EnemyConsumeItem(ttyd::evtmgr::EvtEntry* evt) {
    auto* battleWork = ttyd::battle::g_BattleWork;
    int32_t id = evtGetValue(evt, evt->evtArguments[0]);
    id = ttyd::battle_sub::BattleTransID(evt, id);
    auto* unit = ttyd::battle::BattleGetUnitPtr(battleWork, id);
    if (unit->current_kind <= BattleUnitType::BONETAIL) {
        g_EnemyItem = unit->held_item;
    }
}

bool GetEnemyConsumeItem(ttyd::evtmgr::EvtEntry* evt) {
    auto* battleWork = ttyd::battle::g_BattleWork;
    BattleWorkUnit* unit = nullptr;
    if (evt->wActorThisPtr) {
        unit = ttyd::battle::BattleGetUnitPtr(
            battleWork,
            reinterpret_cast<uint32_t>(evt->wActorThisPtr));
        if (unit->current_kind <= BattleUnitType::BONETAIL) {
            evtSetValue(evt, evt->evtArguments[0], g_EnemyItem);
            return true;
        }
    }
    return false;
}

void* EnemyUseAdditionalItemsCheck(BattleWorkUnit* unit) {
    switch (unit->held_item) {
        // Items that aren't normally usable but work with no problems:
        case ItemType::COURAGE_MEAL:
        case ItemType::EGG_BOMB:
        case ItemType::COCONUT_BOMB:
        case ItemType::ZESS_DYNAMITE:
        case ItemType::HOT_SAUCE:
        case ItemType::SPITE_POUCH:
        case ItemType::KOOPA_CURSE:
        case ItemType::SHROOM_BROTH:
        case ItemType::LOVE_PUDDING:
        case ItemType::PEACH_TART:
        case ItemType::ELECTRO_POP:
        // Additional items (would not have the desired effect without patches):
        case ItemType::POISON_SHROOM:
        case ItemType::POINT_SWAP:
        case ItemType::TRIAL_STEW:
        case ItemType::TRADE_OFF:
            return ttyd::battle_enemy_item::_check_attack_item(unit);
        case ItemType::FRESH_JUICE:
        case ItemType::HEALTHY_SALAD:
            return ttyd::battle_enemy_item::_check_status_recover_item(unit);
        // Explicitly not allowed:
        case ItemType::FRIGHT_MASK:
        case ItemType::MYSTERY:
        default:
            return nullptr;
    }
}

void DisplayStarPowerNumber() {
    // Don't display SP if Mario hasn't gotten any Star Powers yet.
    if (ttyd::mario_pouch::pouchGetMaxAP() < 100) return;
    
    // Don't try to display SP if the status bar is not on-screen.
    float menu_height = *reinterpret_cast<float*>(
        reinterpret_cast<uintptr_t>(ttyd::statuswindow::g_StatusWindowWork)
        + 0x24);
    if (menu_height < 100.f || menu_height > 330.f) return;
    
    gc::mtx34 matrix;
    int32_t unknown_param = -1;
    int32_t current_AP = ttyd::mario_pouch::pouchGetAP();
    gc::mtx::PSMTXTrans(&matrix, 192.f, menu_height - 100.f, 0.f);
    ttyd::icondrv::iconNumberDispGx(
        &matrix, current_AP, /* is_small = */ 1, &unknown_param);
}

void DisplayStarPowerOrbs(double x, double y, int32_t star_power) {
    int32_t max_star_power = ttyd::mario_pouch::pouchGetMaxAP();
    if (star_power > max_star_power) star_power = max_star_power;
    if (star_power < 0) star_power = 0;
    
    int32_t full_orbs = star_power / 100;
    int32_t remainder = star_power % 100;
    int32_t part_frame = remainder * 15 / 99;
    if (remainder > 0 && star_power > 0 && part_frame == 0) part_frame = 1;
    
    if (part_frame != 0) {
        gc::vec3 pos = { 
            static_cast<float>(x + 32.f * full_orbs),
            static_cast<float>(y),
            0.f };
        ttyd::icondrv::iconDispGx(
            1.f, &pos, 0x10, ttyd::statuswindow::gauge_wakka[part_frame]);
    }
    for (int32_t i = 0; i < max_star_power / 100; ++i) {
        gc::vec3 pos = {
            static_cast<float>(x + 32.f * i), 
            static_cast<float>(y + 12.f),
            0.f };
        uint16_t icon = i < full_orbs ?
            ttyd::statuswindow::gauge_back[i] : 0x1c7;
        ttyd::icondrv::iconDispGx(1.f, &pos, 0x10, icon);
    }
}

void ReplaceCharlietonStock() {
    int16_t* saved_items = g_Randomizer->state_.charlieton_items_;
    // If there are already items from a previous save, use and clear them.
    if (saved_items[0]) {
        for (int32_t i = 0; i < 6; ++i) {
            ttyd::evt_badgeshop::badge_bottakuru100_table[i] = saved_items[i];
            saved_items[i] = 0;
        }
    } else {
        for (int32_t i = 0; i < 6; ++i) {
            bool found = true;
            while (found) {
                found = false;
                // Two normal items, two recipe items, two badges.
                int32_t item = PickRandomItem(
                    /* seeded = */ true, i < 2, i & 2, i >= 4, 0);
                // Make sure no duplicate items exist.
                for (int32_t j = 0; j < i; ++j) {
                    if (ttyd::evt_badgeshop::badge_bottakuru100_table[j]
                            == item) {
                        found = true;
                    }
                }
                ttyd::evt_badgeshop::badge_bottakuru100_table[i] = item;
                saved_items[i] = item;
            }
        }
    }
}

const char* GetReplacementMessage(const char* msg_key) {
    // Do not use for more than one custom message at a time!
    static char buf[128];
    
    // TODO: Consider replacing these compares with a single hash and
    // comparison if many more strings are added.
    if (!strcmp(msg_key, "pit_reward_party_join")) {
        return "<system>\n<p>\nYou got a new party member!\n<k>";
    } else if (!strcmp(msg_key, "pit_disabled_return")) {
        return "<system>\n<p>\nYou can't leave the Infinite Pit!\n<k>";
    } else if (!strcmp(msg_key, "pit_chest_unclaimed")) {
        return "<system>\n<p>\nYou haven't claimed your\nreward!\n<k>";
    } else if (!strcmp(msg_key, "msg_jon_kanban_1")) {
        sprintf(buf, "<kanban>\n<pos 150 25>\nFloor %" PRId32 "\n<k>", 
                g_Randomizer->state_.floor_ + 1);
        return buf;
    } else if (!strcmp(msg_key, "msg_jon_kanban_3")) {
        sprintf(buf, "<kanban>\nYour seed: %s\n"
            "(Name your file \"random\" or \"\xde\"\n"
            "to have one picked randomly.)<k>",
            ttyd::mariost::g_MarioSt->saveFileName);
        return buf;
    } else if (!strcmp(msg_key, "tik_06_02")) {
        sprintf(buf, "<kanban>\n"
            "Thanks for playing the PM:TTYD\n"
            "Infinite Pit mod! Check the \n"
            "sign in back for your seed.\n<k>");
        return buf;
    } else if (!strcmp(msg_key, "in_cake")) {
        return "Strawberry Cake";
    } else if (!strcmp(msg_key, "msg_cake")) {
        return "Scrumptious strawberry cake \n"
               "that heals 15 HP and 15 FP.";
    } else if (!strcmp(msg_key, "msg_kame_no_noroi")) {
        return "Has a chance of inducing Slow \n"
               "status on all foes.";
    } else if (!strcmp(msg_key, "msg_teki_kyouka")) {
        return "Boosts foes' level by 5, but \n"
               "temporarily gives them +3 ATK.";
    } else if (!strcmp(msg_key, "msg_ice_candy")) {
        return "A dessert made by Zess T.\n"
               "Gives 15 FP, but might freeze!";
    } else if (!strcmp(msg_key, "list_ice_candy")) {
        return "A dessert made by Zess T.\n"
               "Gives 15 FP, but might freeze!\n"
               "Made by mixing Honey Syrup \n"
               "with an Ice Storm.";
    } else if (!strcmp(msg_key, "msg_nancy_frappe")) {
        return "A dessert made by Zess T.\n"
               "Gives 20 FP, but might freeze!";
    } else if (!strcmp(msg_key, "list_nancy_frappe")) {
        return "A dessert made by Zess T.\n"
               "Gives 20 FP, but might freeze!\n"
               "Made by mixing Maple Syrup \n"
               "with an Ice Storm.";
    } else if (!strcmp(msg_key, "in_toughen_up")) {
        return "Toughen Up";
    } else if (!strcmp(msg_key, "in_toughen_up_p")) {
        return "Toughen Up P";
    } else if (!strcmp(msg_key, "msg_toughen_up")) {
        return "Wear this to add Toughen Up\n"
               "to Mario's Tactics menu.\n"
               "This uses 1 FP to raise DEF\n"
               "by 2 points for a turn.\n"
               "Wearing more copies raises\n"
               "the effect and FP cost.";
    } else if (!strcmp(msg_key, "msg_toughen_up_p")) {
        return "Wear this to add Toughen Up\n"
               "to partners' Tactics menu.\n"
               "This uses 1 FP to raise DEF\n"
               "by 2 points for a turn.\n"
               "Wearing more copies raises\n"
               "the effect and FP cost.";
    } else if (!strcmp(msg_key, "btl_hlp_cmd_operation_super_charge")) {
        return "Briefly increases DEF by\n"
               "more than Defending.";
    } else if (!strcmp(msg_key, "msg_ptr_meromero_kiss")) {
        return "Blow a kiss to an enemy to try\n"
               "to win them to your side.";
    } else if (!strcmp(msg_key, "msg_super_coin")) {
        return "A mysterious, powerful object.\n"
               "Use it to power up your partner!";
    } else if (!strcmp(msg_key, "msg_custom_super_boots")) {
        return "A stronger pair of boots.";
    } else if (!strcmp(msg_key, "msg_custom_ultra_boots")) {
        return "An even stronger pair of boots.";
    } else if (!strcmp(msg_key, "msg_custom_super_hammer")) {
        return "A more powerful hammer.";
    } else if (!strcmp(msg_key, "msg_custom_ultra_hammer")) {
        return "An even more powerful hammer.";
    }

    return nullptr;
}

void ApplyWeaponLevelSelectionPatches() {
    g_pouchEquipCheckBadge_trampoline = patch::hookFunction(
        ttyd::mario_pouch::pouchEquipCheckBadge, [](int16_t badge_id) {
            if (g_InBattle) {
                int32_t idx = GetWeaponLevelSelectionIndex(badge_id);
                if (idx >= 0) {
                    return static_cast<int32_t>(g_CurMoveBadgeCounts[idx]);
                }
            }
            return g_pouchEquipCheckBadge_trampoline(badge_id);
        });

    g_BtlUnit_GetWeaponCost_trampoline = patch::hookFunction(
        ttyd::battle_unit::BtlUnit_GetWeaponCost,
        [](BattleWorkUnit* unit, BattleWeapon* weapon) {
            int32_t cost = GetSelectedLevelWeaponCost(unit, weapon);
            if (cost >= 0) return cost;
            return g_BtlUnit_GetWeaponCost_trampoline(unit, weapon);
        });

    g_DrawOperationWin_trampoline = patch::hookFunction(
        ttyd::battle_menu_disp::DrawOperationWin, []() {
            CheckForSelectingWeaponLevel(/* is_strategies_menu = */ true);
            g_DrawOperationWin_trampoline();
        });
        
    g_DrawWeaponWin_trampoline = patch::hookFunction(
        ttyd::battle_menu_disp::DrawWeaponWin, []() {
            CheckForSelectingWeaponLevel(/* is_strategies_menu = */ false);
            g_DrawWeaponWin_trampoline();
        });
        
    g__getSickStatusParam_trampoline = patch::hookFunction(
        ttyd::battle_damage::_getSickStatusParam, [](
            BattleWorkUnit* unit, BattleWeapon* weapon, int32_t status_type,
            int8_t* turn_count, int8_t* strength) {
                // Run vanilla logic.
                g__getSickStatusParam_trampoline(
                    unit, weapon, status_type, turn_count, strength);
                // If badge type and status type (DEF-Up) are correct,
                // change the effect strength based on the badges equipped.
                if (status_type == 14 && (
                    weapon->item_id == ItemType::SUPER_CHARGE ||
                    weapon->item_id == ItemType::SUPER_CHARGE_P)) {
                    bool is_mario = unit->current_kind == BattleUnitType::MARIO;
                    int8_t badges = g_CurMoveBadgeCounts[17 - is_mario];
                    *strength = badges + 1;
                }
                // TODO: Alter power of enemy charge moves.
            });
}

void ApplyItemAndAttackPatches() {
    // Rebalanced price tiers for badges.
    static const constexpr uint32_t kBadgePriceTiers[] = {
        0x32222633U, 0x66262224U, 0x77662244U, 0x55778888U, 0x33555555U, 
        0x44444773U, 0x32224581U, 0x63355553U, 0x41182216U, 0x11111564U,
        0x04111411U, 0x10000000U, 0x00000221U
    };
    static const constexpr int16_t kSquareDiamondIconId     =  44;
    static const constexpr int16_t kSquareDiamondPartnerId  =  87;
    static const constexpr int16_t kKoopaCurseIconId        = 390;
    
    // Individual balance / visual changes.
    itemDataTable[ItemType::TRADE_OFF].buy_price = 50;
    itemDataTable[ItemType::TRADE_OFF].sell_price = 25;
    itemDataTable[ItemType::TRIAL_STEW].buy_price = 50;
    itemDataTable[ItemType::TRIAL_STEW].sell_price = 25;
    itemDataTable[ItemType::STRANGE_SACK].buy_price = 250;
    itemDataTable[ItemType::ULTRA_HAMMER].buy_price = 250;
    itemDataTable[ItemType::CAKE].buy_price = 30;
    itemDataTable[ItemType::CAKE].sell_price = 15;
    itemDataTable[ItemType::CAKE].hp_restored = 15;
    itemDataTable[ItemType::CAKE].fp_restored = 15;
    itemDataTable[ItemType::FRESH_PASTA].buy_price = 15;
    itemDataTable[ItemType::FRESH_PASTA].sell_price = 10;
    itemDataTable[ItemType::KOOPASTA].buy_price = 20;
    itemDataTable[ItemType::KOOPASTA].sell_price = 15;
    itemDataTable[ItemType::SPICY_PASTA].buy_price = 30;
    itemDataTable[ItemType::SPICY_PASTA].sell_price = 20;
    itemDataTable[ItemType::INK_PASTA].buy_price = 30;
    itemDataTable[ItemType::INK_PASTA].sell_price = 20;
    itemDataTable[ItemType::KOOPA_CURSE].icon_id = kKoopaCurseIconId;
    itemDataTable[ItemType::PEEKABOO].bp_cost = 0;
    itemDataTable[ItemType::FP_DRAIN_P].bp_cost = 1;
    // Because, let's be honest.
    itemDataTable[ItemType::TORNADO_JUMP].bp_cost = 1;
    
    // Changed pickup messages for Super / Ultra boots and hammer.
    itemDataTable[ItemType::SUPER_BOOTS].description = "msg_custom_super_boots";
    itemDataTable[ItemType::ULTRA_BOOTS].description = "msg_custom_ultra_boots";
    itemDataTable[ItemType::SUPER_HAMMER].description = "msg_custom_super_hammer";
    itemDataTable[ItemType::ULTRA_HAMMER].description = "msg_custom_ultra_hammer";
    
    // New badges (Toughen Up, Toughen Up P).
    itemDataTable[ItemType::SUPER_CHARGE].bp_cost = 1;
    itemDataTable[ItemType::SUPER_CHARGE].icon_id = kSquareDiamondIconId;
    itemDataTable[ItemType::SUPER_CHARGE].name = "in_toughen_up";
    itemDataTable[ItemType::SUPER_CHARGE].description = "msg_toughen_up";
    itemDataTable[ItemType::SUPER_CHARGE].menu_description = "msg_toughen_up";
    itemDataTable[ItemType::SUPER_CHARGE_P].bp_cost = 1;
    itemDataTable[ItemType::SUPER_CHARGE_P].icon_id = kSquareDiamondPartnerId;
    itemDataTable[ItemType::SUPER_CHARGE_P].name = "in_toughen_up_p";
    itemDataTable[ItemType::SUPER_CHARGE_P].description = "msg_toughen_up";
    itemDataTable[ItemType::SUPER_CHARGE_P].menu_description = "msg_toughen_up_p";
    
    // Turn Gold Bars x3 into "Shine Sprites" that can be used from the menu.
    memcpy(&itemDataTable[ItemType::GOLD_BAR_X3], 
           &itemDataTable[ItemType::SHINE_SPRITE], sizeof(ItemData));
    itemDataTable[ItemType::GOLD_BAR_X3].usable_locations 
        |= ItemUseLocation::kField;
    
    // Set coin buy / discount / sell prices for badges to rebalanced values,
    // badge Star Piece costs on BP cost, recipe prices based on sell price,
    // cooking items' weapons, and fix unused items' and badges' sort order.    
    for (int32_t i = 0; i < ItemType::MAX_ITEM_TYPE; ++i) {
        ItemData& item = itemDataTable[i];
        if (i >= ItemType::GOLD_BAR && i <= ItemType::FRESH_JUICE) {
            // For all items that restore HP or FP, assign the "cooked item"
            // weapon struct if they don't already have a weapon assigned.
            if (!item.weapon_params && (item.hp_restored || item.fp_restored)) {
                item.weapon_params = 
                    &ttyd::battle_item_data::ItemWeaponData_CookingItem;
            }
            
            if (item.buy_price == 10 && item.sell_price > 8) {
                item.buy_price = item.sell_price * 5 / 4;
            }
            
            if (item.type_sort_order > 0x31) {
                item.type_sort_order += 1;
            }
        } else if (i >= ItemType::POWER_JUMP) {
            const int32_t word_index = (i - ItemType::POWER_JUMP) >> 3;
            const int32_t nybble_index = (i - ItemType::POWER_JUMP) & 7;
            const int32_t tier = 
                (kBadgePriceTiers[word_index] >> (nybble_index << 2)) & 15;
            if (tier > 0) {
                item.buy_price = tier > 4 ? tier * 50 - 100 : tier * 25;
            }
            item.star_piece_price = tier > 0 ? tier : 1;
            
            // Higher discounted price, since most prices in general are lower.
            item.discount_price = item.buy_price * 4 / 5;
            item.sell_price = item.buy_price >> 1;
            
            if (item.type_sort_order > 0x49) ++item.type_sort_order;
            if (item.type_sort_order > 0x43) ++item.type_sort_order;
            if (item.type_sort_order > 0x3b) ++item.type_sort_order;
            if (item.type_sort_order > 0x24) ++item.type_sort_order;
            if (item.type_sort_order > 0x21) ++item.type_sort_order;
            if (item.type_sort_order > 0x1f) ++item.type_sort_order;
            if (item.type_sort_order > 0x16) item.type_sort_order += 2;
        }
    }
    
    // Fixed sort order for Koopa Curse, new badges, and unused 'P' badges.
    itemDataTable[ItemType::KOOPA_CURSE].type_sort_order        = 0x31 + 1;
    
    itemDataTable[ItemType::SUPER_CHARGE].type_sort_order       = 0x16 + 1;
    itemDataTable[ItemType::SUPER_CHARGE_P].type_sort_order     = 0x16 + 2;
    // Leftover code for Mini HP-/FP-Plus from Shufflizer.
    // itemDataTable[ItemType::SQUARE_DIAMOND_BADGE].type_sort_order = 0x1f + 3;
    // itemDataTable[ItemType::SQUARE_DIAMOND_BADGE_P].type_sort_order = 0x21 + 4;
    itemDataTable[ItemType::ALL_OR_NOTHING_P].type_sort_order   = 0x24 + 5;
    itemDataTable[ItemType::LUCKY_DAY_P].type_sort_order        = 0x3b + 6;
    itemDataTable[ItemType::PITY_FLOWER_P].type_sort_order      = 0x43 + 7;
    itemDataTable[ItemType::FP_DRAIN_P].type_sort_order         = 0x49 + 8;
    
    // Reinstate Fire Pop's fire damage (base it off of Electro Pop's params).
    static BattleWeapon kFirePopParams;
    memcpy(&kFirePopParams,
           &ttyd::battle_item_data::ItemWeaponData_BiribiriCandy,
           sizeof(BattleWeapon));
    kFirePopParams.item_id = ItemType::FIRE_POP;
    kFirePopParams.damage_function =
        reinterpret_cast<void*>(
            &ttyd::battle_weapon_power::weaponGetPowerDefault);
    kFirePopParams.damage_function_params[0] = 1;
    kFirePopParams.element = 1;  // fire (naturally)
    kFirePopParams.special_property_flags = 0x00030048;  // pierce defense
    kFirePopParams.electric_chance = 0;
    kFirePopParams.electric_time = 0;
    itemDataTable[ItemType::FIRE_POP].weapon_params = &kFirePopParams;
    
    // Make enemies prefer to use CookingItems like standard healing items.
    // (i.e. they use them on characters with less HP)
    ttyd::battle_item_data::ItemWeaponData_CookingItem.target_weighting_flags =
        ttyd::battle_item_data::ItemWeaponData_Kinoko.target_weighting_flags;
        
    // Make Point Swap and Trial Stew only target Mario or his partner.
    ttyd::battle_item_data::ItemWeaponData_Irekaeeru.target_class_flags = 
        0x01100070;
    ttyd::battle_item_data::ItemWeaponData_LastDinner.target_class_flags = 
        0x01100070;

    // Make Poison Mushrooms able to target anyone, and make enemies prefer
    // to target Mario's team or characters with lower health.
    ttyd::battle_item_data::ItemWeaponData_PoisonKinoko.target_class_flags = 
        0x01100060;
    ttyd::battle_item_data::ItemWeaponData_PoisonKinoko.target_weighting_flags =
        0x80001403;
    // Make Poison Mushrooms poison & halve HP 67% of the time instead of 80%.
    const int32_t kPoisonMushroomChanceAddr = 0x8036c914;
    const int32_t kPoisonMushroomChance = 67;
    mod::patch::writePatch(
        reinterpret_cast<void*>(kPoisonMushroomChanceAddr),
        &kPoisonMushroomChance, sizeof(kPoisonMushroomChance));
        
    // Make Trade Off usable only on the enemy party.
    ttyd::battle_item_data::ItemWeaponData_Teki_Kyouka.target_class_flags =
        0x02100063;
    // Make it inflict +ATK for 9 turns (and increase level by 5, as usual).
    ttyd::battle_item_data::ItemWeaponData_Teki_Kyouka.atk_change_chance = 100;
    ttyd::battle_item_data::ItemWeaponData_Teki_Kyouka.atk_change_time     = 9;
    ttyd::battle_item_data::ItemWeaponData_Teki_Kyouka.atk_change_strength = 3;
    // Patch in evt code to actually apply the item's newly granted status.
    const int32_t kTradeOffScriptHookAddr = 0x80369b34;
    mod::patch::writePatch(
        reinterpret_cast<void*>(kTradeOffScriptHookAddr),
        TradeOffPatch, sizeof(TradeOffPatch));
        
    // Make Koopa Curse multi-target.
    ttyd::battle_item_data::ItemWeaponData_Kameno_Noroi.target_class_flags =
        0x02101260;
        
    // Add 75%-rate Dizzy status to Tornado Jump's tornadoes.
    ttyd::battle_mario::badgeWeapon_TatsumakiJumpInvolved.dizzy_time = 3;
    ttyd::battle_mario::badgeWeapon_TatsumakiJumpInvolved.dizzy_chance = 75;
        
    // Make Piercing Blow stackable (copy Hammer Throw damage function & params)
    memcpy(
        &ttyd::battle_mario::badgeWeapon_TsuranukiNaguri.damage_function,
        &ttyd::battle_mario::badgeWeapon_HammerNageru.damage_function,
        9 * sizeof(uint32_t));
    // Determines which badge type to count to determine the power level.
    ttyd::battle_mario::badgeWeapon_TsuranukiNaguri.damage_function_params[6] =
        ItemType::PIERCING_BLOW;
        
    // Change Super Charge (P) into Toughen Up (P), a single-turn DEF buff.
    ttyd::battle_mario::badgeWeapon_SuperCharge.base_fp_cost = 1;
    ttyd::battle_mario::badgeWeapon_SuperCharge.charge_strength = 0;
    ttyd::battle_mario::badgeWeapon_SuperCharge.def_change_chance = 100;
    ttyd::battle_mario::badgeWeapon_SuperCharge.def_change_time = 1;
    ttyd::battle_mario::badgeWeapon_SuperCharge.def_change_strength = 2;
    ttyd::battle_mario::badgeWeapon_SuperCharge.icon = kSquareDiamondIconId;
    ttyd::battle_mario::badgeWeapon_SuperCharge.name = "in_toughen_up";
    
    ttyd::battle_mario::badgeWeapon_SuperChargeP.base_fp_cost = 1;
    ttyd::battle_mario::badgeWeapon_SuperChargeP.charge_strength = 0;
    ttyd::battle_mario::badgeWeapon_SuperChargeP.def_change_chance = 100;
    ttyd::battle_mario::badgeWeapon_SuperChargeP.def_change_time = 1;
    ttyd::battle_mario::badgeWeapon_SuperChargeP.def_change_strength = 2;
    ttyd::battle_mario::badgeWeapon_SuperChargeP.icon = kSquareDiamondIconId;
    ttyd::battle_mario::badgeWeapon_SuperChargeP.name = "in_toughen_up";
        
    // Change base FP cost of some moves.
    ttyd::battle_mario::badgeWeapon_Charge.base_fp_cost = 2;
    ttyd::battle_mario::badgeWeapon_ChargeP.base_fp_cost = 2;
    ttyd::battle_mario::badgeWeapon_IceNaguri.base_fp_cost = 2;
    ttyd::battle_mario::badgeWeapon_TatsumakiJump.base_fp_cost = 2;
    ttyd::battle_mario::badgeWeapon_FireNaguri.base_fp_cost = 4;
    
    // Disable getting coins and experience from a successful Gale Force.
    const int32_t kGaleForceKillHookAddr = 0x80351ea4;
    mod::patch::writePatch(
        reinterpret_cast<void*>(kGaleForceKillHookAddr),
        GaleForceKillPatch, sizeof(GaleForceKillPatch));
        
    // Make Infatuate single-target and slightly cheaper.
    ttyd::unit_party_vivian::partyWeapon_VivianCharmKissAttack.
        target_class_flags = 0x01101260;
    ttyd::unit_party_vivian::partyWeapon_VivianCharmKissAttack.base_fp_cost = 3;
    // Disable checking for the next enemy in the attack's event script.
    *reinterpret_cast<int32_t*>(0x8038df34) = 0x0002001a;
    // Call a custom function on successfully hitting an enemy w/Infatuate
    // that makes the enemy permanently switch alliances (won't work on bosses).
    const int32_t kInfatuateChangeAllianceHookAddr = 0x8038de50;
    const int32_t kInfatuateChangeAllianceFuncAddr =
        reinterpret_cast<int32_t>(InfatuateChangeAlliance);
    mod::patch::writePatch(
        reinterpret_cast<void*>(kInfatuateChangeAllianceHookAddr),
        &kInfatuateChangeAllianceFuncAddr, sizeof(int32_t));
        
    // Replace Kiss Thief's item stealing routine with a custom one.
    const int32_t kKissThiefItemHookAddr = 0x80386258;
    const int32_t kKissThiefItemFuncAddr =
        reinterpret_cast<int32_t>(GetKissThiefResult);
    mod::patch::writePatch(
        reinterpret_cast<void*>(kKissThiefItemHookAddr),
        &kKissThiefItemFuncAddr, sizeof(int32_t));
        
    // Increase Tease's base status rate to 1.27x.
    g__make_madowase_weapon_trampoline = mod::patch::hookFunction(
        ttyd::unit_party_chuchurina::_make_madowase_weapon,
        [](EvtEntry* evt, bool isFirstCall) {
            g__make_madowase_weapon_trampoline(evt, isFirstCall);
            reinterpret_cast<BattleWeapon*>(evt->lwData[12])->dizzy_chance
                *= 1.27;
            return 2;
        });
        
    // Increase Hold Fast and Return Postage's returned damage to 1x
    // (but not Poison counter status / regular Payback status).
    const int32_t kHoldFastCounterDivisorHookAddr = 0x800fb800;
    const int32_t kReturnPostageCounterDivisorHookAddr = 0x800fb824;
    const uint32_t kLoadCounterDivisorOpcode = 0x38000032;  // li r0, 50
    mod::patch::writePatch(
        reinterpret_cast<void*>(kHoldFastCounterDivisorHookAddr),
        &kLoadCounterDivisorOpcode, sizeof(int32_t));
    mod::patch::writePatch(
        reinterpret_cast<void*>(kReturnPostageCounterDivisorHookAddr),
        &kLoadCounterDivisorOpcode, sizeof(int32_t));

    // Smooch parameter changes to make it restore 0 - 15 HP and cost 5 FP.
    *reinterpret_cast<int32_t*>(0x80386ae4) = 7;    // % of bar that = 1 HP   
    *reinterpret_cast<int32_t*>(0x80386b10) = 1;    // flag to disable base 1 HP
    *reinterpret_cast<int32_t*>(0x80386af8) = 35;   // Bar color thresholds
    *reinterpret_cast<int32_t*>(0x80386afc) = 70;
    *reinterpret_cast<int32_t*>(0x80386c4c) = 15;   // AC success lvl thresholds
    *reinterpret_cast<int32_t*>(0x80386c60) = 10;
    *reinterpret_cast<int32_t*>(0x80386c74) = 5;
    ttyd::unit_party_chuchurina::partyWeapon_ChuchurinaKiss.base_fp_cost = 5;
    
    // Misc. other party move balance changes.
    ttyd::unit_party_christine::partyWeapon_ChristineNormalAttack.
        damage_function_params[0] = 2;
    ttyd::unit_party_christine::partyWeapon_ChristineNormalAttack.
        damage_function_params[1] = 2;
    static constexpr const int32_t kKoopsBobberyBaseAttackParams[] =
        { 1, 3, 2, 5, 3, 7 };
    static constexpr const int32_t kKoopsBobberyFirstAttackParams[] =
        { 3, 3, 5, 5, 7, 7 };
    memcpy(
        ttyd::unit_party_nokotarou::partyWeapon_NokotarouKouraAttack.
            damage_function_params,
        kKoopsBobberyBaseAttackParams, sizeof(kKoopsBobberyBaseAttackParams));
    memcpy(
        ttyd::unit_party_sanders::partyWeapon_SandersNormalAttack.
            damage_function_params,
        kKoopsBobberyBaseAttackParams, sizeof(kKoopsBobberyBaseAttackParams));
    memcpy(
        ttyd::unit_party_nokotarou::partyWeapon_NokotarouFirstAttack.
            damage_function_params,
        kKoopsBobberyFirstAttackParams, sizeof(kKoopsBobberyFirstAttackParams));
    memcpy(
        ttyd::unit_party_sanders::partyWeapon_SandersFirstAttack.
            damage_function_params,
        kKoopsBobberyFirstAttackParams, sizeof(kKoopsBobberyFirstAttackParams));
    // Stampede is limited to targets within Hammer range.
    ttyd::unit_party_yoshi::partyWeapon_YoshiCallGuard.
        target_property_flags |= 0x2000;
    ttyd::unit_party_yoshi::partyWeapon_YoshiCallGuard.base_fp_cost = 7;
    // Bomb Squad and Bob-ombast are Defense-piercing.
    ttyd::unit_bomzou::weapon_bomzou_explosion.special_property_flags |= 0x40;
    ttyd::unit_party_sanders::partyWeapon_SandersSuperBombAttack.
        special_property_flags |= 0x40;
    // Love Slap immune to fire and spikes (except pre-emptive spikes).
    ttyd::unit_party_chuchurina::partyWeapon_ChuchurinaNormalAttackLeft.
        counter_resistance_flags |= 0x1a;
    ttyd::unit_party_chuchurina::partyWeapon_ChuchurinaNormalAttackRight.
        counter_resistance_flags |= 0x1a;
    ttyd::unit_party_chuchurina::partyWeapon_ChuchurinaNormalAttackLeftLast.
        counter_resistance_flags |= 0x1a;
    ttyd::unit_party_chuchurina::partyWeapon_ChuchurinaNormalAttackRightLast.
        counter_resistance_flags |= 0x1a;
    // Love Slap maximum power of 4, 6 at Super, Ultra Rank.
    *reinterpret_cast<int32_t*>(0x803825ac) = 34;  // % of bar per extra damage
    *reinterpret_cast<int32_t*>(0x80382558) = 20;
    *reinterpret_cast<int32_t*>(0x8038256c) = 40;  // Bar color thresholds
    *reinterpret_cast<int32_t*>(0x80382570) = 60;
    *reinterpret_cast<int32_t*>(0x80382574) = 80;
    // Kiss Thief immune to all contact hazards and can target any height.
    ttyd::unit_party_chuchurina::partyWeapon_ChuchurinaItemSteal.
        counter_resistance_flags |= 0x7ff;
    ttyd::unit_party_chuchurina::partyWeapon_ChuchurinaItemSteal.
        target_property_flags &= ~0x2000;
    ttyd::unit_party_chuchurina::partyWeapon_ChuchurinaItemSteal.
        base_fp_cost = 5;
    
    // Sweet Feast and Showstopper both cost 4 SP.
    ttyd::battle_mario::marioWeapon_Genki1.base_sp_cost = 4;
    ttyd::battle_mario::marioWeapon_Suki.base_sp_cost = 4;
}

void ApplyMiscPatches() {
    // Skip the calls to blank out all GSW(F)s when loading a new file.
    const int32_t kGswfInitHookAddr1 = 0x800f6358;      // loading new file
    const int32_t kGswfInitHookAddr2 = 0x800f3ecc;      // continuing new file
    const uint32_t kSkipGswfInitOpcode = 0x48000010;    // b 0x10
    mod::patch::writePatch(
        reinterpret_cast<void*>(kGswfInitHookAddr1),
        &kSkipGswfInitOpcode, sizeof(kSkipGswfInitOpcode));
    mod::patch::writePatch(
        reinterpret_cast<void*>(kGswfInitHookAddr2),
        &kSkipGswfInitOpcode, sizeof(kSkipGswfInitOpcode));
    
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
    
    // Individual instruction patches to make the battle condition message
    // display longer and only be dismissable by the B button.
    const int32_t kLengthenRuleDispTimeOpAddr = 0x8011c5ec;
    const uint32_t kLengthenRuleDispTimeOpcode = 0x3800012c;  // li r0, 300
    mod::patch::writePatch(
        reinterpret_cast<void*>(kLengthenRuleDispTimeOpAddr),
        &kLengthenRuleDispTimeOpcode, sizeof(uint32_t));
    const int32_t kDismissRuleDispButtonOpAddr = 0x8011c62c;
    const uint32_t kDismissRuleDispButtonOpcode = 0x38600200;  // li r3, 0x200
    mod::patch::writePatch(
        reinterpret_cast<void*>(kDismissRuleDispButtonOpAddr),
        &kDismissRuleDispButtonOpcode, sizeof(uint32_t));
        
    // Patch Charlieton's sell price scripts, making them scale from 1x to 3x.
    const int32_t kCharlietonPitListHookAddr = 0x8023c120;
    const int32_t kCharlietonPitItemHookAddr = 0x8023d2e0;
    mod::patch::writePatch(
        reinterpret_cast<void*>(kCharlietonPitListHookAddr),
        reinterpret_cast<void*>(CharlietonPitPriceListPatchStart),
        reinterpret_cast<void*>(CharlietonPitPriceListPatchEnd));
    mod::patch::writePatch(
        reinterpret_cast<void*>(kCharlietonPitItemHookAddr),
        reinterpret_cast<void*>(CharlietonPitPriceItemPatchStart),
        reinterpret_cast<void*>(CharlietonPitPriceItemPatchEnd));
        
    // Enable the crash handler.
    const int32_t kCrashHandlerEnableOpAddr = 0x80009b2c;
    const uint32_t kEnableHandlerOpcode = 0x3800FFFF;  // li r0, -1
    mod::patch::writePatch(
        reinterpret_cast<void*>(kCrashHandlerEnableOpAddr),
        &kEnableHandlerOpcode, sizeof(uint32_t));
        
    // Skip tutorials for boots / hammer upgrades.
    const int32_t kSkipUpgradeCutsceneOpAddr = 0x800abcd8;
    const uint32_t kSkipCutsceneOpcode = 0x48000030;  // b 0x30
    mod::patch::writePatch(
        reinterpret_cast<void*>(kSkipUpgradeCutsceneOpAddr),
        &kSkipCutsceneOpcode, sizeof(uint32_t));
        
    // Disable the check for enemies only holding certain types of items.
    const int32_t kSkipEnemyHeldItemCheckOpAddr = 0x80125d54;
    const uint32_t kSkipEnemyHeldItemCheckOpcode = 0x60000000;  // nop
    mod::patch::writePatch(
        reinterpret_cast<void*>(kSkipEnemyHeldItemCheckOpAddr),
        &kSkipEnemyHeldItemCheckOpcode, sizeof(uint32_t));
        
    // Make item names in battle menu based on item data rather than weapon data
    const int32_t kGetItemWeaponNameOpAddr = 0x80124924;
    const uint32_t kGetItemWeaponNameOpcode = 0x807b0004;  // r3, 4 (r27)
    mod::patch::writePatch(
        reinterpret_cast<void*>(kGetItemWeaponNameOpAddr),
        &kGetItemWeaponNameOpcode, sizeof(uint32_t));
}

EVT_DEFINE_USER_FUNC(GetEnemyNpcInfo) {
    ttyd::npcdrv::NpcTribeDescription* npc_tribe_description;
    ttyd::npcdrv::NpcSetupInfo* npc_setup_info;
    int32_t lead_enemy_type;
    BuildBattle(
        g_PitModulePtr, g_Randomizer->state_.floor_, &npc_tribe_description, 
        &npc_setup_info, &lead_enemy_type);
    int8_t* enemy_100 = 
        reinterpret_cast<int8_t*>(g_PitModulePtr + kPitEnemy100Offset);
    int8_t battle_setup_idx = enemy_100[g_Randomizer->state_.floor_ % 100];
    const int32_t x_sign = ttyd::system::irand(2) ? 1 : -1;
    const int32_t x_pos = ttyd::system::irand(50) + 80;
    const int32_t z_pos = ttyd::system::irand(200) - 100;
    int32_t y_pos = 0;
    
    // Select the NPC's y_pos based on the lead enemy type.
    switch(lead_enemy_type) {
        case BattleUnitType::DARK_PUFF:
        case BattleUnitType::RUFF_PUFF:
        case BattleUnitType::ICE_PUFF:
        case BattleUnitType::POISON_PUFF:
            y_pos = 10;
            break;
        case BattleUnitType::EMBER:
        case BattleUnitType::LAVA_BUBBLE:
        case BattleUnitType::PHANTOM_EMBER:
        case BattleUnitType::WIZZERD:
        case BattleUnitType::DARK_WIZZERD:
        case BattleUnitType::ELITE_WIZZERD:
        case BattleUnitType::LAKITU:
        case BattleUnitType::DARK_LAKITU:
            y_pos = 20;
            break;
        case BattleUnitType::BOO:
        case BattleUnitType::DARK_BOO:
        case BattleUnitType::ATOMIC_BOO:
        case BattleUnitType::YUX:
        case BattleUnitType::Z_YUX:
        case BattleUnitType::X_YUX:
            y_pos = 30;
            break;
        case BattleUnitType::PARAGOOMBA:
        case BattleUnitType::PARAGLOOMBA:
        case BattleUnitType::HYPER_PARAGOOMBA:
        case BattleUnitType::PARATROOPA:
        case BattleUnitType::KP_PARATROOPA:
        case BattleUnitType::SHADY_PARATROOPA:
        case BattleUnitType::DARK_PARATROOPA:
        case BattleUnitType::PARABUZZY:
        case BattleUnitType::SPIKY_PARABUZZY:
            y_pos = 50;
            break;
        case BattleUnitType::SWOOPER:
        case BattleUnitType::SWOOPULA:
        case BattleUnitType::SWAMPIRE:
            y_pos = 80;
            break;
        case BattleUnitType::PIDER:
        case BattleUnitType::ARANTULA:
            y_pos = 140;
            break;
        case BattleUnitType::CHAIN_CHOMP:
        case BattleUnitType::RED_CHOMP:
            npc_setup_info->territoryBase = { 
                static_cast<float>(x_pos), 0.f, static_cast<float>(z_pos) };
            break;
    }
    
    evtSetValue(evt, evt->evtArguments[0], PTR(npc_tribe_description->modelName));
    evtSetValue(evt, evt->evtArguments[1], PTR(npc_tribe_description->nameJp));
    evtSetValue(evt, evt->evtArguments[2], PTR(npc_setup_info));
    evtSetValue(evt, evt->evtArguments[3], battle_setup_idx);
    evtSetValue(evt, evt->evtArguments[4], x_pos * x_sign);
    evtSetValue(evt, evt->evtArguments[5], y_pos);
    evtSetValue(evt, evt->evtArguments[6], z_pos);
        
    return 2;
}

EVT_DEFINE_USER_FUNC(SetEnemyNpcBattleInfo) {
    const char* name = 
        reinterpret_cast<const char*>(evtGetValue(evt, evt->evtArguments[0]));
    int32_t battle_id = evtGetValue(evt, evt->evtArguments[1]);
    NpcEntry* npc = ttyd::evt_npc::evtNpcNameToPtr(evt, name);
    ttyd::npcdrv::npcSetBattleInfo(npc, battle_id);
    
    // Set the enemies' held items.
    NpcBattleInfo* battle_info = &npc->battleInfo;
    for (int32_t i = 0; i < battle_info->pConfiguration->num_enemies; ++i) {
        battle_info->wHeldItems[i] =
            PickRandomItem(/* seeded = */ true, 40, 20, 40, 0);
    }
    // Occasionally, set a battle condition for an optional bonus reward.
    SetBattleCondition(&npc->battleInfo);
    return 2;
}

EVT_DEFINE_USER_FUNC(GetNumChestRewards) {
    int32_t num_rewards = 1;
    if (g_Randomizer->state_.floor_ % 50 == 49) num_rewards = 2;
    evtSetValue(evt, evt->evtArguments[0], num_rewards);
    return 2;
}

EVT_DEFINE_USER_FUNC(GetChestReward) {
    evtSetValue(evt, evt->evtArguments[0], PickChestReward());
    return 2;
}

EVT_DEFINE_USER_FUNC(CheckRewardClaimed) {
    bool reward_claimed = false;
    for (uint32_t i = 0x13d3; i <= 0x13dc; ++i) {
        if (ttyd::swdrv::swGet(i)) reward_claimed = true;
    }
    evtSetValue(evt, evt->evtArguments[0], reward_claimed);
    return 2;
}

EVT_DEFINE_USER_FUNC(CheckPromptSave) {
    evtSetValue(evt, evt->evtArguments[0], g_PromptSave);
    if (g_PromptSave) {
        g_Randomizer->state_.Save();
        g_PromptSave = false;
    }
    return 2;
}

EVT_DEFINE_USER_FUNC(IncrementInfinitePitFloor) {
    int32_t actual_floor = ++g_Randomizer->state_.floor_;
    // Update the floor number used by the game.
    // Floors 101+ are treated as looping 81-90 nine times + 91-100.
    int32_t gsw_floor = actual_floor;
    if (actual_floor >= 100) {
        gsw_floor = actual_floor % 10;
        gsw_floor += ((actual_floor / 10) % 10 == 9) ? 90 : 80;
    }
    ttyd::swdrv::swByteSet(1321, gsw_floor);
    return 2;
}

EVT_DEFINE_USER_FUNC(AddItemStarPower) {
    int16_t item = evtGetValue(evt, evt->evtArguments[0]);
    if (item == ItemType::MAGICAL_MAP ||
        (item >= ItemType::DIAMOND_STAR && item <= ItemType::CRYSTAL_STAR)) {
        PouchData& pouch = *ttyd::mario_pouch::pouchGetPtr();
        pouch.max_sp += 100;
        pouch.current_sp = pouch.max_sp;
        if (item == ItemType::MAGICAL_MAP) {
            pouch.star_powers_obtained |= 1;
        } else {
            pouch.star_powers_obtained |= 
                (1 << (item + 1 - ItemType::DIAMOND_STAR));
        }
    }
    return 2;
}

EVT_DEFINE_USER_FUNC(InfatuateChangeAlliance) {
    // Assumes that LW(3) / LW(4) contains the targeted battle unit / part's id.
    auto* battleWork = ttyd::battle::g_BattleWork;
    int32_t unit_idx = ttyd::battle_sub::BattleTransID(evt, evt->lwData[3]);
    int32_t part_idx = evt->lwData[4];
    auto* unit = ttyd::battle::BattleGetUnitPtr(battleWork, unit_idx);
    auto* part = ttyd::battle::BattleGetUnitPartsPtr(unit_idx, part_idx);
    
    // If not a boss enemy, undo Confusion status and change alliance.
    if (unit->current_kind != BattleUnitType::BONETAIL &&
        unit->current_kind != BattleUnitType::ATOMIC_BOO) {
        uint32_t dummy = 0;
        ttyd::battle_damage::BattleSetStatusDamage(
            &dummy, unit, part, 0x100 /* ignore status vulnerability */,
            5 /* Confusion */, 100, 0, 0, 0);
        unit->alliance = 0;
        
        // Unqueue the status message for inflicting confusion.
        static constexpr const uint32_t kNoStatusMsg[] = { 0xff000000U, 0, 0 };
        memcpy(
            reinterpret_cast<void*>(
                reinterpret_cast<uintptr_t>(battleWork) + 0x18ddc),
            kNoStatusMsg, sizeof(kNoStatusMsg));
        memcpy(
            reinterpret_cast<void*>(
                reinterpret_cast<uintptr_t>(unit) + 0xae8),
            kNoStatusMsg, sizeof(kNoStatusMsg));
    }
    // Call the function this user_func replaced with its original params.
    return ttyd::battle_event_cmd::btlevtcmd_AudienceDeclareACResult(
        evt, isFirstCall);
}

EVT_DEFINE_USER_FUNC(GetPercentOfMaxHP) {
    auto* battleWork = ttyd::battle::g_BattleWork;
    int32_t id = evtGetValue(evt, evt->evtArguments[0]);
    id = ttyd::battle_sub::BattleTransID(evt, id);
    auto* unit = ttyd::battle::BattleGetUnitPtr(battleWork, id);
    evtSetValue(
        evt, evt->evtArguments[1], unit->current_hp * 100 / unit->max_hp);
    return 2;
}

EVT_DEFINE_USER_FUNC(GetKissThiefResult) {
    auto* battleWork = ttyd::battle::g_BattleWork;
    int32_t id = evtGetValue(evt, evt->evtArguments[0]);
    id = ttyd::battle_sub::BattleTransID(evt, id);
    auto* unit = ttyd::battle::BattleGetUnitPtr(battleWork, id);
    uint32_t ac_result = evtGetValue(evt, evt->evtArguments[2]);
    
    int32_t item = unit->held_item;
    // No held item; pick a random item to steal;
    // 30% chance of item (20% normal, 10% recipe), 10% badge, 60% coin.
    if (!item) {
        item = PickRandomItem(/* seeded = */ false, 20, 10, 10, 60);
        if (!item) item = ItemType::COIN;
    }
    if ((ac_result & 2) == 0 || !ttyd::mario_pouch::pouchGetItem(item)) {
        // Action command unsuccessful, or inventory cannot hold the item.
        evtSetValue(evt, evt->evtArguments[1], 0);
    } else {
        // Remove the unit's held item.
        unit->held_item = 0;
        
        // Remove the corresponding held/stolen item from the NPC setup,
        // if this was one of the initial enemies in the loadout.
        if (!ttyd::battle_unit::BtlUnit_CheckUnitFlag(unit, 0x40000000)) {
            if (unit->group_index >= 0) {
                battleWork->fbat_info->wBattleInfo->wHeldItems
                    [unit->group_index] = 0;
            }
        } else {
            ttyd::battle_unit::BtlUnit_OffUnitFlag(unit, 0x40000000);
            if (unit->group_index >= 0) {
                auto* npc_battle_info = battleWork->fbat_info->wBattleInfo;
                npc_battle_info->wHeldItems[unit->group_index] = 0;
                npc_battle_info->wStolenItems[unit->group_index] = 0;
            }
        }
        
        // If a badge was stolen, re-equip the target unit's remaining badges.
        if (item >= ItemType::POWER_JUMP && item < ItemType::MAX_ITEM_TYPE) {
            int32_t kind = unit->current_kind;
            if (kind == BattleUnitType::MARIO) {
                ttyd::battle::BtlUnit_EquipItem(unit, 3, 0);
            } else if (kind >= BattleUnitType::GOOMBELLA) {
                ttyd::battle::BtlUnit_EquipItem(unit, 5, 0);
            } else {
                ttyd::battle::BtlUnit_EquipItem(unit, 1, 0);
            }
        }
        
        evtSetValue(evt, evt->evtArguments[1], item);
    }
    return 2;
}

}