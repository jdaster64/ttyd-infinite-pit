#include "patches_partner.h"

#include "custom_enemy.h"
#include "custom_item.h"
#include "evt_cmd.h"
#include "mod.h"
#include "mod_state.h"
#include "patch.h"

#include <ttyd/battle.h>
#include <ttyd/battle_damage.h>
#include <ttyd/battle_database_common.h>
#include <ttyd/battle_event_cmd.h>
#include <ttyd/battle_sub.h>
#include <ttyd/evtmgr.h>
#include <ttyd/evtmgr_cmd.h>
#include <ttyd/item_data.h>
#include <ttyd/mario_pouch.h>
#include <ttyd/unit_bomzou.h>
#include <ttyd/unit_koura.h>
#include <ttyd/unit_party_christine.h>
#include <ttyd/unit_party_chuchurina.h>
#include <ttyd/unit_party_clauda.h>
#include <ttyd/unit_party_nokotarou.h>
#include <ttyd/unit_party_sanders.h>
#include <ttyd/unit_party_vivian.h>
#include <ttyd/unit_party_yoshi.h>

#include <cstdint>
#include <cstring>

namespace mod::infinite_pit {

namespace {

using ::ttyd::battle_database_common::BattleWeapon;
using ::ttyd::evtmgr::EvtEntry;
using ::ttyd::evtmgr_cmd::evtGetValue;
using ::ttyd::evtmgr_cmd::evtSetValue;

namespace BattleUnitType = ::ttyd::battle_database_common::BattleUnitType;
namespace ItemType = ::ttyd::item_data::ItemType;

}

// Function hooks.
extern int32_t (*g__make_madowase_weapon_trampoline)(EvtEntry*, bool);
extern int32_t (*g_btlevtcmd_get_monosiri_msg_no_trampoline)(EvtEntry*, bool);
// Patch addresses.
extern const int32_t g_partyNokotarouAttack_KouraGuard_Patch_SetHpFunc;
extern const int32_t g_koura_pose_tbl_reset_Patch_HeavyDmg;
extern const int32_t g_koura_pose_tbl_reset_Patch_LightDmg;
extern const int32_t g_koura_damage_core_Patch_HeavyDmg;
extern const int32_t g_koura_damage_core_Patch_LightDmg;
extern const int32_t g_subsetevt_blow_dead_Patch_GetRewards;
extern const int32_t g_BattleSetStatusDamage_Patch_GaleLevelFactor;
extern const int32_t g_partyClaudaAttack_KumoGuard_Patch_UseOnSelf;
extern const int32_t g_partyVivianAttack_CharmKissAttack_Patch_DisableNext;
extern const int32_t g_partyVivianAttack_CharmKissAttack_Patch_ResultFunc;
extern const int32_t g_partyChuchurinaAttack_ItemSteal_Patch_StealFunc;
extern const int32_t g_partyChuchurinaAttack_Kiss_Patch_BarPercentPerHp;
extern const int32_t g_partyChuchurinaAttack_Kiss_Patch_DisableBase1Hp;
extern const int32_t g_partyChuchurinaAttack_Kiss_Patch_BarColor1;
extern const int32_t g_partyChuchurinaAttack_Kiss_Patch_BarColor2;
extern const int32_t g_partyChuchurinaAttack_Kiss_Patch_AcLevel3;
extern const int32_t g_partyChuchurinaAttack_Kiss_Patch_AcLevel2;
extern const int32_t g_partyChuchurinaAttack_Kiss_Patch_AcLevel1;
extern const int32_t g_partyChuchurinaAttack_NormalAttack_Patch_PercentPerDmgSuper;
extern const int32_t g_partyChuchurinaAttack_NormalAttack_Patch_PercentPerDmgUltra;
extern const int32_t g_partyChuchurinaAttack_NormalAttack_Patch_BarColor1;
extern const int32_t g_partyChuchurinaAttack_NormalAttack_Patch_BarColor2;
extern const int32_t g_partyChuchurinaAttack_NormalAttack_Patch_BarColor3;

namespace partner {
    
namespace {

// Run at end of Dodgy Fog evt for Flurrie to also apply the status to herself.
EVT_BEGIN(DodgyFogFlurrieEvt)
USER_FUNC(
    ttyd::battle_event_cmd::btlevtcmd_CheckDamage, -2, -2, 1, LW(12), 256, LW(5))
RETURN()
EVT_END()
// Hook to run above event.
EVT_BEGIN(DodgyFogFlurriePatch)
RUN_CHILD_EVT(DodgyFogFlurrieEvt)
RETURN()
EVT_END()

// Patch to disable the coins / EXP from Gale Force for most enemies.
// (Replaces the code with no-ops).  Needs to be patched into enemies' specific
// death events in some cases as well.
EVT_BEGIN(GaleForceKillPatch)
DEBUG_REM(0) DEBUG_REM(0) DEBUG_REM(0) DEBUG_REM(0)
DEBUG_REM(0) DEBUG_REM(0) DEBUG_REM(0)
EVT_PATCH_END()
static_assert(sizeof(GaleForceKillPatch) == 0x38);

EVT_DEFINE_USER_FUNC(ShellShieldSetInitialHp) {
    int32_t ac_level = evtGetValue(evt, evt->evtArguments[1]);
    // Action command rating of 2, 4, 6, 8 -> starting HP of 1, 1, 2, 3.
    int32_t starting_hp = ac_level / 2 - 1;
    if (starting_hp < 1) starting_hp = 1;
    if (starting_hp > 3) starting_hp = 3;
    
    evtSetValue(evt, evt->evtArguments[1], starting_hp);
    ttyd::battle_event_cmd::btlevtcmd_SetHp(evt, isFirstCall);
    
    // Reset variable 1 to the action command level before exiting.
    evtSetValue(evt, evt->evtArguments[1], ac_level);
    return 2;
}

EVT_DEFINE_USER_FUNC(InfatuateChangeAlliance) {
    // Assumes that LW(3) / LW(4) contains the targeted battle unit / part's id.
    auto* battleWork = ttyd::battle::g_BattleWork;
    int32_t unit_idx = ttyd::battle_sub::BattleTransID(evt, evt->lwData[3]);
    int32_t part_idx = evt->lwData[4];
    auto* unit = ttyd::battle::BattleGetUnitPtr(battleWork, unit_idx);
    auto* part = ttyd::battle::BattleGetUnitPartsPtr(unit_idx, part_idx);
    
    // If not a boss enemy or Yux, undo Confusion status and change alliance.
    switch (unit->current_kind) {
        case BattleUnitType::BONETAIL:
        case BattleUnitType::ATOMIC_BOO:
        case BattleUnitType::YUX:
        case BattleUnitType::Z_YUX:
        case BattleUnitType::X_YUX:
        case BattleUnitType::MINI_YUX:
        case BattleUnitType::MINI_Z_YUX:
        case BattleUnitType::MINI_X_YUX:
            break;
            
        default: {
            uint32_t dummy = 0;
            ttyd::battle_damage::BattleSetStatusDamage(
                &dummy, unit, part, 0x100 /* ignore status vulnerability */,
                5 /* Confusion */, 100, 0, 0, 0);
            unit->alliance = 0;
            
            // Unqueue the status message for inflicting confusion.
            static constexpr const uint32_t kNoStatusMsg[] = {
                0xff000000U, 0, 0
            };
            memcpy(
                reinterpret_cast<void*>(
                    reinterpret_cast<uintptr_t>(battleWork) + 0x18ddc),
                kNoStatusMsg, sizeof(kNoStatusMsg));
            memcpy(
                reinterpret_cast<void*>(
                    reinterpret_cast<uintptr_t>(unit) + 0xae8),
                kNoStatusMsg, sizeof(kNoStatusMsg));
        }
    }
    
    // Call the function this user_func replaced with its original params.
    return ttyd::battle_event_cmd::btlevtcmd_AudienceDeclareACResult(
        evt, isFirstCall);
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
        item = PickRandomItem(RNG_KISS_THIEF, 20, 10, 10, 60);
        if (!item) item = ItemType::COIN;
    }
    if ((ac_result & 2) == 0 || item == ItemType::GOLD_BAR_X3 ||
        !ttyd::mario_pouch::pouchGetItem(item)) {
        // Action command unsuccessful, item = Shine Sprite (can't be stolen),
        // or the player's inventory cannot hold the item.
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
    
void ApplyFixedPatches() {
    // Tattle returns a custom message based on the enemy's stats.
    g_btlevtcmd_get_monosiri_msg_no_trampoline = mod::patch::hookFunction(
        ttyd::unit_party_christine::btlevtcmd_get_monosiri_msg_no,
        [](EvtEntry* evt, bool isFirstCall) {
            auto* battleWork = ttyd::battle::g_BattleWork;
            int32_t unit_idx = evtGetValue(evt, evt->evtArguments[0]);
            unit_idx = ttyd::battle_sub::BattleTransID(evt, unit_idx);
            auto* unit = ttyd::battle::BattleGetUnitPtr(battleWork, unit_idx);
            
            // Get original pointer to Tattle string.
            g_btlevtcmd_get_monosiri_msg_no_trampoline(evt, isFirstCall);
            const char* tattle_msg = 
                reinterpret_cast<const char*>(
                    evtGetValue(evt, evt->evtArguments[2]));
            // Build a custom tattle, if the enemy has stats to pull from.
            tattle_msg = SetCustomTattle(unit, tattle_msg);
            evtSetValue(evt, evt->evtArguments[2], PTR(tattle_msg));
            return 2;
        });
    // Tattle also has an increased Stylish multiplier, essentially making it
    // better than an Excellent + Stylish if successful.
    ttyd::unit_party_christine::partyWeapon_ChristineMonosiri.
        stylish_multiplier = 3;
        
    // Set Shell Shield's max HP to 3.
    ttyd::unit_koura::unit_koura.max_hp = 3;
    // Set Shell Shield's starting HP to 1 ~ 3 when spawned rather than 2 ~ 8.
    mod::patch::writePatch(
        reinterpret_cast<void*>(
            g_partyNokotarouAttack_KouraGuard_Patch_SetHpFunc),
        reinterpret_cast<int32_t>(ShellShieldSetInitialHp));
    // Set HP thresholds for different disrepair animation states...
    // - On initialization (pose_tbl_reset)
    mod::patch::writePatch(
        reinterpret_cast<void*>(g_koura_pose_tbl_reset_Patch_HeavyDmg), 1);
    mod::patch::writePatch(
        reinterpret_cast<void*>(g_koura_pose_tbl_reset_Patch_LightDmg), 2);
    // - On damage (damage_core)
    mod::patch::writePatch(
        reinterpret_cast<void*>(g_koura_damage_core_Patch_HeavyDmg), 1);
    mod::patch::writePatch(
        reinterpret_cast<void*>(g_koura_damage_core_Patch_LightDmg), 2);
    // Set FP cost to 5.
    ttyd::unit_party_nokotarou::partyWeapon_NokotarouKouraGuard.base_fp_cost = 5;
    
    // Disable getting coins and experience from a successful Gale Force.
    mod::patch::writePatch(
        reinterpret_cast<void*>(g_subsetevt_blow_dead_Patch_GetRewards),
        GaleForceKillPatch, sizeof(GaleForceKillPatch));
    // Remove the (Mario - enemy level) adjustment to Gale Force's chance.
    mod::patch::writePatch(
        reinterpret_cast<void*>(g_BattleSetStatusDamage_Patch_GaleLevelFactor),
        0x60000000U /* nop */);
        
    // Have Flurrie also apply Dodgy to herself at the end of Dodgy Fog event.
    mod::patch::writePatch(
        reinterpret_cast<void*>(g_partyClaudaAttack_KumoGuard_Patch_UseOnSelf),
        DodgyFogFlurriePatch, sizeof(DodgyFogFlurriePatch));
        
    // Make Infatuate single-target and slightly more expensive.
    ttyd::unit_party_vivian::partyWeapon_VivianCharmKissAttack.
        target_class_flags = 0x01101260;
    ttyd::unit_party_vivian::partyWeapon_VivianCharmKissAttack.base_fp_cost = 5;
    // Disable checking for the next enemy in the attack's event script.
    mod::patch::writePatch(
        reinterpret_cast<void*>(
            g_partyVivianAttack_CharmKissAttack_Patch_DisableNext),
        0x0002001aU /* IF_SMALL()... */);
    // Call a custom function on successfully hitting an enemy w/Infatuate
    // that makes the enemy permanently switch alliances (won't work on bosses).
    mod::patch::writePatch(
        reinterpret_cast<void*>(
            g_partyVivianAttack_CharmKissAttack_Patch_ResultFunc),
        reinterpret_cast<int32_t>(InfatuateChangeAlliance));
        
    // Replace Kiss Thief's item stealing routine with a custom one.
    mod::patch::writePatch(
        reinterpret_cast<void*>(
            g_partyChuchurinaAttack_ItemSteal_Patch_StealFunc),
        reinterpret_cast<int32_t>(GetKissThiefResult));
        
    // Increase Tease's base status rate to 1.27x.
    g__make_madowase_weapon_trampoline = mod::patch::hookFunction(
        ttyd::unit_party_chuchurina::_make_madowase_weapon,
        [](EvtEntry* evt, bool isFirstCall) {
            g__make_madowase_weapon_trampoline(evt, isFirstCall);
            reinterpret_cast<BattleWeapon*>(evt->lwData[12])->dizzy_chance
                *= 1.27;
            return 2;
        });

    ttyd::unit_party_chuchurina::partyWeapon_ChuchurinaKiss.base_fp_cost = 5;
    // Smooch parameter changes to make it restore 0 - 15 HP.
    // % of bar filled to gain 1 HP.
    mod::patch::writePatch(
        reinterpret_cast<void*>(
            g_partyChuchurinaAttack_Kiss_Patch_BarPercentPerHp), 7);
    // Disables having 1 HP base.
    mod::patch::writePatch(
        reinterpret_cast<void*>(
            g_partyChuchurinaAttack_Kiss_Patch_DisableBase1Hp), 1);
    // Bar fullness thresholds for color change.
    mod::patch::writePatch(
        reinterpret_cast<void*>(
            g_partyChuchurinaAttack_Kiss_Patch_BarColor1), 35);
    mod::patch::writePatch(
        reinterpret_cast<void*>(
            g_partyChuchurinaAttack_Kiss_Patch_BarColor2), 70);
    // HP thresholds for Action Command success.
    mod::patch::writePatch(
        reinterpret_cast<void*>(
            g_partyChuchurinaAttack_Kiss_Patch_AcLevel1), 5);
    mod::patch::writePatch(
        reinterpret_cast<void*>(
            g_partyChuchurinaAttack_Kiss_Patch_AcLevel2), 10);
    mod::patch::writePatch(
        reinterpret_cast<void*>(
            g_partyChuchurinaAttack_Kiss_Patch_AcLevel3), 15);
    
    // Misc. other party move balance changes.
    
    // Goombella's base attack does 2+2 at base rank instead of 1+1.
    ttyd::unit_party_christine::partyWeapon_ChristineNormalAttack.
        damage_function_params[0] = 2;
    ttyd::unit_party_christine::partyWeapon_ChristineNormalAttack.
        damage_function_params[1] = 2;
    // Koops, Bobbery, and Flurrie's base attacks do 3 / 5 / 7 damage.
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
        ttyd::unit_party_clauda::partyWeapon_ClaudaNormalAttack.
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
    // Lip Lock immune to fire and spikes (except pre-emptive spikes).
    ttyd::unit_party_clauda::partyWeapon_ClaudaLipLockAttack.
        counter_resistance_flags |= 0x1a;
    // Gulp's shot attack has a cascading effect, essentially acting like
    // the Super / Ultra Hammer moves; secondary hits' damage reduced to 2 ~ 4.
    ttyd::unit_party_yoshi::partyWeapon_YoshiNomikomi_Involved.
        damage_pattern = 6;  // "shot" effect on hit
    ttyd::unit_party_yoshi::partyWeapon_YoshiNomikomi_Involved.
        damage_function_params[1] = 2;
    ttyd::unit_party_yoshi::partyWeapon_YoshiNomikomi_Involved.
        damage_function_params[3] = 3;
    ttyd::unit_party_yoshi::partyWeapon_YoshiNomikomi_Involved.
        damage_function_params[5] = 4;
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
    // % of bar fullness for additional damage (Super, Ultra rank).
    mod::patch::writePatch(
        reinterpret_cast<void*>(
            g_partyChuchurinaAttack_NormalAttack_Patch_PercentPerDmgSuper), 34);
    mod::patch::writePatch(
        reinterpret_cast<void*>(
            g_partyChuchurinaAttack_NormalAttack_Patch_PercentPerDmgUltra), 20);
    // Bar fullness thresholds for color change.
    mod::patch::writePatch(
        reinterpret_cast<void*>(
            g_partyChuchurinaAttack_NormalAttack_Patch_BarColor1), 40);
    mod::patch::writePatch(
        reinterpret_cast<void*>(
            g_partyChuchurinaAttack_NormalAttack_Patch_BarColor2), 60);
    mod::patch::writePatch(
        reinterpret_cast<void*>(
            g_partyChuchurinaAttack_NormalAttack_Patch_BarColor3), 80);
    // Kiss Thief immune to all contact hazards and can target any height.
    ttyd::unit_party_chuchurina::partyWeapon_ChuchurinaItemSteal.
        counter_resistance_flags |= 0x7ff;
    ttyd::unit_party_chuchurina::partyWeapon_ChuchurinaItemSteal.
        target_property_flags &= ~0x2000;
    ttyd::unit_party_chuchurina::partyWeapon_ChuchurinaItemSteal.
        base_fp_cost = 5;

    // Prevent partner status buff moves from being used on Infatuated enemies.
    ttyd::unit_party_christine::partyWeapon_ChristineKiss.
        target_class_flags |= 0x10;
    ttyd::unit_party_chuchurina::partyWeapon_ChuchurinaKiss.
        target_class_flags |= 0x10;
    ttyd::unit_party_clauda::partyWeapon_ClaudaKumogakureAttack.
        target_class_flags |= 0x10;
    ttyd::unit_party_vivian::partyWeapon_VivianShadowGuard.
        target_class_flags |= 0x10;
}

EVT_DEFINE_USER_FUNC(InitializePartyMember) {
    const int32_t starting_rank =
        g_Mod->state_.GetOptionNumericValue(OPT_PARTNER_RANK);
    const int32_t idx = evtGetValue(evt, evt->evtArguments[0]);
    const int16_t starting_hp =
        ttyd::mario_pouch::_party_max_hp_table[idx * 4 + starting_rank];
    const int32_t hp_plus_p_cnt =
        ttyd::mario_pouch::pouchEquipCheckBadge(ItemType::HP_PLUS_P);
    auto& party_data = ttyd::mario_pouch::pouchGetPtr()->party_data[idx];
    party_data.base_max_hp = starting_hp;
    party_data.max_hp = starting_hp + hp_plus_p_cnt * 5;
    party_data.current_hp = starting_hp + hp_plus_p_cnt * 5;
    party_data.hp_level = starting_rank;
    party_data.attack_level = starting_rank;
    party_data.tech_level = starting_rank;
    return 2;
}

}  // namespace partner
}  // namespace mod::infinite_pit