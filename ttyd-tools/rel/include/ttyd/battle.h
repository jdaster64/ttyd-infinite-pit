#pragma once

#include <cstdint>

namespace ttyd::battle_database_common {
struct BattleWeapon;
}
namespace ttyd::battle_unit {
struct BattleWorkUnit;
}
namespace ttyd::npcdrv {
struct FbatBattleInformation;
}

namespace ttyd::battle {

struct BattleWorkCommandAction {
    uint32_t        type;
    uint32_t        enabled;
    const char*     description;
    int16_t         icon;
    int16_t         pad_0e;
    uint32_t        unk_10;  // has to do with greying out?
} __attribute__((__packed__));

static_assert(sizeof(BattleWorkCommandAction) == 0x14);

struct BattleWorkCommandWeapon {
    battle_database_common::BattleWeapon* weapon;
    uint32_t        unk_04;
    const char*     name;
    int16_t         icon;
    int16_t         pad_0e;
    uint32_t        unk_10;
    uint32_t        item_id;
    uint32_t        unk_18;
} __attribute__((__packed__));

static_assert(sizeof(BattleWorkCommandWeapon) == 0x1c);

struct BattleWorkCommandOperation {
    uint32_t        type;
    uint32_t        enabled;
    uint32_t        unk_08;  // has to do with greying out?
    const char*     name;
    int16_t         icon;
    int16_t         pad_12;
    const char*     help_message;
    int32_t         cost;
} __attribute__((__packed__));

static_assert(sizeof(BattleWorkCommandOperation) == 0x1c);

struct BattleWorkCommandParty {
    int32_t         party_unit_kind;
    uint32_t        enabled;
    const char*     name;
    int16_t         icon;
    int16_t         unk_0e;
    const char*     help_message;
    int16_t         current_hp;
    int16_t         max_hp;
} __attribute__((__packed__));

static_assert(sizeof(BattleWorkCommandParty) == 0x18);

struct BattleWorkCommandMultiItem {
    uint32_t        enabled;
    const char*     name;
    const char*     help_message;
    int16_t         icon;
    int16_t         unk_0e;
    int32_t         cost;
    uint32_t        unk_14;
} __attribute__((__packed__));

static_assert(sizeof(BattleWorkCommandMultiItem) == 0x18);

struct BattleWorkCommandCursor {
    int32_t         abs_position;
    int32_t         rel_position;
    int32_t         num_options;
} __attribute__((__packed__));

static_assert(sizeof(BattleWorkCommandCursor) == 0xc);
    
struct BattleWorkCommand {
    uint32_t        state;
    uint32_t        menu_type_active;
    BattleWorkCommandAction     act_class_table[6];
    BattleWorkCommandWeapon     weapon_table[21];
    BattleWorkCommandOperation  operation_table[7];
    BattleWorkCommandParty      party_table[8];
    BattleWorkCommandMultiItem  multi_item_table[3];
    BattleWorkCommandCursor     cursor_table[14];
    int8_t          unk_540[0x1c];
    void*           window_work;
    int8_t          unk_560[0x14];
} __attribute__((__packed__));

static_assert(sizeof(BattleWorkCommand) == 0x574);

struct BattleWork {
    int16_t         turn_count;
    int16_t         pad_00002;
    int32_t         battle_seq_0;
    int8_t          alliance_information[0x8 * 3];
    battle_unit::BattleWorkUnit* battle_units[64];
    int32_t         move_priority_queue[64];
    int32_t         phase_evt_queue[64][2];
    int32_t         active_unit_idx;
    int32_t         unknown_unit_idx;  // BattleTransID -6
    int8_t          weapon_targets_work[0xacc];
    uint32_t        battle_flags;
    uint32_t        unk_00ef8;  // flags
    uint32_t        unk_00efc;
    int32_t         stored_exp;
    int32_t         stored_exp_displayed;
    int32_t         stored_exp_displayed_inc_anim_timer;
    // BattleSeq sequence 1-7 values
    int32_t         init_seq;
    int32_t         first_act_seq;
    int32_t         turn_seq;
    int32_t         phase_seq;
    int32_t         move_seq;
    int32_t         act_seq;
    int32_t         end_seq;
    
    void*           battle_end_work;  // ptr to struct of length 0x2ac
    int8_t          pad_work[0x1fc * 4];
    BattleWorkCommand command_work;
    int8_t          ac_manager_work[0xaa8];
    npcdrv::FbatBattleInformation* fbat_info;
    int8_t          status_window_related[0x14];
    int8_t          unk_02750[4];
    int8_t          camera_work[0x104];
    int8_t          audience_work[0x13914];
    int8_t          bingo_work[0x134];
    int8_t          party_info_work[0x2c * 7];
    
    uint32_t        tattled_unit_type_flags[8];
    uint32_t        badge_equipped_flags;
    int8_t          unk_163f8[4];
    
    int8_t          stage_work[0xb3c];
    int8_t          act_record_work[0x24];
    int8_t          after_reaction_queue[0x8 * 64];
    int8_t          stage_object_work[0x7c * 32];
    int8_t          stage_hazard_work[0x1f0];
    int8_t          icon_work[0x9c * 16];
    int8_t          unk_18c8c[0x114];
    int8_t          status_change_msg_work[0x258];
    
    int8_t          unk_18ff8;
    int8_t          impending_merlee_spell_type;
    uint16_t        unk_18ffa;  // frame counter for something in btlseqFirstAct
    battle_database_common::BattleWeapon* impending_bonus_weapon;
    float           impending_sp_ac_success_multiplier;
    int8_t          impending_sp_stylish_multiplier;
    int8_t          unk_19005;
    int8_t          impending_sp_bingo_card_chance;
    int8_t          unk_19007;
    const char*     weapon_ac_help_msg;
    uint32_t        battle_ac_help_disp_type;
    
    int8_t          unk_19010[0x4c];
    int32_t         lucky_start_evt_tid;
    int32_t         reserve_items[4];
    int32_t         curtain_sfx_entry_idx;
    bool            last_ac_successful;
    int8_t          pad_19075[3];

    uint32_t        debug_event_trigger_flags;
    int8_t          unk_1907c[8];
    int32_t         debug_audience_count;
    int32_t         debug_audience_monotype_kind;
    int32_t         debug_force_bingo_slot_type;
    int8_t          unk_19090[8];
} __attribute__((__packed__));

static_assert(sizeof(BattleWork) == 0x19098);

extern "C" {

// .text
// BattleConsumeReserveItem
// BattleStatusWindowCheck
// BattleStatusWindowSystemOff
// BattleStatusWindowEventOff
// BattleStatusWindowSystemOn
// BattleStatusWindowEventOn
// BattleStatusWindowAPRecoveryOff
// BattleStatusWindowAPRecoveryOn
// BattleMajinaiEndCheck
// BattleMajinaiDone
// BattleMajinaiCheck
// battleDisableHResetCheck
// BattleAfterReactionMain
// BattleAfterReactionRelease
// BattleAfterReactionEntry
// BattleAfterReactionQueueInit
// BattleCheckUnitBroken
// BattleGetFloorHeight
// BattleGetStockExp
// BattleStoreExp
// BattleStoreCoin
// BattlePartyInfoWorkInit
// _EquipItem
// BtlUnit_EquipItem
// BattleTransPartyIdToUnitKind
// BattleTransPartyId
// BattleChangeParty
// BattlePartyAnimeLoad
// BattleGetPartnerPtr
// BattleGetPartyPtr
// BattleGetMarioPtr
// BattleGetSystemPtr
// BattleGetUnitPartsPtr
// BattleSetUnitPtr
battle_unit::BattleWorkUnit* BattleGetUnitPtr(
    BattleWork* battleWork, int32_t idx);
// BattleFree
// BattleAlloc
// BattleIncSeq
// BattleGetSeq
// BattleSetSeq
// BattleSetMarioParamToFieldBattle
// Btl_UnitSetup
// BattleEnd
// BattleMain
// BattleInit
// battleSeqEndCheck
// battleMain

 // .data
extern BattleWork* g_BattleWork; 

}

}