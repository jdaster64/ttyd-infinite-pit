#include "randomizer_data.h"

#include "common_types.h"

#include <ttyd/battle_database_common.h>
#include <ttyd/npcdrv.h>
#include <ttyd/npc_event.h>

#include <cstdint>

namespace mod::pit_randomizer {

namespace {

using ::ttyd::npcdrv::NpcSetupInfo;
using namespace ::ttyd::battle_database_common;  // for convenience
using namespace ::ttyd::npc_event;               // for convenience


// TODO: Move any struct defs that need to be in the header to the header.
// It's possible that all of these structs can remain here.

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
    // The internal names used for the NPC kind and model.
    const char*     tribe_name;
    const char*     model_name;
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
    // Makes a type of audience member more likely to spawn (if nonzero).
    int32_t         audience_type_boosted;
};

// All data required to construct a particular enemy NPC in a particular module.
// In particular, contains the offset in the given module for an existing
// BattleUnitSetup* to use as a reference for 
struct EnemyModuleInfo {
    ModuleId::e module;
    int32_t     battle_unit_setup_offset;
    int32_t     npc_ent_type_info_idx;
    int32_t     enemy_type_stats_idx;
};

const NpcEntTypeInfo kNpcInfo[] = {
    
};

const EnemyTypeInfo kEnemyInfo[] = {
    
};

const EnemyModuleInfo kEnemyModuleInfo[] = {
    
};

// TODO: Declare globals for the constructed intermediate battle structures.

}

void BuildBattle(
    uintptr_t pit_module_ptr, int32_t floor,
    const char** out_tribe_name, const char** out_model_name,
    NpcSetupInfo* out_npc_setup_info, ModuleId::e* out_secondary_module) {
    // TODO: Implement, testing an enemy type at a time.
}

}