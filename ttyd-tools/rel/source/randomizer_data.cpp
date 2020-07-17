#include "randomizer_data.h"

#include "common_functions.h"
#include "common_types.h"

#include <ttyd/battle_database_common.h>
#include <ttyd/mariost.h>
#include <ttyd/npcdrv.h>
#include <ttyd/npc_data.h>
#include <ttyd/npc_event.h>

#include <cstdint>
#include <cstring>

namespace mod::pit_randomizer {

namespace {

using ::ttyd::npcdrv::NpcSetupInfo;
using ::ttyd::npcdrv::NpcTribeDescription;
using namespace ::ttyd::battle_database_common;  // for convenience
using namespace ::ttyd::npc_event;               // for convenience

// Events to run for a particular class of NPC (e.g. Goomba-like enemies).
struct NpcEntTypeInfo {
    int32_t* init_event;
    int32_t* regular_event;
    int32_t* dead_event;
    int32_t* find_event;
    int32_t* lost_event;
    int32_t* return_event;
    int32_t* blow_event;
};

// Stats for a particular kind of enemy (e.g. Hyper Goomba).
struct EnemyTypeInfo {
    int32_t         npc_tribe_idx;
    // How quickly the enemy's HP, ATK and DEF scale with the floor number.
    float           hp_scale;
    float           atk_scale;
    float           def_scale;
    // The reference point used as the enemy's "base" attack power; other
    // attacks will have the same difference in power as in the original game.
    // (e.g. a Hyper Goomba will charge by its attack power + 4).
    int32_t         atk_base;
    // The enemy's level will be this much higher than Mario's at base.
    int32_t         level_offset;
    // The enemy's HP and FP drop yields (this info isn't in BattleUnitSetup).
    PointDropData*  hp_drop_table;
    PointDropData*  fp_drop_table;
    // Makes a type of audience member more likely to spawn (-1 = none).
    int32_t         audience_type_boosted;
};

// All data required to construct a particular enemy NPC in a particular module.
// In particular, contains the offset in the given module for an existing
// BattleUnitSetup* to use as a reference for the constructed battle.
struct EnemyModuleInfo {
    BattleUnitType::e   unit_type;
    ModuleId::e         module;
    int32_t             battle_unit_setup_offset;
    int32_t             npc_ent_type_info_idx;
    int32_t             enemy_type_stats_idx;
};

PointDropData* kHpTables[] = {
    &battle_heart_drop_param_default, &battle_heart_drop_param_default2,
    &battle_heart_drop_param_default3, &battle_heart_drop_param_default4,
    &battle_heart_drop_param_default5
};
PointDropData* kFpTables[] = {
    &battle_flower_drop_param_default, &battle_flower_drop_param_default2,
    &battle_flower_drop_param_default3, &battle_flower_drop_param_default4,
    &battle_flower_drop_param_default5
};
const float kEnemyPartyCenterX = 90.0f;

const NpcEntTypeInfo kNpcInfo[] = {
    {
        // 0. Invalid npc type
    },
    {
        // 1. Goomba-like
        &kuriboo_init_event, &kuriboo_move_event, &enemy_common_dead_event,
        &kuriboo_find_event, &kuriboo_lost_event, &kuriboo_return_event,
        &enemy_common_blow_event
    }
};

const EnemyTypeInfo kEnemyInfo[] = {
    {
        // 0: Invalid enemy type
    },
    {
        // 1. Gloomba
        220, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1
    },
    {
        // 2. Hyper Goomba
        217, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1
    }
};

const EnemyModuleInfo kEnemyModuleInfo[] = {
    { BattleUnitType::GLOOMBA, ModuleId::JON, 0x15a20, 1, 1 },
    { BattleUnitType::HYPER_GOOMBA, ModuleId::GRA, 0x8690, 1, 2 }
};

// Global structures for holding constructed battle information.
int32_t g_NumEnemies = 3;
int32_t g_Enemies[5] = { 1, 1, 1, -1, -1 };
NpcSetupInfo g_CustomNpc[2];
BattleUnitSetup g_CustomUnits[6];
BattleGroupSetup g_CustomBattleParty;
// AudienceTypeWeights g_CustomAudienceWeights[16];

}

ModuleId::e SelectEnemies(int32_t floor) {
    // TODO: Procedurally pick a group of enemies based on the floor number;
    // currently hardcoded to test one enemy type, in a group of 3.
    for (int32_t i = 0; i < g_NumEnemies; ++i) {
        const EnemyModuleInfo* emi = kEnemyModuleInfo + g_Enemies[i];
        if (emi->module != ModuleId::JON) {
            return emi->module;
        }
    }
    return ModuleId::INVALID_MODULE;
}

void BuildBattle(
    uintptr_t pit_module_ptr, int32_t floor,
    NpcTribeDescription** out_npc_tribe_description,
    NpcSetupInfo** out_npc_setup_info) {

    const EnemyModuleInfo* enemy_module_info[5];
    const EnemyTypeInfo* enemy_info[5];
    const NpcEntTypeInfo* npc_info;
    const BattleUnitSetup* unit_info[5];
    
    for (int32_t i = 0; i < g_NumEnemies; ++i) {
        uintptr_t module_ptr = pit_module_ptr;
        enemy_module_info[i] = kEnemyModuleInfo + g_Enemies[i];
        enemy_info[i] = kEnemyInfo + enemy_module_info[i]->enemy_type_stats_idx;
        if (enemy_module_info[i]->module != ModuleId::JON) {
            module_ptr = reinterpret_cast<uintptr_t>(
                ttyd::mariost::g_MarioSt->pMapAlloc);
        }
        if (i == 0) {
            npc_info = kNpcInfo + enemy_module_info[i]->npc_ent_type_info_idx;
        }
        unit_info[i] = reinterpret_cast<const BattleUnitSetup*>(
            module_ptr + enemy_module_info[i]->battle_unit_setup_offset);
    }
    
    // Construct the data for the NPC on the field from the lead enemy's info.
    NpcTribeDescription* npc_tribe =
        &ttyd::npc_data::npcTribe + enemy_info[0]->npc_tribe_idx;
    g_CustomNpc[0].nameJp           = "\x93\x47";  // enemy
    g_CustomNpc[0].flags            = 0x1000000a;
    g_CustomNpc[0].reactionFlags    = 0;
    g_CustomNpc[0].initEvtCode      = npc_info->init_event;
    g_CustomNpc[0].regularEvtCode   = npc_info->regular_event;
    g_CustomNpc[0].talkEvtCode      = nullptr;
    g_CustomNpc[0].deadEvtCode      = npc_info->dead_event;
    g_CustomNpc[0].findEvtCode      = npc_info->find_event;
    g_CustomNpc[0].lostEvtCode      = npc_info->lost_event;
    g_CustomNpc[0].returnEvtCode    = npc_info->return_event;
    g_CustomNpc[0].blowEvtCode      = npc_info->blow_event;
    g_CustomNpc[0].territoryType    = ttyd::npcdrv::NpcTerritoryType::kSquare;
    g_CustomNpc[0].territoryBase    = { 0.0f, 0.0f, 0.0f };
    g_CustomNpc[0].territoryLoiter  = { 150.0f, 100.0f, 100.0f };
    g_CustomNpc[0].searchRange      = 200.0f;
    g_CustomNpc[0].searchAngle      = 300.0f;
    g_CustomNpc[0].homingRange      = 1000.0f;
    g_CustomNpc[0].homingAngle      = 360.0f;
    g_CustomNpc[0].battleInfoId     = 1;
    
    *out_npc_tribe_description = npc_tribe;
    *out_npc_setup_info = g_CustomNpc;
    
    // TODO: Generate custom audience weights.
    for (int32_t i = 0; i < g_NumEnemies; ++i) {
        // TODO: This won't work with secondary areas.
        BattleUnitSetup& custom_unit = g_CustomUnits[i];
        memcpy(&custom_unit, unit_info[i], sizeof(BattleUnitSetup));
        // TODO: Place enemies in the proper positions.
        custom_unit.position.x = kEnemyPartyCenterX + (i-1) * 40.0f;
    }
    g_CustomBattleParty.num_enemies         = g_NumEnemies;
    g_CustomBattleParty.enemy_data          = g_CustomUnits;
    g_CustomBattleParty.held_item_weight    = 100;
    g_CustomBattleParty.random_item_weight  = 0;
    g_CustomBattleParty.no_item_weight      = 0;
    g_CustomBattleParty.hp_drop_table       = enemy_info[0]->hp_drop_table;
    g_CustomBattleParty.fp_drop_table       = enemy_info[0]->fp_drop_table;
    
    // Make the current floor's battle point to the constructed party setup.
    int8_t* enemy_100 =
        reinterpret_cast<int8_t*>(pit_module_ptr + kPitEnemy100Offset);
    BattleSetupData* pit_battle_setups =
        reinterpret_cast<BattleSetupData*>(
            pit_module_ptr + kPitBattleSetupTblOffset);
    BattleSetupData* battle_setup = pit_battle_setups + enemy_100[floor % 100];
    battle_setup->flag_off_loadouts[0].group_data = &g_CustomBattleParty;
    battle_setup->flag_off_loadouts[1].weight = 0;
    battle_setup->flag_off_loadouts[1].group_data = nullptr;
    battle_setup->flag_off_loadouts[1].stage_data = nullptr;
    // If floor > 100, fix the background to always display the floor 80+ bg.
    if (floor > 100 && floor % 100 != 99) {
        battle_setup->flag_off_loadouts[0].stage_data =
            pit_battle_setups[50].flag_off_loadouts[0].stage_data;
    }
    // TODO: other battle setup data tweaks (audience makeup, etc.)?
}

}