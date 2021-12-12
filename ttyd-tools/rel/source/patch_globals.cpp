#include "common_types.h"

#include <gc/OSLink.h>
#include <ttyd/battle.h>
#include <ttyd/battle_database_common.h>
#include <ttyd/battle_unit.h>
#include <ttyd/evtmgr.h>
#include <ttyd/npcdrv.h>
#include <ttyd/seqdrv.h>

#include <cstdint>

namespace mod::infinite_pit {
    
namespace {

using ::gc::OSLink::OSModuleInfo;
using ::ttyd::battle_database_common::BattleUnitSetup;
using ::ttyd::battle_database_common::BattleWeapon;
using ::ttyd::battle::BattleWork;
using ::ttyd::battle_unit::BattleWorkUnit;
using ::ttyd::battle_unit::BattleWorkUnitPart;
using ::ttyd::evtmgr::EvtEntry;
using ::ttyd::npcdrv::FbatBattleInformation;
using ::ttyd::seqdrv::SeqIndex;

}

// Function hooks.

// seqdrv.o  8002e218
void (*g_seqSetSeq_trampoline)(SeqIndex, const char*, const char*) = nullptr;
// npcdrv.o  80046a78
void (*g_fbatBattleMode_trampoline)(void) = nullptr;
// seq_battle.o  80073f20
void (*g_seq_battleInit_trampoline)(void) = nullptr;
// msgdrv.o  80081b8c
const char* (*g_msgSearch_trampoline)(const char*) = nullptr;
// cardmgr.o  800b2388
void (*g_cardCopy2Main_trampoline)(int32_t) = nullptr;
// mario_pouch.o  800d3300
int32_t (*g_pouchEquipCheckBadge_trampoline)(int16_t) = nullptr;
// mario_pouch.o  800d35a8
void (*g_pouchReviseMarioParam_trampoline)() = nullptr;
// mario_pouch.o 800d3e18
int32_t (*g_pouchAddCoin_trampoline)(int16_t) = nullptr;
// mario_pouch.o 800d50cc
uint32_t (*g_pouchGetItem_trampoline)(int32_t) = nullptr;
// pmario_sound.o  800daf24
uint32_t (*g_psndBGMOn_f_d_trampoline)(
    uint32_t, const char*, uint32_t, uint16_t) = nullptr;
// event.o  800ee688
void (*g_stg0_00_init_trampoline)(void) = nullptr;
// battle_ac.o  800fa1b8
int32_t (*g_BattleActionCommandCheckDefence_trampoline)(
    BattleWorkUnit*, BattleWeapon*) = nullptr;
// battle_damage.o  800fd030
void (*g__getSickStatusParam_trampoline)(
    BattleWorkUnit*, BattleWeapon*, int32_t, int8_t*, int8_t*) = nullptr;
// battle_damage.o  800fd680
int32_t (*g_BattleCalculateFpDamage_trampoline)(
    BattleWorkUnit*, BattleWorkUnit*, BattleWorkUnitPart*, BattleWeapon*,
    uint32_t*, uint32_t) = nullptr;
// battle_damage.o  800fd790
int32_t (*g_BattleCalculateDamage_trampoline)(
    BattleWorkUnit*, BattleWorkUnit*, BattleWorkUnitPart*, BattleWeapon*,
    uint32_t*, uint32_t) = nullptr;
// battle_damage.o 800fdfd4
void (*g_BattleDamageDirect_trampoline)(
    int32_t, BattleWorkUnit*, BattleWorkUnitPart*, int32_t, int32_t,
    uint32_t, uint32_t, uint32_t) = nullptr;
// battle_event_cmd.o  801054e8
int32_t (*g_btlevtcmd_WeaponAftereffect_trampoline)(EvtEntry*, bool) = nullptr;
// battle_event_cmd.o  801056f4
int32_t (*g_btlevtcmd_GetItemRecoverParam_trampoline)(EvtEntry*, bool) = nullptr;
// battle_event_cmd.o  8010af58
int32_t (*g_btlevtcmd_ConsumeItem_trampoline)(EvtEntry*, bool) = nullptr;
// battle_event_cmd.o  8010b3d4
int32_t (*g_btlevtcmd_GetConsumeItem_trampoline)(EvtEntry*, bool) = nullptr;
// battle_event_cmd.o  8010b540
int32_t (*g_btlevtcmd_GetSelectEnemy_trampoline)(EvtEntry*, bool) = nullptr;
// battle_event_cmd.o  8010ef44
int32_t (*g_btlevtcmd_CheckSpace_trampoline)(EvtEntry*, bool) = nullptr;
// battle_event_default.o  801138d8
int32_t (*g__get_flower_suitoru_point_trampoline)(EvtEntry*, bool) = nullptr;
// battle_event_default.o  80113950
int32_t (*g__get_heart_suitoru_point_trampoline)(EvtEntry*, bool) = nullptr;
// battle_information.o  801141bc
void (*g_BattleInformationSetDropMaterial_trampoline)(
    FbatBattleInformation*) = nullptr;
// battle_menu_disp.o 80115b30
void (*g_DrawOperationWin_trampoline)() = nullptr;
// battle_menu_disp.o 80115dac
void (*g_DrawWeaponWin_trampoline)() = nullptr;
// battle_seq.o  8011aa34
uint32_t (*g_BattleCheckConcluded_trampoline)(BattleWork*) = nullptr;
// battle_seq.o  8011c350
void (*g__rule_disp_trampoline)(void) = nullptr;
// battle_unit.o  80126840
void (*g_BtlUnit_PayWeaponCost_trampoline)(
    BattleWorkUnit*, BattleWeapon*) = nullptr;
// battle_unit.o  80126968
int32_t (*g_BtlUnit_GetWeaponCost_trampoline)(
    BattleWorkUnit*, BattleWeapon*) = nullptr;
// battle_unit.o  80128fe0
BattleWorkUnit* (*g_BtlUnit_Entry_trampoline)(BattleUnitSetup*) = nullptr;
// statuswindow.o  8013cb24
void (*g_statusWinDisp_trampoline)(void) = nullptr;
// statuswindow.o  8013d440
void (*g_gaugeDisp_trampoline)(double, double, int32_t) = nullptr;
// unit_party_chuchurina.o  80181bdc
int32_t (*g__make_madowase_weapon_trampoline)(EvtEntry*, bool) = nullptr;
// unit_party_christine.o  801895b0
int32_t (*g_btlevtcmd_get_monosiri_msg_no_trampoline)(EvtEntry*, bool) = nullptr;
// battle_actrecord.o  8018ef8c
void (*g_BtlActRec_JudgeRuleKeep_trampoline)(void) = nullptr;
// battle_actrecord.o 8018f990
void (*g_BtlActRec_AddCount_trampoline)(uint8_t*) = nullptr;
// battle_audience.o  801a5a0c
void (*g_BattleAudienceSetThrowItemMax_trampoline)() = nullptr;
// battle_enemy_item.o  801f9658
void* (*g_BattleEnemyUseItemCheck_trampoline)(BattleWorkUnit*) = nullptr;
// battle_seq_end.o  802166a4
const char* (*g_BattleGetRankNameLabel_trampoline)(int32_t) = nullptr;
// sac_bakugame.o  8022f990
int32_t (*g_bakuGameDecideWeapon_trampoline)(EvtEntry*, bool) = nullptr;
// sac_zubastar.o  80235c7c
uint32_t (*g_weaponGetPower_ZubaStar_trampoline)(
    BattleWorkUnit*, BattleWeapon*, BattleWorkUnit*, BattleWorkUnitPart*) = nullptr;
// sac_genki.o  80245a50
int32_t (*g_sac_genki_get_score_trampoline)(EvtEntry*, bool) = nullptr;
// sac_deka.o  80249048
int32_t (*g_weaponGetPower_Deka_trampoline)(
    BattleWorkUnit*, BattleWeapon*, BattleWorkUnit*, BattleWorkUnitPart*) = nullptr;
// sac_muki.o  8024ee50
int32_t (*g_main_muki_trampoline)(EvtEntry*, bool) = nullptr;
// sac_suki.o  8024fa74
int32_t (*g_sac_suki_set_weapon_trampoline)(EvtEntry*, bool) = nullptr;
// os.a OSLink.c  8029a8e4
bool (*g_OSLink_trampoline)(OSModuleInfo*, void*) = nullptr;

// Branch / patch addresses (BH / EH / CH# = begin / end / conditional hooks).

extern const int32_t g_seq_mapChangeMain_OnMapUnload_BH = 0x80007e0c;
extern const int32_t g_seq_mapChangeMain_MapLoad_BH = 0x80007ef0;
extern const int32_t g_seq_mapChangeMain_MapLoad_EH = 0x80008148;
extern const int32_t g_titleInit_Patch_EnableCrashHandler = 0x80009b2c;
extern const int32_t g_fbatBattleMode_Patch_DoubleCoinsBadge1 = 0x80046f70;
extern const int32_t g_fbatBattleMode_Patch_DoubleCoinsBadge2 = 0x80046f80;
extern const int32_t g_fbatBattleMode_GivePlayerInvuln_BH = 0x8004706c;
extern const int32_t g_fbatBattleMode_GivePlayerInvuln_EH = 0x800470c8;
extern const int32_t g_msgWindow_Entry_Patch_FixAllocSize = 0x800816f4;
extern const int32_t g_itemseq_GetItem_Patch_SkipTutorials = 0x800abcd8;
extern const int32_t g_pouchInit_FixAllocLeak_BH = 0x800d59dc;
extern const int32_t g_continueGame_Patch_SkipZeroingGswfs = 0x800f3ecc;
extern const int32_t g_loadMain_Patch_SkipZeroingGswfs = 0x800f6358;
extern const int32_t g_BattleCheckDamage_Patch_PaybackDivisor = 0x800fb7dc;
extern const int32_t g_BattleCheckDamage_Patch_HoldFastDivisor = 0x800fb800;
extern const int32_t g_BattleCheckDamage_Patch_ReturnPostageDivisor = 0x800fb824;
extern const int32_t g_BattlePreCheckDamage_CheckEvasion_BH = 0x800fbbec;
extern const int32_t g_BattlePreCheckDamage_CheckEvasion_CH1 = 0x800fbc8c;
extern const int32_t g_BattlePreCheckDamage_CheckEvasion_EH = 0x800fbca0;
extern const int32_t g_BattleSetStatusDamage_Patch_GaleLevelFactor = 0x800fc0a8;
extern const int32_t g_BattleCalculateDamage_MegaRushStrength_BH = 0x800fd91c;
extern const int32_t g_BattleCalculateDamage_PowerRushStrength_BH = 0x800fd93c;
extern const int32_t g__getSickStatusParam_Patch_CheckChargeCap = 0x800fd468;
extern const int32_t g__getSickStatusParam_Patch_SetChargeCap = 0x800fd470;
extern const int32_t g_BattleDamageDirect_Patch_AddTotalDamage = 0x800fe058;
extern const int32_t g_BattleDamageDirect_Patch_PityFlowerChance = 0x800fe500;
extern const int32_t g_BattleChoiceSamplingEnemy_SumRandWeights_BH = 0x800ff528;
extern const int32_t g_BattleChoiceSamplingEnemy_SumRandWeights_EH = 0x800ff544;
extern const int32_t g_btlDispMain_DrawNormalHeldItem_BH = 0x80102ff4;
extern const int32_t g_btlevtcmd_ConsumeItemReserve_Patch_RefundPer = 0x8010ae84;
extern const int32_t g_btlevtcmd_ConsumeItemReserve_Patch_RefundBase = 0x8010aea0;
extern const int32_t g_btlevtcmd_ConsumeItem_Patch_RefundPer = 0x8010affc;
extern const int32_t g_btlevtcmd_ConsumeItem_Patch_RefundBase = 0x8010b018;
extern const int32_t g_btlevtcmd_CheckSpace_Patch_CheckEnemyTypes = 0x8010efdc;
extern const int32_t g_DrawMenuPartyPinchMark_CheckThreshold_BH = 0x80117e90;
extern const int32_t g_DrawMenuPartyPinchMark_Patch_CheckResult1 = 0x80117e98;
extern const int32_t g_DrawMenuPartyPinchMark_Patch_CheckResult2 = 0x80117e9c;
extern const int32_t g_DrawMenuMarioPinchMark_CheckThreshold_BH = 0x80118074;
extern const int32_t g_DrawMenuMarioPinchMark_Patch_CheckResult1 = 0x8011807c;
extern const int32_t g_DrawMenuMarioPinchMark_Patch_CheckResult2 = 0x80118080;
extern const int32_t g_btlseqTurn_Patch_RuleDispShowLonger = 0x8011c5ec;
extern const int32_t g_btlseqTurn_Patch_RuleDispDismissOnlyWithB = 0x8011c62c;
extern const int32_t g_btlseqTurn_Patch_HappyHeartReductionAtMax = 0x8011dee4;
extern const int32_t g_btlseqTurn_Patch_HappyHeartBaseRate = 0x8011dee8;
extern const int32_t g_btlseqTurn_Patch_HappyFlowerReductionAtMax = 0x8011e09c;
extern const int32_t g_btlseqTurn_Patch_HappyFlowerBaseRate = 0x8011e0a0;
extern const int32_t g_BattleDrawEnemyHP_DrawEnemyHPText_BH = 0x8011ffcc;
extern const int32_t g__btlcmd_SetAttackEvent_SwitchPartnerCost_BH = 0x801204c8;
extern const int32_t g__btlcmd_MakeOperationTable_AppealAlways_BH = 0x801239e4;
extern const int32_t g__btlcmd_MakeSelectWeaponTable_Patch_GetNameFromItem = 0x80124924;
extern const int32_t g_BtlUnit_EnemyItemCanUseCheck_Patch_SkipCheck = 0x80125d54;
extern const int32_t g_BtlUnit_CheckPinchStatus_PerilThreshold_BH = 0x80126e20;
extern const int32_t g_BtlUnit_CheckPinchStatus_DangerThreshold_BH = 0x80126e84;
extern const int32_t g_statusWinDisp_HideDpadMenuOutsidePit_BH = 0x8013d140;
extern const int32_t g_statusWinDisp_HideDpadMenuOutsidePit_EH = 0x8013d144;
extern const int32_t g_statusWinDisp_HideDpadMenuOutsidePit_CH1 = 0x8013d404;
extern const int32_t g_itemUseDisp_FixPartyOrder_BH = 0x80169f40;
extern const int32_t g_itemUseDisp_FixPartyOrder_EH = 0x8016a088;
extern const int32_t g_winItemMain_CheckInvalidTarget_BH = 0x8016cd74;
extern const int32_t g_winItemMain_CheckInvalidTarget_EH = 0x8016cd78;
extern const int32_t g_winItemMain_Patch_AlwaysUseItemsInMenu = 0x8016ce40;
extern const int32_t g_winItemMain_FixPartyOrder_BH = 0x8016ce88;
extern const int32_t g_winItemMain_FixPartyOrder_EH = 0x8016cfd0;
extern const int32_t g_winItemMain_UseSpecialItems_BH = 0x8016cfd0;
extern const int32_t g_winItemMain_CheckInvalidTarget_CH1 = 0x8016d1a4;
extern const int32_t g_winMarioDisp_StarPowerMenu_BH = 0x8016f584;
extern const int32_t g_winMarioDisp_StarPowerMenu_EH = 0x8016f7f8;
extern const int32_t g_winMarioMain_StarPowerDescription_BH = 0x80170c38;
extern const int32_t g_winMarioMain_StarPowerDescription_EH = 0x80170c44;
extern const int32_t g_winLogInit_Patch_DisableCrystalStarLog = 0x80176e0c;
extern const int32_t g_winLogInit_InitTattleLog_BH = 0x80177940;
extern const int32_t g_BattleAudienceAddPuni_EnableAlways_BH = 0x801a15c8;
extern const int32_t g_BattleAudienceAddAudienceNum_EnableAlways_BH = 0x801a1734;
extern const int32_t g_BattleAudienceItemCtrlProcess_Patch_CheckItemValidRange = 0x801a5418;
extern const int32_t g_BattleAudienceItemCtrlProcess_CheckSpace_BH = 0x801a5424;
extern const int32_t g_BattleAudienceDetectTargetPlayer_CheckPlayer_BH = 0x801a58b0;
extern const int32_t g_BattleAudienceDetectTargetPlayer_CheckPlayer_EH = 0x801a58b8;
extern const int32_t g_BattleAudienceItemOn_RandomItem_BH = 0x801a5c70;
extern const int32_t g_BattleAudienceSettingAudience_EnableAlways_BH = 0x801a61ac;
extern const int32_t g__object_fall_attack_AudienceEnableAlways_BH = 0x801469e4;
extern const int32_t g_effUpdownDisp_TwoDigitSupport_BH = 0x80193aec;
extern const int32_t g_effUpdownDisp_TwoDigitSupport_EH = 0x80193cd4;
extern const int32_t g_BattleAudience_ApRecoveryBuild_BingoRegen_BH = 0x801a1ef4;
extern const int32_t g_BattleAudience_ApRecoveryBuild_NoBingoRegen_BH = 0x801a1f30;
extern const int32_t g_BattleAudience_SetTargetAmount_BH = 0x801a61ec;
extern const int32_t g_BattleAudience_End_SaveAmountAlways_BH = 0x801a6b68;
extern const int32_t g_BattleAudience_Disp_EnableAlways_BH = 0x801a6cb0;
extern const int32_t g_getBadgeBottakuru100TableMaxCount_Patch_ShopSize = 0x801fae60;
extern const int32_t g_BattleBreakSlot_PointInc_EnableAlways_BH = 0x802034b4;
extern const int32_t g_btlseqEnd_JudgeRuleEarly_BH = 0x80215348;
extern const int32_t g_btlseqEnd_Patch_RemoveJudgeRule = 0x80216678;
extern const int32_t g_scissor_damage_sub_ArtAttackDamage_BH = 0x80231cc0;
extern const int32_t g_scissor_damage_sub_ArtAttackDamage_EH = 0x80231d38;
extern const int32_t g_select_disp_Patch_PitListPriceHook = 0x8023c120;
extern const int32_t g_select_disp_Patch_PitItemPriceHook = 0x8023d2e0;
extern const int32_t g_sac_genki_main_base_BlinkNumbers_BH = 0x80248220;
extern const int32_t g_sac_genki_main_base_BlinkNumbers_EH = 0x802483b4;
extern const int32_t g_sac_genki_main_base_SetupTargets_BH = 0x80248430;
extern const int32_t g_sac_genki_main_base_SetupTargets_EH = 0x8024864c;
extern const int32_t g_sac_deka_main_base_GetNumberOfBars_BH = 0x8024b0b8;
extern const int32_t g_crashHandler_Patch_LoopForever1 = 0x8025e4a4;
extern const int32_t g_crashHandler_Patch_LoopForever2 = 0x8025e4a8;
extern const int32_t g_subsetevt_blow_dead_Patch_GetRewards = 0x80351ea4;
extern const int32_t g_marioAttackEvent_JyabaraJump_Patch_GetSp1 = 0x803589b4;
extern const int32_t g_marioAttackEvent_JyabaraJump_Patch_GetSp2 = 0x80359010;
extern const int32_t g_marioAttackEvent_JyabaraJump_Patch_PrizeTier1 = 0x80359178;
extern const int32_t g_marioAttackEvent_JyabaraJump_Patch_PrizeTier2 = 0x80359484;
extern const int32_t g_marioAttackEvent_JyabaraJump_Patch_GetSp3 = 0x80359558;
extern const int32_t g_marioAttackEvent_JyabaraJump_Patch_GetSp4 = 0x803596d8;
extern const int32_t g_marioAttackEvent_JyabaraJump_Patch_ResetFace = 0x80359940;
extern const int32_t g_ItemEvent_Support_NoEffect_TradeOffJumpPoint = 0x803652b8;
extern const int32_t g_ItemEvent_Teki_Kyouka_ApplyStatusHook = 0x80369b34;
extern const int32_t g_ItemEvent_Poison_Kinoko_PoisonChance = 0x8036c914;
extern const int32_t g_ItemEvent_LastDinner_Weapon = 0x8036caf4;
extern const int32_t g_partyClaudaAttack_KumoGuard_Patch_UseOnSelf = 0x8037ba04;
extern const int32_t g_partyChuchurinaAttack_NormalAttack_Patch_PercentPerDmgSuper = 0x803825ac;
extern const int32_t g_partyChuchurinaAttack_NormalAttack_Patch_PercentPerDmgUltra = 0x80382558;
extern const int32_t g_partyChuchurinaAttack_NormalAttack_Patch_BarColor1 = 0x8038256c;
extern const int32_t g_partyChuchurinaAttack_NormalAttack_Patch_BarColor2 = 0x80382570;
extern const int32_t g_partyChuchurinaAttack_NormalAttack_Patch_BarColor3 = 0x80382574;
extern const int32_t g_partyChuchurinaAttack_NormalAttack_Patch_NumSlaps3 = 0x8038280c;
extern const int32_t g_partyChuchurinaAttack_NormalAttack_Patch_NumSlaps2 = 0x80382828;
extern const int32_t g_partyChuchurinaAttack_NormalAttack_Patch_NumSlaps1 = 0x80382840;
extern const int32_t g_partyChuchurinaAttack_NormalAttack_Patch_GetSp1 = 0x80382b84;
extern const int32_t g_partyChuchurinaAttack_NormalAttack_Patch_GetPrizeTier = 0x80383034;
extern const int32_t g_partyChuchurinaAttack_NormalAttack_Patch_GetSp2 = 0x803835f8;
extern const int32_t g_partyChuchurinaAttack_NormalAttack_Patch_GetSp3 = 0x8038365c;
extern const int32_t g_partyChuchurinaAttack_NormalAttack_Patch_ResetFace = 0x80383964;
extern const int32_t g_partyChuchurinaAttack_ItemSteal_Patch_StealFunc = 0x80386258;
extern const int32_t g_partyChuchurinaAttack_Kiss_Patch_BarPercentPerHp = 0x80386ae4;
extern const int32_t g_partyChuchurinaAttack_Kiss_Patch_BarColor1 = 0x80386af8;
extern const int32_t g_partyChuchurinaAttack_Kiss_Patch_BarColor2 = 0x80386afc;
extern const int32_t g_partyChuchurinaAttack_Kiss_Patch_DisableBase1Hp = 0x80386b10;
extern const int32_t g_partyChuchurinaAttack_Kiss_Patch_AcLevel3 = 0x80386c4c;
extern const int32_t g_partyChuchurinaAttack_Kiss_Patch_AcLevel2 = 0x80386c60;
extern const int32_t g_partyChuchurinaAttack_Kiss_Patch_AcLevel1 = 0x80386c74;
extern const int32_t g_partyVivianAttack_CharmKissAttack_Patch_DisableNext = 0x8038df34;
extern const int32_t g_partyVivianAttack_CharmKissAttack_Patch_ResultFunc = 0x8038de50;
extern const int32_t g_partyNokotarouAttack_KouraGuard_Patch_SetHpFunc = 0x80392238;
extern const int32_t g_koura_damage_core_Patch_HeavyDmg = 0x8039c190;
extern const int32_t g_koura_damage_core_Patch_LightDmg = 0x8039c1ec;
extern const int32_t g_koura_pose_tbl_reset_Patch_HeavyDmg = 0x8039c2ec;
extern const int32_t g_koura_pose_tbl_reset_Patch_LightDmg = 0x8039c338;
extern const int32_t g_genki_evt_common_Patch_SweetTreatFeastResult = 0x803b6bac;
extern const int32_t g_genki_evt_common_SweetTreatResultJumpPoint = 0x803b6be8;
extern const int32_t g_crashHandler_Patch_FontScale = 0x80428bc0;

// Patch offsets for data in modules (relative to REL base pointer).

// JON (Pit of 100 Trials)
extern const int32_t g_jon_setup_npc_ex_para_FuncOffset = 0x388;
extern const int32_t g_jon_yattukeFlag_FuncOffset = 0x3e8;
extern const int32_t g_jon_init_evt_MoverSetupHookOffset = 0xeaa0;
extern const int32_t g_jon_enemy_100_Offset = 0xef90;
extern const int32_t g_jon_enemy_setup_EvtOffset = 0x102a4;
extern const int32_t g_jon_evt_open_box_EvtOffset = 0x11348;
extern const int32_t g_jon_move_evt_EvtOffset = 0x11400;
extern const int32_t g_jon_talk_gyousyou_MinItemForBadgeDialogOffset = 0x11b1c;
extern const int32_t g_jon_talk_gyousyou_NoInvSpaceBranchOffset = 0x11c7c;
extern const int32_t g_jon_gyousyou_setup_CharlietonSpawnChanceOffset = 0x11ea4;
extern const int32_t g_jon_dokan_open_PipeOpenEvtOffset = 0x120d0;
extern const int32_t g_jon_evt_kanban2_ReturnSignEvtOffset = 0x12374;
extern const int32_t g_jon_floor_inc_EvtOffset = 0x123c4;
extern const int32_t g_jon_bero_boss_EntryBeroEntryOffset = 0x1240c;
extern const int32_t g_jon_bero_boss_ReturnBeroEntryOffset = 0x12448;
extern const int32_t g_jon_setup_boss_EvtOffset = 0x124c0;
extern const int32_t g_jon_bero_return_ReturnBeroEntryOffset = 0x12520;
extern const int32_t g_jon_zonbaba_first_event_EvtOffset = 0x14570;
extern const int32_t g_jon_btlsetup_jon_tbl_Offset = 0x1d460;
extern const int32_t g_jon_unit_boss_zonbaba_battle_entry_event = 0x1ebdc;
extern const int32_t g_jon_BanditAttackEvt_CheckConfusionOffset = 0x2ffa8;
extern const int32_t g_jon_DarkKoopatrolAttackEvt_NormalAttackReturnLblOffset = 0x51790;
extern const int32_t g_jon_DarkWizzerdAttackEvt_CheckEnemyNumOffset = 0x535a4;
extern const int32_t g_jon_DarkWizzerdGaleForceDeathEvt_PatchOffset = 0x544e4;
extern const int32_t g_jon_EliteWizzerdAttackEvt_CheckEnemyNumOffset = 0x5cecc;
extern const int32_t g_jon_EliteWizzerdGaleForceDeathEvt_PatchOffset = 0x5de0c;
extern const int32_t g_jon_BadgeBanditAttackEvt_CheckConfusionOffset = 0x73e40;
extern const int32_t g_jon_PiderGaleForceDeathEvt_PatchOffset = 0x760e4;
extern const int32_t g_jon_ArantulaGaleForceDeathEvt_PatchOffset = 0x79ec4;

// TIK (Rogueport Underground)
extern const int32_t g_tik_06_PitBeroEntryOffset = 0x1f240;
extern const int32_t g_tik_06_RightBeroEntryOffset = 0x1f2f4;

// CUSTOM
extern const int32_t g_custom_XNautAttackEvt_NormalAttackReturnLblOffset = 0x14f0c;
extern const int32_t g_custom_XNautAttackEvt_JumpAttackReturnLblOffset = 0x153a4;
extern const int32_t g_custom_ZYux_PrimaryKindPartOffset = 0x160a4;
extern const int32_t g_custom_EliteXNautAttackEvt_NormalAttackReturnLblOffset = 0x19b5c;
extern const int32_t g_custom_EliteXNautAttackEvt_JumpAttackReturnLblOffset = 0x19ff4;
extern const int32_t g_custom_XYux_PrimaryKindPartOffset = 0x1cb64;
extern const int32_t g_custom_Yux_PrimaryKindPartOffset = 0x201c4;
extern const int32_t g_custom_MagikoopaGaleForceDeathEvt_PatchOffset = 0x40f98;
extern const int32_t g_custom_MagikoopaAttackEvt_CheckEnemyNumOffset = 0x4142c;
extern const int32_t g_custom_KoopatrolAttackEvt_NormalAttackReturnLblOffset = 0x469d8;
extern const int32_t g_custom_BoomerangBrosAttackEvt_CheckHpOffset = 0x4ace4;
extern const int32_t g_custom_FireBrosAttackEvt_CheckHpOffset = 0x4e86c;
extern const int32_t g_custom_HammerBrosAttackEvt_CheckHpOffset = 0x508dc;
extern const int32_t g_custom_GrnMagikoopa_DefenseOffset = 0x59b70;
extern const int32_t g_custom_GrnMagikoopaGaleForceDeathEvt_PatchOffset = 0x5ad00;
extern const int32_t g_custom_GrnMagikoopaAttackEvt_CheckEnemyNumOffset = 0x5b194;
extern const int32_t g_custom_RedMagikoopaGaleForceDeathEvt_PatchOffset = 0x5e628;
extern const int32_t g_custom_RedMagikoopaAttackEvt_CheckEnemyNumOffset = 0x5eabc;
extern const int32_t g_custom_WhtMagikoopaGaleForceDeathEvt_PatchOffset = 0x61f50;
extern const int32_t g_custom_WhtMagikoopaAttackEvt_CheckEnemyNumOffset = 0x623e4;
extern const int32_t g_custom_BigBanditAttackEvt_CheckConfusionOffset = 0x64a18;

}