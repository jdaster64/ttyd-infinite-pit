#include "patches_field.h"

#include "common_functions.h"
#include "common_types.h"
#include "custom_chest_reward.h"
#include "custom_condition.h"
#include "custom_enemy.h"
#include "custom_item.h"
#include "evt_cmd.h"
#include "mod.h"
#include "mod_state.h"
#include "patch.h"
#include "patches_core.h"
#include "patches_partner.h"

#include <ttyd/battle_database_common.h>
#include <ttyd/evt_badgeshop.h>
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
#include <ttyd/evt_window.h>
#include <ttyd/evtmgr.h>
#include <ttyd/evtmgr_cmd.h>
#include <ttyd/item_data.h>
#include <ttyd/mario_pouch.h>
#include <ttyd/npcdrv.h>
#include <ttyd/swdrv.h>
#include <ttyd/system.h>

#include <cinttypes>
#include <cstdio>

// Assembly patch functions.
extern "C" {
    // charlieton_patches.s (patched directly, not branched to)
    void CharlietonPitPriceListPatchStart();
    void CharlietonPitPriceListPatchEnd();
    void CharlietonPitPriceItemPatchStart();
    void CharlietonPitPriceItemPatchEnd();
}

namespace mod::infinite_pit {

namespace {
    
using ::ttyd::battle_database_common::BattleUnitSetup;
using ::ttyd::evt_bero::BeroEntry;
using ::ttyd::evtmgr_cmd::evtGetValue;
using ::ttyd::evtmgr_cmd::evtSetValue;
using ::ttyd::mario_pouch::PouchData;
using ::ttyd::npcdrv::NpcBattleInfo;
using ::ttyd::npcdrv::NpcEntry;

namespace BattleUnitType = ::ttyd::battle_database_common::BattleUnitType;
namespace ItemType = ::ttyd::item_data::ItemType;

}

// Function hooks.
// Patch addresses.
extern const int32_t g_select_disp_Patch_PitListPriceHook;
extern const int32_t g_select_disp_Patch_PitItemPriceHook;
extern const int32_t g_getBadgeBottakuru100TableMaxCount_Patch_ShopSize;
extern const uint32_t g_jon_setup_npc_ex_para_FuncOffset;
extern const uint32_t g_jon_yattukeFlag_FuncOffset;
extern const uint32_t g_jon_enemy_100_Offset;
extern const uint32_t g_jon_enemy_setup_EvtOffset;
extern const uint32_t g_jon_evt_open_box_EvtOffset;
extern const uint32_t g_jon_idouya_setup_MoverLastSpawnFloorOffset;
extern const uint32_t g_jon_talk_gyousyou_MinItemForBadgeDialogOffset;
extern const uint32_t g_jon_talk_gyousyou_NoInvSpaceBranchOffset;
extern const uint32_t g_jon_gyousyou_setup_CharlietonSpawnChanceOffset;
extern const uint32_t g_jon_dokan_open_PipeOpenEvtOffset;
extern const uint32_t g_jon_evt_kanban2_ReturnSignEvtOffset;
extern const uint32_t g_jon_floor_inc_EvtOffset;
extern const uint32_t g_jon_bero_boss_EntryBeroEntryOffset;
extern const uint32_t g_jon_bero_boss_ReturnBeroEntryOffset;
extern const uint32_t g_jon_setup_boss_EvtOffset;
extern const uint32_t g_jon_bero_return_ReturnBeroEntryOffset;
extern const uint32_t g_jon_zonbaba_first_event_EvtOffset;
extern const uint32_t g_jon_btlsetup_jon_tbl_Offset;

namespace field {

namespace {

// Global variables and constants.
constexpr const int32_t kNumCharlietonItemsPerType = 5;

const char kPitNpcName[] = "\x93\x47";  // "enemy"
const char kPiderName[] = "\x83\x70\x83\x43\x83\x5f\x81\x5b\x83\x58";
const char kArantulaName[] = 
    "\x83\x60\x83\x85\x83\x89\x83\x93\x83\x5e\x83\x89\x81\x5b";
const char kChainChompName[] = "\x83\x8f\x83\x93\x83\x8f\x83\x93";
const char kRedChompName[] = 
    "\x83\x6f\x81\x5b\x83\x58\x83\x67\x83\x8f\x83\x93\x83\x8f\x83\x93";
const char kBonetailName[] = "\x83\x5d\x83\x93\x83\x6f\x83\x6f";
    
// Declarations for USER_FUNCs.
EVT_DECLARE_USER_FUNC(GetEnemyNpcInfo, 7)
EVT_DECLARE_USER_FUNC(SetEnemyNpcBattleInfo, 2)
EVT_DECLARE_USER_FUNC(GetNumChestRewards, 1)
EVT_DECLARE_USER_FUNC(GetChestReward, 1)
EVT_DECLARE_USER_FUNC(CheckRewardClaimed, 1)
EVT_DECLARE_USER_FUNC(CheckPromptSave, 1)
EVT_DECLARE_USER_FUNC(IncrementInfinitePitFloor, 0)
EVT_DECLARE_USER_FUNC(GetUniqueItemName, 1)
EVT_DECLARE_USER_FUNC(AddItemStarPower, 1)

// Event that plays "get partner" fanfare.
EVT_BEGIN(PartnerFanfareEvt)
USER_FUNC(ttyd::evt_snd::evt_snd_bgmoff, 0x400)
USER_FUNC(ttyd::evt_snd::evt_snd_bgmon, 1, PTR("BGM_FF_GET_PARTY1"))
WAIT_MSEC(2000)
RETURN()
EVT_END()

// Event that handles a chest being opened, rewarding the player with
// items / partners (1 ~ 5 based on randomizer settings, +1 for boss floors).
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
        WAIT_MSEC(100)  // If the second+ reward
        USER_FUNC(ttyd::evt_mobj::evt_mobj_wait_animation_end, PTR("box"))
        USER_FUNC(ttyd::evt_mario::evt_mario_normalize)
        USER_FUNC(ttyd::evt_mario::evt_mario_goodbye_party, 0)
        WAIT_MSEC(500)
        USER_FUNC(ttyd::evt_pouch::evt_pouch_party_join, LW(1))
        USER_FUNC(partner::InitializePartyMember, LW(1))
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
        USER_FUNC(GetUniqueItemName, LW(14))
        USER_FUNC(
            ttyd::evt_item::evt_item_entry,
            LW(14), LW(1), LW(10), LW(11), LW(12), 17, -1, 0)
        WAIT_MSEC(300)  // If the second+ reward
        USER_FUNC(ttyd::evt_mobj::evt_mobj_wait_animation_end, PTR("box"))
        USER_FUNC(ttyd::evt_item::evt_item_get_item, LW(14))
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

// Event that sets up the boss floor (adds a signboard by the chest).
EVT_BEGIN(BossSetupEvt)
SET(LW(0), REL_PTR(ModuleId::JON, g_jon_bero_boss_EntryBeroEntryOffset))
USER_FUNC(ttyd::evt_bero::evt_bero_get_info)
RUN_CHILD_EVT(ttyd::evt_bero::evt_bero_info_run)
USER_FUNC(
    ttyd::evt_mobj::evt_mobj_signboard, PTR("board"), 190, 0, 200,
    REL_PTR(ModuleId::JON, g_jon_evt_kanban2_ReturnSignEvtOffset), LSWF(0))
RETURN()
EVT_END()

// Wrapper for modified boss floor setup event.
EVT_BEGIN(BossSetupEvtHook)
RUN_CHILD_EVT(BossSetupEvt)
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
    RUN_EVT(REL_PTR(ModuleId::JON, g_jon_zonbaba_first_event_EvtOffset))
END_IF()
USER_FUNC(SetEnemyNpcBattleInfo, PTR(kPitNpcName), LW(3))
UNCHECKED_USER_FUNC(
    REL_PTR(ModuleId::JON, g_jon_setup_npc_ex_para_FuncOffset),
    PTR(kPitNpcName))
UNCHECKED_USER_FUNC(REL_PTR(ModuleId::JON, g_jon_yattukeFlag_FuncOffset))
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
        RUN_EVT(REL_PTR(ModuleId::JON, g_jon_dokan_open_PipeOpenEvtOffset))
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

// Inventory full; ask the player if they still want to buy the item,
// and throw out an existing one.
EVT_BEGIN(CharlietonInvFullEvt)
USER_FUNC(
    ttyd::evt_msg::evt_msg_print_add, 0, PTR("pit_charlieton_full_inv"))
USER_FUNC(ttyd::evt_msg::evt_msg_select, 0, PTR("100kai_item_00"))
IF_EQUAL(LW(0), 1)  // Declined.
    USER_FUNC(ttyd::evt_window::evt_win_coin_off, LW(12))
    USER_FUNC(ttyd::evt_msg::evt_msg_print_add, 0, PTR("100kai_item_08"))
    RETURN()
END_IF()
// Spend coins.
MUL(LW(3), -1)
USER_FUNC(ttyd::evt_pouch::evt_pouch_add_coin, LW(3))
USER_FUNC(ttyd::evt_window::evt_win_coin_wait, LW(12))
WAIT_MSEC(200)
USER_FUNC(ttyd::evt_window::evt_win_coin_off, LW(12))
// Close text dialog and handle spawning item / throwing away.
USER_FUNC(ttyd::evt_msg::evt_msg_continue)
USER_FUNC(GetUniqueItemName, LW(0))
USER_FUNC(
    ttyd::evt_item::evt_item_entry,
    LW(0), LW(1), FLOAT(0.0), FLOAT(-999.0), FLOAT(0.0), 17, -1, 0)
USER_FUNC(ttyd::evt_item::evt_item_get_item, LW(0))
RETURN()
EVT_END()

// Wrapper for Charlieton full-inventory event.
EVT_BEGIN(CharlietonInvFullEvtHook)
RUN_CHILD_EVT(CharlietonInvFullEvt)
RETURN()
EVT_PATCH_END()

// Fetches information required for dynamically spawning an enemy NPC,
// such as the model name, battle id, and initial position.
EVT_DEFINE_USER_FUNC(GetEnemyNpcInfo) {
    ttyd::npcdrv::NpcTribeDescription* npc_tribe_description;
    ttyd::npcdrv::NpcSetupInfo* npc_setup_info;
    int32_t lead_enemy_type;
    BuildBattle(
        core::GetPitModulePtr(), g_Mod->state_.floor_, &npc_tribe_description, 
        &npc_setup_info, &lead_enemy_type);
    int8_t* enemy_100 = reinterpret_cast<int8_t*>(
        core::GetPitModulePtr() + g_jon_enemy_100_Offset);
    int8_t battle_setup_idx = enemy_100[g_Mod->state_.floor_ % 100];
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
                static_cast<float>(x_pos * x_sign), 0.f, 
                static_cast<float>(z_pos) };
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

// Sets final battle info on a Pit enemy, as well as setting any
// battle conditions.
EVT_DEFINE_USER_FUNC(SetEnemyNpcBattleInfo) {
    const char* name = 
        reinterpret_cast<const char*>(evtGetValue(evt, evt->evtArguments[0]));
    int32_t battle_id = evtGetValue(evt, evt->evtArguments[1]);
    NpcEntry* npc = ttyd::evt_npc::evtNpcNameToPtr(evt, name);
    ttyd::npcdrv::npcSetBattleInfo(npc, battle_id);
    
    PouchData& pouch = *ttyd::mario_pouch::pouchGetPtr();
    
    // If on a Bonetail floor and Bonetail is already defeated,
    // skip the item / condition generation to keep seeding consistent.
    if (ttyd::swdrv::swGet(0x13dc)) return 2;

    // Set the enemies' held items, if they get any.
    const int32_t reward_mode =
        g_Mod->state_.GetOptionValue(StateManager::BATTLE_REWARD_MODE);
    NpcBattleInfo* battle_info = &npc->battleInfo;
    if (reward_mode == StateManager::NO_HELD_ITEMS) {
        for (int32_t i = 0; i < battle_info->pConfiguration->num_enemies; ++i) {
            battle_info->wHeldItems[i] = 0;
        }
    } else {
        // If item drops only come from conditions, spawn Shine Sprites
        // as held items occasionally after floor 30.
        int32_t shine_rate = 0;
        if (g_Mod->state_.floor_ >= 30 &&
            reward_mode == StateManager::CONDITION_DROPS_HELD) {
            shine_rate = 13;
        }
            
        for (int32_t i = 0; i < battle_info->pConfiguration->num_enemies; ++i) {
            int32_t item =
                PickRandomItem(/* seeded = */ true, 40, 20, 40, shine_rate);
            
            // Indirectly attacking enemies should not hold defense-increasing
            // badges if the player cannot damage them at base rank equipment /
            // without a damaging Star Power.
            if (pouch.jump_level < 2 && !(pouch.star_powers_obtained & 0x92)) {
                const BattleUnitSetup& unit_setup = 
                    battle_info->pConfiguration->enemy_data[i];
                switch (unit_setup.unit_kind_params->unit_type) {
                    case BattleUnitType::DULL_BONES:
                    case BattleUnitType::LAKITU:
                    case BattleUnitType::DARK_LAKITU:
                    case BattleUnitType::MAGIKOOPA:
                    case BattleUnitType::RED_MAGIKOOPA:
                    case BattleUnitType::WHITE_MAGIKOOPA:
                    case BattleUnitType::GREEN_MAGIKOOPA:
                    case BattleUnitType::HAMMER_BRO:
                    case BattleUnitType::BOOMERANG_BRO:
                    case BattleUnitType::FIRE_BRO:
                        switch (item) {
                            case ItemType::DEFEND_PLUS:
                            case ItemType::DEFEND_PLUS_P:
                            case ItemType::P_DOWN_D_UP:
                            case ItemType::P_DOWN_D_UP_P:
                                // Pick a new item, disallowing badges.
                                item = PickRandomItem(
                                    /* seeded = */ true, 40, 20, 0, shine_rate);
                        }
                }
            }
            
            if (!item) item = ItemType::GOLD_BAR_X3;
            battle_info->wHeldItems[i] = item;
        }
    }
    
    // Occasionally, set a battle condition for an optional bonus reward.
    SetBattleCondition(&npc->battleInfo);
    
    return 2;
}

// Returns the number of chest rewards to spawn based on the floor number.
EVT_DEFINE_USER_FUNC(GetNumChestRewards) {
    int32_t num_rewards = 
        g_Mod->state_.options_ & StateManager::NUM_CHEST_REWARDS;
    if (num_rewards > 0) {
        // Add a bonus reward for beating a boss (Atomic Boo or Bonetail).
        if (g_Mod->state_.floor_ % 50 == 49) ++num_rewards;
    } else {
        // Pick a number of rewards randomly from 1 ~ 5.
        num_rewards = g_Mod->state_.Rand(5) + 1;
    }
    evtSetValue(evt, evt->evtArguments[0], num_rewards);
    return 2;
}

// Returns the item or partner to spawn from the chest on a Pit reward floor.
EVT_DEFINE_USER_FUNC(GetChestReward) {
    evtSetValue(evt, evt->evtArguments[0], PickChestReward());
    return 2;
}

// Returns whether or not the current floor's reward has been claimed.
EVT_DEFINE_USER_FUNC(CheckRewardClaimed) {
    bool reward_claimed = false;
    for (uint32_t i = 0x13d3; i <= 0x13dc; ++i) {
        if (ttyd::swdrv::swGet(i)) reward_claimed = true;
    }
    evtSetValue(evt, evt->evtArguments[0], reward_claimed);
    return 2;
}

// Returns whether or not to prompt the player to save.
EVT_DEFINE_USER_FUNC(CheckPromptSave) {
    evtSetValue(evt, evt->evtArguments[0], core::GetShouldPromptSave());
    if (core::GetShouldPromptSave()) {
        g_Mod->state_.SaveCurrentTime();
        g_Mod->state_.Save();
        core::SetShouldPromptSave(false);
    }
    return 2;
}

// Increments the actual current Pit floor, and the corresponding GSW value.
EVT_DEFINE_USER_FUNC(IncrementInfinitePitFloor) {
    int32_t actual_floor = ++g_Mod->state_.floor_;
    // Update the floor number used by the game.
    // Floors 101+ are treated as looping 81-90 nine times + 91-100.
    int32_t gsw_floor = actual_floor;
    if (actual_floor >= 100) {
        gsw_floor = actual_floor % 10;
        gsw_floor += ((actual_floor / 10) % 10 == 9) ? 90 : 80;
    }
    ttyd::swdrv::swByteSet(1321, gsw_floor);
    // Increase stage rank if entering floor 31/61/91, depending on settings.
    switch (actual_floor) {
        case 30:
        case 60:
        case 90: {
            if (g_Mod->state_.GetOptionValue(
                    StateManager::RANK_UP_REQUIREMENT) == 
                    StateManager::RANK_UP_BY_FLOOR) {
                ttyd::mario_pouch::pouchGetPtr()->rank++;
            }
            break;
        }
        default:
            break;
    }
    return 2;
}

// Gets a unique id for an item to spawn when buying items from Charlieton
// with a maxed inventory.
EVT_DEFINE_USER_FUNC(GetUniqueItemName) {
    static int32_t id = 0;
    static char name[16];
    
    id = (id + 1) % 1000;
    sprintf(name, "ch_item_%03" PRId32, id);
    evtSetValue(evt, evt->evtArguments[0], PTR(name));
    return 2;
}

// If the item is a Crystal Star, gives the player +1.00 max SP and
// the respective Star Power.
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

// Replaces Pit Charlieton's stock with items from the random pool.
void ReplaceCharlietonStock() {
    // Before setting stock, check if reloading an existing save file;
    // if so, set the RNG state to what it was at the start of the floor
    // so Charlieton's stock is the same as it was before.
    StateManager& state = g_Mod->state_;
    const int32_t current_rng_state = state.rng_state_;
    if (g_Mod->state_.load_from_save_) {
        g_Mod->state_.rng_state_ = state.saved_rng_state_;
    }
    
    // Fill in Charlieton's expanded inventory.
    int32_t* inventory = ttyd::evt_badgeshop::badge_bottakuru100_table;
    for (int32_t i = 0; i < kNumCharlietonItemsPerType * 3; ++i) {
        bool found = true;
        while (found) {
            found = false;
            int32_t item = PickRandomItem(
                /* seeded = */ true,
                i / kNumCharlietonItemsPerType == 0,
                i / kNumCharlietonItemsPerType == 1, 
                i / kNumCharlietonItemsPerType == 2, 
                0,
                /* force_no_partner = */ state.disable_partner_badges_in_shop_);
            // Make sure no duplicate items exist.
            for (int32_t j = 0; j < i; ++j) {
                if (inventory[j] == item) {
                    found = true;
                    break;
                }
            }
            inventory[i] = item;
        }
    }
    
    if (state.load_from_save_) {
        // If loaded from save, restore the previous RNG value so the seed
        // matches up with where it should start on the following floor.
        state.rng_state_ = current_rng_state;
    } else {
        // Otherwise, save what the RNG was before generating the list, so the
        // item list can be duplicated after loading a save.
        state.saved_rng_state_ = current_rng_state;
        state.load_from_save_ = true;
    }
}

}

void ApplyFixedPatches() {
    // Patch Charlieton's sell price scripts, making them scale from 20 to 100%.
    mod::patch::writePatch(
        reinterpret_cast<void*>(g_select_disp_Patch_PitListPriceHook),
        reinterpret_cast<void*>(CharlietonPitPriceListPatchStart),
        reinterpret_cast<void*>(CharlietonPitPriceListPatchEnd));
    mod::patch::writePatch(
        reinterpret_cast<void*>(g_select_disp_Patch_PitItemPriceHook),
        reinterpret_cast<void*>(CharlietonPitPriceItemPatchStart),
        reinterpret_cast<void*>(CharlietonPitPriceItemPatchEnd));
        
    // Change the length of Charlieton's shop item list.
    mod::patch::writePatch(
        reinterpret_cast<void*>(
            g_getBadgeBottakuru100TableMaxCount_Patch_ShopSize),
        0x38600000U | (kNumCharlietonItemsPerType * 3) /* li r3, N */);
}

void ApplyModuleLevelPatches(void* module_ptr, ModuleId::e module_id) {
    if (module_id != ModuleId::JON || !module_ptr) return;
    const uint32_t module_start = reinterpret_cast<uint32_t>(module_ptr);
    
    // Apply custom logic to box opening event to allow spawning partners.
    mod::patch::writePatch(
        reinterpret_cast<void*>(module_start + g_jon_evt_open_box_EvtOffset),
        ChestOpenEvtHook, sizeof(ChestOpenEvtHook));
        
    // Patch over boss floor setup script to spawn a sign in the boss room.
    mod::patch::writePatch(
        reinterpret_cast<void*>(module_start + g_jon_setup_boss_EvtOffset),
        BossSetupEvtHook, sizeof(BossSetupEvtHook));
        
    // Disable reward floors' return pipe and display a message if entered.
    BeroEntry* return_bero = reinterpret_cast<BeroEntry*>(
        module_start + g_jon_bero_return_ReturnBeroEntryOffset);
    return_bero->target_map = nullptr;
    return_bero->target_bero = return_bero->name;
    return_bero->out_evt_code = reinterpret_cast<void*>(
        const_cast<int32_t*>(DisabledBeroEvt));
    
    // Change destination of boss room pipe to be the usual 80+ floor.
    BeroEntry* boss_bero = reinterpret_cast<BeroEntry*>(
        module_start + g_jon_bero_boss_ReturnBeroEntryOffset);
    boss_bero->target_map = "jon_02";
    boss_bero->target_bero = "dokan_2";
    boss_bero->out_evt_code = reinterpret_cast<void*>(
        module_start + g_jon_floor_inc_EvtOffset);
    
    // Update the actual Pit floor alongside GSW(1321); also, check for
    // unclaimed rewards and prompt the player to save on X0 floors.
    mod::patch::writePatch(
        reinterpret_cast<void*>(module_start + g_jon_floor_inc_EvtOffset),
        FloorIncrementEvtHook, sizeof(FloorIncrementEvtHook));
       
    // Update the enemy setup event.
    mod::patch::writePatch(
        reinterpret_cast<void*>(module_start + g_jon_enemy_setup_EvtOffset),
        EnemyNpcSetupEvtHook, sizeof(EnemyNpcSetupEvtHook));
        
    // Replace Charlieton talk evt, adding a dialog for buying an item
    // and throwing away an old one if you have a full item/badge inventory.
    mod::patch::writePatch(
        reinterpret_cast<void*>(
            module_start + g_jon_talk_gyousyou_NoInvSpaceBranchOffset),
        CharlietonInvFullEvtHook, sizeof(CharlietonInvFullEvtHook));
        
    // Fix Charlieton's text when offering to sell a badge.
    mod::patch::writePatch(
        reinterpret_cast<void*>(
            module_start + g_jon_talk_gyousyou_MinItemForBadgeDialogOffset),
            1000);
        
    // Make Charlieton always spawn, and Movers never spawn.
    mod::patch::writePatch(
        reinterpret_cast<void*>(
            module_start + g_jon_gyousyou_setup_CharlietonSpawnChanceOffset),
            1000);
    mod::patch::writePatch(
        reinterpret_cast<void*>(
            module_start + g_jon_idouya_setup_MoverLastSpawnFloorOffset), 0);
    
    // If not reward floor, reset Pit-related flags and save-related status.
    if (g_Mod->state_.floor_ % 10 != 9) {
        for (uint32_t i = 0x13d3; i <= 0x13dd; ++i) {
            ttyd::swdrv::swClear(i);
        }
        
        core::SetShouldPromptSave(true);
        g_Mod->state_.load_from_save_ = false;

        const PouchData& pouch = *ttyd::mario_pouch::pouchGetPtr();
        for (int32_t i = 0; i < 8; ++i) {
            if (pouch.party_data[i].flags & 1) {
                g_Mod->state_.disable_partner_badges_in_shop_ = false;
                break;
            }
        }
    }
    // Otherwise, modify Charlieton's stock, using items from the randomizer
    // state if continuing from an existing save file.
    else {
        ReplaceCharlietonStock();
    }
}

void LinkCustomEvts(void* module_ptr, ModuleId::e module_id, bool link) {
    if (module_id != ModuleId::JON || !module_ptr) return;
    
    if (link) {
        LinkCustomEvt(
            module_id, module_ptr, const_cast<int32_t*>(BossSetupEvt));
        LinkCustomEvt(
            module_id, module_ptr, const_cast<int32_t*>(EnemyNpcSetupEvt));
        LinkCustomEvt(
            module_id, module_ptr, const_cast<int32_t*>(CharlietonInvFullEvt));
    } else {
        UnlinkCustomEvt(
            module_id, module_ptr, const_cast<int32_t*>(BossSetupEvt));
        UnlinkCustomEvt(
            module_id, module_ptr, const_cast<int32_t*>(EnemyNpcSetupEvt));
        UnlinkCustomEvt(
            module_id, module_ptr, const_cast<int32_t*>(CharlietonInvFullEvt));
    }
}

}  // namespace field
}  // namespace mod::infinite_pit