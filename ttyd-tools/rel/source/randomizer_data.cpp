#include "randomizer_data.h"

#include "common_functions.h"
#include "common_types.h"
#include "randomizer.h"
#include "randomizer_state.h"

#include <ttyd/battle_actrecord.h>
#include <ttyd/battle_database_common.h>
#include <ttyd/item_data.h>
#include <ttyd/mario_pouch.h>
#include <ttyd/mariost.h>
#include <ttyd/npcdrv.h>
#include <ttyd/npc_data.h>
#include <ttyd/npc_event.h>
#include <ttyd/system.h>

#include <cinttypes>
#include <cstdio>
#include <cstring>

namespace mod::pit_randomizer {

namespace {

using ::ttyd::mario_pouch::PouchData;
using ::ttyd::npcdrv::NpcSetupInfo;
using ::ttyd::npcdrv::NpcTribeDescription;
using namespace ::ttyd::battle_database_common;  // for convenience
using namespace ::ttyd::npc_event;               // for convenience

namespace ItemType = ::ttyd::item_data::ItemType;

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
    BattleUnitType::e unit_type;
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
    { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr },
    { &kuriboo_init_event, &kuriboo_move_event, &enemy_common_dead_event, &kuriboo_find_event, &kuriboo_lost_event, &kuriboo_return_event, &enemy_common_blow_event },
    { &patakuri_init_event, &patakuri_move_event, &enemy_common_dead_event, &patakuri_find_event, &patakuri_lost_event, &patakuri_return_event, &enemy_common_blow_event },
    { &nokonoko_init_event, &nokonoko_move_event, &enemy_common_dead_event, &nokonoko_find_event, &nokonoko_lost_event, &nokonoko_return_event, &enemy_common_blow_event },
    { &togenoko_init_event, &togenoko_move_event, &enemy_common_dead_event, &togenoko_find_event, &togenoko_lost_event, &togenoko_return_event, &enemy_common_blow_event },
    { &patapata_init_event, &patapata_move_event, &enemy_common_dead_event, &patapata_find_event, &patapata_lost_event, &patapata_return_event, &enemy_common_blow_event },
    { &met_init_event, &met_move_event, &enemy_common_dead_event, &met_find_event, &met_lost_event, &met_return_event, &enemy_common_blow_event },
    { &patamet_init_event, &patamet_move_event, &enemy_common_dead_event, &patamet_find_event, &patamet_lost_event, &patamet_return_event, &enemy_common_blow_event },
    { &chorobon_init_event, &chorobon_move_event, &enemy_common_dead_event, &chorobon_find_event, &chorobon_lost_event, &chorobon_return_event, &enemy_common_blow_event },
    { &pansy_init_event, &pansy_move_event, &enemy_common_dead_event, &pansy_find_event, &pansy_lost_event, &pansy_return_event, &enemy_common_blow_event },
    { &twinkling_pansy_init_event, &twinkling_pansy_move_event, &enemy_common_dead_event, &twinkling_pansy_find_event, &twinkling_pansy_lost_event, &twinkling_pansy_return_event, &enemy_common_blow_event },
    { &karon_init_event, &karon_move_event, &enemy_common_dead_event, &karon_find_event, &karon_lost_event, &karon_return_event, &karon_blow_event },
    { &honenoko2_init_event, &honenoko2_move_event, &enemy_common_dead_event, &honenoko2_find_event, &honenoko2_lost_event, &honenoko2_return_event, &enemy_common_blow_event },
    { &killer_init_event, &killer_move_event, &killer_dead_event, nullptr, nullptr, nullptr, &enemy_common_blow_event },
    { &killer_cannon_init_event, &killer_cannon_move_event, &enemy_common_dead_event, nullptr, nullptr, nullptr, &enemy_common_blow_event },
    { &sambo_init_event, &sambo_move_event, &enemy_common_dead_event, &sambo_find_event, &sambo_lost_event, &sambo_return_event, &enemy_common_blow_event },
    { &sinnosuke_init_event, &sinnosuke_move_event, &enemy_common_dead_event, &sinnosuke_find_event, &sinnosuke_lost_event, &sinnosuke_return_event, &enemy_common_blow_event },
    { &sinemon_init_event, &sinemon_move_event, &enemy_common_dead_event, &sinemon_find_event, &sinemon_lost_event, &sinemon_return_event, &enemy_common_blow_event },
    { &togedaruma_init_event, &togedaruma_move_event, &enemy_common_dead_event, &togedaruma_find_event, &togedaruma_lost_event, &togedaruma_return_event, &enemy_common_blow_event },
    { &barriern_init_event, &barriern_move_event, &barriern_dead_event, &barriern_find_event, &barriern_lost_event, &barriern_return_event, &barriern_blow_event },
    { &piders_init_event, &piders_move_event, &enemy_common_dead_event, &piders_find_event, nullptr, nullptr, &piders_blow_event },
    { &pakkun_init_event, &pakkun_move_event, &enemy_common_dead_event, &pakkun_find_event, nullptr, &pakkun_return_event, &enemy_common_blow_event },
    { &dokugassun_init_event, &dokugassun_move_event, &enemy_common_dead_event, &dokugassun_find_event, &dokugassun_lost_event, &dokugassun_return_event, &enemy_common_blow_event },
    { &basabasa2_init_event, &basabasa2_move_event, &basabasa2_dead_event, &basabasa2_find_event, &basabasa2_lost_event, &basabasa2_return_event, &enemy_common_blow_event },
    { &teresa_init_event, &teresa_move_event, &enemy_common_dead_event, &teresa_find_event, &teresa_lost_event, &teresa_return_event, &enemy_common_blow_event },
    { &bubble_init_event, &bubble_move_event, &enemy_common_dead_event, &bubble_find_event, &bubble_lost_event, &bubble_return_event, &enemy_common_blow_event },
    { &hbom_init_event, &hbom_move_event, &enemy_common_dead_event, &hbom_find_event, &hbom_lost_event, &hbom_return_event, &enemy_common_blow_event },
    { &zakowiz_init_event, &zakowiz_move_event, &zakowiz_dead_event, &zakowiz_find_event, &zakowiz_lost_event, &zakowiz_return_event, &zakowiz_blow_event },
    { &hannya_init_event, &hannya_move_event, &enemy_common_dead_event, &hannya_find_event, &hannya_lost_event, &hannya_return_event, &enemy_common_blow_event },
    { &mahoon_init_event, &mahoon_move_event, &mahoon_dead_event, &mahoon_find_event, &mahoon_lost_event, &mahoon_return_event, &enemy_common_blow_event },
    { &kamec_init_event, &kamec_move_event, &kamec_dead_event, &kamec_find_event, &kamec_lost_event, &kamec_return_event, &kamec_blow_event },
    { &kamec2_init_event, &kamec2_move_event, &kamec2_dead_event, &kamec2_find_event, &kamec2_lost_event, &kamec2_return_event, &kamec2_blow_event },
    { &hbross_init_event, &hbross_move_event, &hbross_dead_event, &hbross_find_event, &hbross_lost_event, &hbross_return_event, &hbross_blow_event },
    { &wanwan_init_event, &wanwan_move_event, &enemy_common_dead_event, &wanwan_find_event, nullptr, nullptr, &enemy_common_blow_event },
    { nullptr, nullptr, &enemy_common_dead_event, nullptr, nullptr, nullptr, nullptr },
};

const EnemyTypeInfo kEnemyInfo[] = {
    { BattleUnitType::BONETAIL, 325, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::ATOMIC_BOO, 148, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::BANDIT, 274, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::BIG_BANDIT, 129, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::BADGE_BANDIT, 275, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::BILL_BLASTER, 254, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::BOMBSHELL_BILL_BLASTER, 256, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::BULLET_BILL, 255, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::BOMBSHELL_BILL, 257, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::BOB_OMB, 283, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::BULKY_BOB_OMB, 304, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::BOB_ULK, 305, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::DULL_BONES, 39, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::RED_BONES, 36, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::DRY_BONES, 196, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::DARK_BONES, 197, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::BOO, 146, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::DARK_BOO, 147, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::BRISTLE, 258, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::DARK_BRISTLE, 259, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::HAMMER_BRO, 206, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::BOOMERANG_BRO, 294, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::FIRE_BRO, 293, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::LAVA_BUBBLE, 302, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::EMBER, 159, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::PHANTOM_EMBER, 303, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::BUZZY_BEETLE, 225, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::SPIKE_TOP, 226, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::PARABUZZY, 228, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::SPIKY_PARABUZZY, 227, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::RED_SPIKY_BUZZY, 230, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::CHAIN_CHOMP, 301, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::RED_CHOMP, 306, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::CLEFT, 237, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::HYPER_CLEFT, 236, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::MOON_CLEFT, 235, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::HYPER_BALD_CLEFT, 288, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::DARK_CRAW, 308, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::CRAZEE_DAYZEE, 252, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::AMAZY_DAYZEE, 253, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::FUZZY, 248, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::GREEN_FUZZY, 249, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::FLOWER_FUZZY, 250, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::GOOMBA, 214, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::SPIKY_GOOMBA, 215, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::PARAGOOMBA, 216, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::HYPER_GOOMBA, 217, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::HYPER_SPIKY_GOOMBA, 218, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::HYPER_PARAGOOMBA, 219, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::GLOOMBA, 220, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::SPIKY_GLOOMBA, 221, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::PARAGLOOMBA, 222, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::KOOPA_TROOPA, 242, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::PARATROOPA, 243, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::KP_KOOPA, 246, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::KP_PARATROOPA, 247, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::SHADY_KOOPA, 282, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::SHADY_PARATROOPA, 291, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::DARK_KOOPA, 244, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::DARK_PARATROOPA, 245, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::KOOPATROL, 205, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::DARK_KOOPATROL, 307, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::LAKITU, 280, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::DARK_LAKITU, 281, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::SPINY, 287, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::SKY_BLUE_SPINY, -1, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::RED_MAGIKOOPA, 318, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::WHITE_MAGIKOOPA, 319, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::GREEN_MAGIKOOPA, 320, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::MAGIKOOPA, 321, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::X_NAUT, 271, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::X_NAUT_PHD, 273, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::ELITE_X_NAUT, 272, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::PIDER, 266, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::ARANTULA, 267, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::PALE_PIRANHA, 261, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::PUTRID_PIRANHA, 262, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::FROST_PIRANHA, 263, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::PIRANHA_PLANT, 260, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::POKEY, 233, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::POISON_POKEY, 234, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::DARK_PUFF, 286, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::RUFF_PUFF, 284, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::ICE_PUFF, 285, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::POISON_PUFF, 265, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::SPINIA, 310, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::SPANIA, 309, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::SPUNIA, 311, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::SWOOPER, 239, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::SWOOPULA, 240, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::SWAMPIRE, 241, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::WIZZERD, 295, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::DARK_WIZZERD, 296, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::ELITE_WIZZERD, 297, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::YUX, 268, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::Z_YUX, 269, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::X_YUX, 270, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::MINI_YUX, -1, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::MINI_Z_YUX, -1, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { BattleUnitType::MINI_X_YUX, -1, 0, 0, 0, 0, 0, kHpTables[0], kFpTables[0], -1 },
    { /* invalid enemy */ },
};

const EnemyModuleInfo kEnemyModuleInfo[] = {
    { BattleUnitType::BONETAIL, ModuleId::JON, 0x159d0, 34, 0 },
    { BattleUnitType::ATOMIC_BOO, ModuleId::JIN, 0x1a6a8, 24, 1 },
    { BattleUnitType::GLOOMBA, ModuleId::JON, 0x15a20, 1, 49 },
    { BattleUnitType::SPINIA, ModuleId::JON, 0x15b70, 28, 85 },
    { BattleUnitType::SPANIA, ModuleId::JON, 0x15e10, 28, 86 },
    { BattleUnitType::DULL_BONES, ModuleId::JON, 0x16050, 12, 12 },
    { BattleUnitType::FUZZY, ModuleId::JON, 0x16260, 8, 40 },
    { BattleUnitType::PARAGLOOMBA, ModuleId::JON, 0x16500, 2, 51 },
    { BattleUnitType::CLEFT, ModuleId::JON, 0x166e0, 17, 33 },
    { BattleUnitType::POKEY, ModuleId::JON, 0x16920, 15, 79 },
    { BattleUnitType::DARK_PUFF, ModuleId::JON, 0x16b60, 22, 81 },
    { BattleUnitType::PIDER, ModuleId::JON, 0x16d40, 20, 73 },
    { BattleUnitType::SPIKY_GLOOMBA, ModuleId::JON, 0x16f80, 1, 50 },
    { BattleUnitType::BANDIT, ModuleId::JON, 0x171f0, 1, 2 },
    { BattleUnitType::LAKITU, ModuleId::JON, 0x17490, 25, 62 },
    { BattleUnitType::BOB_OMB, ModuleId::JON, 0x176d0, 1, 9 },
    { BattleUnitType::BOO, ModuleId::JON, 0x17940, 24, 16 },
    { BattleUnitType::DARK_KOOPA, ModuleId::JON, 0x17bb0, 3, 58 },
    { BattleUnitType::HYPER_CLEFT, ModuleId::JON, 0x17d90, 17, 34 },
    { BattleUnitType::PARABUZZY, ModuleId::JON, 0x17fa0, 7, 28 },
    { BattleUnitType::SHADY_KOOPA, ModuleId::JON, 0x181e0, 3, 56 },
    { BattleUnitType::FLOWER_FUZZY, ModuleId::JON, 0x18420, 8, 42 },
    { BattleUnitType::DARK_PARATROOPA, ModuleId::JON, 0x18660, 5, 59 },
    { BattleUnitType::BULKY_BOB_OMB, ModuleId::JON, 0x188a0, 26, 10 },
    { BattleUnitType::LAVA_BUBBLE, ModuleId::JON, 0x18ab0, 25, 23 },
    { BattleUnitType::POISON_POKEY, ModuleId::JON, 0x18cf0, 15, 80 },
    { BattleUnitType::SPIKY_PARABUZZY, ModuleId::JON, 0x18f30, 7, 29 },
    { BattleUnitType::BADGE_BANDIT, ModuleId::JON, 0x19170, 1, 4 },
    { BattleUnitType::ICE_PUFF, ModuleId::JON, 0x19380, 22, 83 },
    { BattleUnitType::DARK_BOO, ModuleId::JON, 0x19590, 24, 17 },
    { BattleUnitType::RED_CHOMP, ModuleId::JON, 0x197d0, 33, 32 },
    { BattleUnitType::MOON_CLEFT, ModuleId::JON, 0x199e0, 17, 35 },
    { BattleUnitType::DARK_LAKITU, ModuleId::JON, 0x19c20, 25, 63 },
    { BattleUnitType::DRY_BONES, ModuleId::JON, 0x19e00, 11, 14 },
    { BattleUnitType::DARK_WIZZERD, ModuleId::JON, 0x1a010, 29, 92 },
    { BattleUnitType::FROST_PIRANHA, ModuleId::JON, 0x1a220, 21, 77 },
    { BattleUnitType::DARK_CRAW, ModuleId::JON, 0x1a430, 1, 37 },
    { BattleUnitType::WIZZERD, ModuleId::JON, 0x1a5e0, 29, 91 },
    { BattleUnitType::DARK_KOOPATROL, ModuleId::JON, 0x1a7f0, 4, 61 },
    { BattleUnitType::PHANTOM_EMBER, ModuleId::JON, 0x1aa00, 25, 25 },
    { BattleUnitType::SWOOPULA, ModuleId::JON, 0x1acd0, 23, 89 },
    { BattleUnitType::CHAIN_CHOMP, ModuleId::JON, 0x1af70, 33, 31 },
    { BattleUnitType::SPUNIA, ModuleId::JON, 0x1b1b0, 28, 87 },
    { BattleUnitType::DARK_BRISTLE, ModuleId::JON, 0x1b420, 18, 19 },
    { BattleUnitType::ARANTULA, ModuleId::JON, 0x1b630, 20, 74 },
    { BattleUnitType::PIRANHA_PLANT, ModuleId::JON, 0x1b870, 21, 78 },
    { BattleUnitType::ELITE_WIZZERD, ModuleId::JON, 0x1bd80, 29, 93 },
    { BattleUnitType::POISON_PUFF, ModuleId::JON, 0x1bff0, 22, 84 },
    { BattleUnitType::BOB_ULK, ModuleId::JON, 0x1c290, 26, 11 },
    { BattleUnitType::SWAMPIRE, ModuleId::JON, 0x1c500, 23, 90 },
    { BattleUnitType::AMAZY_DAYZEE, ModuleId::JON, 0x1c7a0, 10, 39 },
    { BattleUnitType::GOOMBA, ModuleId::GON, 0x16efc, 1, 43 },
    { BattleUnitType::SPIKY_GOOMBA, ModuleId::GON, 0x16efc, 1, 44 },
    { BattleUnitType::PARAGOOMBA, ModuleId::GON, 0x169dc, 2, 45 },
    { BattleUnitType::KOOPA_TROOPA, ModuleId::GON, 0x16cbc, 3, 52 },
    { BattleUnitType::PARATROOPA, ModuleId::GON, 0x1660c, 5, 53 },
    { BattleUnitType::RED_BONES, ModuleId::GON, 0x166bc, 12, 13 },
    { BattleUnitType::GOOMBA, ModuleId::GRA, 0x8690, 1, 43 },
    { BattleUnitType::HYPER_GOOMBA, ModuleId::GRA, 0x8690, 1, 46 },
    { BattleUnitType::HYPER_PARAGOOMBA, ModuleId::GRA, 0x8950, 2, 48 },
    { BattleUnitType::HYPER_SPIKY_GOOMBA, ModuleId::GRA, 0x8c70, 1, 47 },
    { BattleUnitType::CRAZEE_DAYZEE, ModuleId::GRA, 0x9090, 9, 38 },
    { BattleUnitType::GOOMBA, ModuleId::TIK, 0x27030, 1, 43 },
    { BattleUnitType::PARAGOOMBA, ModuleId::TIK, 0x27080, 2, 45 },
    { BattleUnitType::SPIKY_GOOMBA, ModuleId::TIK, 0x270b0, 1, 44 },
    { BattleUnitType::KOOPA_TROOPA, ModuleId::TIK, 0x26d60, 3, 52 },
    { BattleUnitType::HAMMER_BRO, ModuleId::TIK, 0x27120, 32, 20 },
    { BattleUnitType::MAGIKOOPA, ModuleId::TIK, 0x267d0, 31, 69 },
    { BattleUnitType::KOOPATROL, ModuleId::TIK, 0x26d30, 4, 60 },
    { BattleUnitType::GOOMBA, ModuleId::TOU2, 0x1eb40, 1, 43 },
    { BattleUnitType::KP_KOOPA, ModuleId::TOU2, 0x1ec50, 3, 54 },
    { BattleUnitType::KP_PARATROOPA, ModuleId::TOU2, 0x1ecb0, 5, 55 },
    { BattleUnitType::SHADY_PARATROOPA, ModuleId::TOU2, 0x1f3e0, 5, 57 },
    { BattleUnitType::HAMMER_BRO, ModuleId::TOU2, 0x1f610, 32, 20 },
    { BattleUnitType::BOOMERANG_BRO, ModuleId::TOU2, 0x1f670, 1, 21 },
    { BattleUnitType::FIRE_BRO, ModuleId::TOU2, 0x1f640, 1, 22 },
    { BattleUnitType::RED_MAGIKOOPA, ModuleId::TOU2, 0x1f510, 31, 66 },
    { BattleUnitType::WHITE_MAGIKOOPA, ModuleId::TOU2, 0x1f540, 31, 67 },
    { BattleUnitType::GREEN_MAGIKOOPA, ModuleId::TOU2, 0x1f570, 31, 68 },
    { BattleUnitType::RED_SPIKY_BUZZY, ModuleId::TOU2, 0x1f2b0, 6, 30 },
    { BattleUnitType::GREEN_FUZZY, ModuleId::TOU2, 0x1f490, 8, 41 },
    { BattleUnitType::PALE_PIRANHA, ModuleId::TOU2, 0x1ef10, 21, 75 },
    { BattleUnitType::BIG_BANDIT, ModuleId::TOU2, 0x1f1b0, 1, 3 },
    { BattleUnitType::SWOOPER, ModuleId::TOU2, 0x1f790, 23, 88 },
    { BattleUnitType::HYPER_BALD_CLEFT, ModuleId::TOU2, 0x1efc0, 16, 36 },
    { BattleUnitType::BRISTLE, ModuleId::TOU2, 0x1f330, 18, 18 },
    { BattleUnitType::X_NAUT, ModuleId::AJI, 0x44914, 1, 70 },
    { BattleUnitType::ELITE_X_NAUT, ModuleId::AJI, 0x44894, 1, 72 },
    { BattleUnitType::X_NAUT_PHD, ModuleId::AJI, 0x44aa4, 27, 71 },
    { BattleUnitType::YUX, ModuleId::AJI, 0x44f44, 19, 94 },
    { BattleUnitType::Z_YUX, ModuleId::AJI, 0x44b24, 19, 95 },
    { BattleUnitType::X_YUX, ModuleId::AJI, 0x450d4, 19, 96 },
    { BattleUnitType::GOOMBA, ModuleId::DOU, 0x163d8, 1, 43 },
    { BattleUnitType::BILL_BLASTER, ModuleId::DOU, 0x16698, 14, 5 },
    { BattleUnitType::EMBER, ModuleId::DOU, 0x16488, 25, 24 },
    { BattleUnitType::RED_BONES, ModuleId::LAS, 0x3c100, 12, 13 },
    { BattleUnitType::DARK_BONES, ModuleId::LAS, 0x3c1a0, 11, 15 },
    { BattleUnitType::BOMBSHELL_BILL_BLASTER, ModuleId::LAS, 0x3cab0, 14, 6 },
    { BattleUnitType::BUZZY_BEETLE, ModuleId::JIN, 0x1b078, 6, 26 },
    { BattleUnitType::SPIKE_TOP, ModuleId::JIN, 0x1b148, 6, 27 },
    { BattleUnitType::SWOOPER, ModuleId::JIN, 0x1ab38, 23, 88 },
    { BattleUnitType::GREEN_FUZZY, ModuleId::MUJ, 0x358b8, 8, 41 },
    { BattleUnitType::PUTRID_PIRANHA, ModuleId::MUJ, 0x35ac8, 21, 76 },
    { BattleUnitType::EMBER, ModuleId::MUJ, 0x35218, 25, 24 },
    { BattleUnitType::GOOMBA, ModuleId::EKI, 0xff48, 1, 43 },
    { BattleUnitType::RUFF_PUFF, ModuleId::EKI, 0xfff8, 22, 82 },
};

// Global structures for holding constructed battle information.
int32_t g_NumEnemies = 0;
int32_t g_Enemies[5] = { -1, -1, -1, -1, -1 };
NpcSetupInfo g_CustomNpc[2];
BattleUnitSetup g_CustomUnits[6];
BattleGroupSetup g_CustomBattleParty;
// AudienceTypeWeights g_CustomAudienceWeights[16];

}

ModuleId::e SelectEnemies(int32_t floor) {
    // TODO: Procedurally pick a group of enemies based on the floor number;
    // currently hardcoded to test one enemy type, in a group of 3.
    const int32_t enemyTypeToTest = g_Randomizer->state_.debug_[0];
    g_NumEnemies = enemyTypeToTest < 2 ? 1 : 3;
    for (int32_t i = 0; i < 5; ++i) {
        if (i < g_NumEnemies) {
            g_Enemies[i] = enemyTypeToTest;  // from randomizer.h
        } else {
            g_Enemies[i] = -1;
        }
    }
    
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
    const NpcEntTypeInfo* npc_info = nullptr;
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
        ttyd::npc_data::npcTribe + enemy_info[0]->npc_tribe_idx;
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
        BattleUnitSetup& custom_unit = g_CustomUnits[i];
        memcpy(&custom_unit, unit_info[i], sizeof(BattleUnitSetup));
        // TODO: Place enemies in the proper positions.
        // TODO: Link to Goomba's unit_kind if Goomba is the type selected.
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

struct BattleCondition {
    const char* description;
    uint8_t     type;
    uint8_t     param_min;
    int8_t      param_max;  // if -1, not a range.
    uint8_t     weight = 10;
};

using namespace ttyd::battle_actrecord::ConditionType;
static constexpr const BattleCondition kBattleConditions[] = {
    { "Don't ever use Jump moves!", JUMP_LESS, 1, -1 },
    { "Use fewer than %" PRId32 " Jump moves!", JUMP_LESS, 2, 3, 5 },
    { "Don't ever use Hammer moves!", HAMMER_LESS, 1, -1 },
    { "Use fewer than %" PRId32 " Hammer moves!", HAMMER_LESS, 2, 3, 5 },
    { "Don't use Special moves!", SPECIAL_MOVES_LESS, 1, -1 },
    { "Use a Special move!", SPECIAL_MOVES_MORE, 1, -1 },
    { "Don't take damage with Mario!", MARIO_TOTAL_DAMAGE_LESS, 1, -1 },
    { "Don't take damage with your partner!", PARTNER_TOTAL_DAMAGE_LESS, 1, -1 },
    { "Take less than %" PRId32 " total damage!", TOTAL_DAMAGE_LESS, 1, 5 },
    { "Take at least %" PRId32 " total damage!", TOTAL_DAMAGE_MORE, 1, 5 },
    { "Take damage at least %" PRId32 " times!", HITS_MORE, 3, 5 },
    { "Win with Mario at %" PRId32 " or more HP!", MARIO_FINAL_HP_MORE, 1, -1 },
    { "Win with Mario at 5 HP or less!", MARIO_FINAL_HP_LESS, 6, -1 },
    { "Win with Mario in Peril!", MARIO_FINAL_HP_LESS, 2, -1 },
    { "Don't use any items!", ITEMS_LESS, 1, -1, 30 },
    { "Don't ever swap partners!", SWAP_PARTNERS_LESS, 1, -1 },
    { "Have Mario attack the audience!", MARIO_ATTACK_AUDIENCE_MORE, 1, -1, 3 },
    { "Appeal at least once!", APPEAL_MORE, 1, -1 },
    { "Don't use any FP!", FP_LESS, 1, -1 },
    { "Don't use more than %" PRId32 " FP!", FP_LESS, 4, 11 },
    { "Use at least %" PRId32 " FP!", FP_MORE, 1, 4 },
    { "Mario must only Appeal and Defend!", MARIO_INACTIVE_TURNS, 255, -1 },
    { "Your partner must only Appeal and Defend!", PARTNER_INACTIVE_TURNS, 255, -1 },
    { "Appeal/Defend only for %" PRId32 " turns!", INACTIVE_TURNS, 2, 5 },
    { "Never use attacks with Mario!", MARIO_NO_ATTACK_TURNS, 255, -1 },
    { "Never use attacks with your partner!", PARTNER_NO_ATTACK_TURNS, 255, -1 },
    { "Don't use attacks for %" PRId32 " turns!", NO_ATTACK_TURNS, 2, 5 },
    { "Mario can only Defend or use Jump moves!", JUMPMAN, 1, -1, 3 },
    { "Mario can only Defend or use Hammer moves!", HAMMERMAN, 1, -1, 3 },
    { "Finish the fight within %" PRId32 " turns!", TURNS_LESS, 2, 5, 20 },
};
char g_ConditionTextBuf[64];

void SetBattleCondition(ttyd::npcdrv::NpcBattleInfo* npc_info, bool enable) {
    // TODO: Don't set conditions on _every_ floor.
    // TODO: Select a reward item.
    
    constexpr const int32_t kNumConditions = 
        sizeof(kBattleConditions) / sizeof(BattleCondition);
    BattleCondition conditions[kNumConditions];
    memcpy(conditions, kBattleConditions, sizeof(kBattleConditions));
    
    RandomizerState& state = g_Randomizer->state_;
    const PouchData& pouch = *ttyd::mario_pouch::pouchGetPtr();
    int32_t num_partners = 0;
    for (int32_t i = 0; i < 8; ++i) {
        num_partners += pouch.party_data[i].flags & 1;
    }
    
    // Disable conditions that rely on Star Power or having 1 or more partners.
    for (auto& condition : conditions) {
        switch (condition.type) {
            case SPECIAL_MOVES_LESS:
            case SPECIAL_MOVES_MORE:
            case APPEAL_MORE:
                if (!pouch.star_powers_obtained) condition.weight = 0;
                break;
            case PARTNER_TOTAL_DAMAGE_LESS:
            case MARIO_INACTIVE_TURNS:
            case MARIO_NO_ATTACK_TURNS:
            case PARTNER_INACTIVE_TURNS:
            case PARTNER_NO_ATTACK_TURNS:
            case JUMPMAN:
            case HAMMERMAN:
                if (num_partners < 1) condition.weight = 0;
                break;
            case SWAP_PARTNERS_LESS:
                if (num_partners < 2) condition.weight = 0;
                break;
            case MARIO_ATTACK_AUDIENCE_MORE:
                // This condition will be guaranteed a Shine Sprite,
                // so there needs to be a partner (and SP needs to be unlocked).
                if (!pouch.star_powers_obtained) condition.weight = 0;
                if (num_partners < 1) condition.weight = 0;
                break;
        }
    }
    
    int32_t sum_weights = 0;
    for (int32_t i = 0; i < kNumConditions; ++i) 
        sum_weights += conditions[i].weight;
    
    int32_t weight = state.Rand(sum_weights);
    int32_t idx = 0;
    for (; (weight -= conditions[idx].weight) >= 0; ++idx);
    
    // Finalize and assign selected condition's parameters.
    int32_t param = conditions[idx].param_min;
    if (conditions[idx].param_max > 0) {
        param += state.Rand(conditions[idx].param_max - 
                            conditions[idx].param_min + 1);
    }
    switch (conditions[idx].type) {
        case TOTAL_DAMAGE_LESS:
        case TOTAL_DAMAGE_MORE:
        case FP_MORE:
            param *= state.floor_ < 50 ? 2 : 5;
            break;
        case MARIO_FINAL_HP_MORE:
            // Make it based on percentage of max HP.
            param = state.Rand(4);
            switch (param) {
                case 0:
                    // Half, rounded up.
                    param = (ttyd::mario_pouch::pouchGetMaxHP() + 1) * 50 / 100;
                    break;
                case 1:
                    param = ttyd::mario_pouch::pouchGetMaxHP() * 60 / 100;
                    break;
                case 2:
                    param = ttyd::mario_pouch::pouchGetMaxHP() * 80 / 100;
                    break;
                case 3:
                default:
                    param = ttyd::mario_pouch::pouchGetMaxHP();
                    break;
            }
            break;
    }
    npc_info->ruleCondition = conditions[idx].type;
    npc_info->ruleParameter0 = param;
    npc_info->ruleParameter1 = param;
    
    // Assign the condition text.
    if (conditions[idx].param_max > 0 || 
        conditions[idx].type == MARIO_FINAL_HP_MORE) {
        // FP condition text says "no more than", rather than "less than".
        if (conditions[idx].type == FP_LESS) --param;
        sprintf(g_ConditionTextBuf, conditions[idx].description, param);
    } else {
        sprintf(g_ConditionTextBuf, conditions[idx].description);
    }        
}

void GetBattleConditionString(char* out_buf) {
    // TODO: Parametrize by the reward item selected on room load.
    sprintf(
        out_buf, "Bonus reward (%s):\n%s", "Shine Sprite", g_ConditionTextBuf);
}

int32_t PickRandomItem(
    bool seeded, int32_t normal_item_weight, int32_t recipe_item_weight,
    int32_t badge_weight, int32_t no_item_weight) {
    // Bitfields of whether each item is included in the pool or not;
    // item X is enabled if kItemPool[X / 16 - offset] & (1 << (X % 16)) != 0.
    static constexpr const uint16_t kNormalItems[] = {
        0xffff, 0xffff, 0x000f, 0x0006
    };
    static constexpr const uint16_t kRecipeItems[] = {
        0x2020, 0xffb8, 0x3edf, 0x97ef, 0x0bff
    };
    static constexpr const uint16_t kStackableBadges[] = {
        0x3fff, 0xffff, 0x0fff, 0xfff7, 0x038f, 0x0030, 0x0006
    };
    
    int32_t total_weight =
        normal_item_weight + recipe_item_weight + badge_weight + no_item_weight;
    int32_t result; 
    if (seeded) {
        result = g_Randomizer->state_.Rand(total_weight);
    } else {
        result = ttyd::system::irand(total_weight);
    }
    const uint16_t* bitfield;
    int32_t len_bitfield;
    int32_t offset;
    
    int32_t current_weight = normal_item_weight;
    if (result < current_weight) {
        bitfield = kNormalItems;
        len_bitfield = sizeof(kNormalItems) / sizeof(uint16_t);
        offset = 0x80;
    } else {
        current_weight += recipe_item_weight;
        if (result < current_weight) {
            bitfield = kRecipeItems;
            len_bitfield = sizeof(kRecipeItems) / sizeof(uint16_t);
            offset = 0xa0;
        } else {
            current_weight += badge_weight;
            if (result < current_weight) {
                bitfield = kStackableBadges;
                len_bitfield = sizeof(kStackableBadges) / sizeof(uint16_t);
                offset = 0xf0;
            } else {
                return 0;
            }
        }
    }
    
    int32_t num_items_seen = 0;
    for (int32_t i = 0; i < len_bitfield; ++i) {
        for (int32_t bit = 0; bit < 16; ++bit) {
            if (bitfield[i] & (1U << bit)) ++num_items_seen;
        }
    }
    if (seeded) {
        result = g_Randomizer->state_.Rand(num_items_seen);
    } else {
        result = ttyd::system::irand(num_items_seen);
    }
    num_items_seen = 0;
    for (int32_t i = 0; i < len_bitfield; ++i) {
        for (int32_t bit = 0; bit < 16; ++bit) {
            if (bitfield[i] & (1U << bit)) {
                if (result == num_items_seen) return offset + i * 16 + bit;
                ++num_items_seen;
            }
        }
    }
    // Should not be reached, as that would mean the random function returned
    // a larger index than there are bits in the bitfield.
    return -1;
}

int16_t PickChestReward() {
    static constexpr const int16_t kRewards[] = {
        // Mario / inventory upgrades.
        ItemType::STRANGE_SACK, ItemType::SUPER_BOOTS, ItemType::ULTRA_BOOTS,
        ItemType::SUPER_HAMMER, ItemType::ULTRA_HAMMER,
        // Star Powers.
        ItemType::MAGICAL_MAP, ItemType::DIAMOND_STAR, ItemType::EMERALD_STAR,
        ItemType::GOLD_STAR, ItemType::RUBY_STAR, ItemType::SAPPHIRE_STAR,
        ItemType::GARNET_STAR, ItemType::CRYSTAL_STAR,
        // Unique badges.
        ItemType::CHILL_OUT, ItemType::DOUBLE_DIP, ItemType::DOUBLE_DIP,
        ItemType::DOUBLE_DIP_P, ItemType::DOUBLE_DIP_P, ItemType::FEELING_FINE,
        ItemType::FEELING_FINE_P, ItemType::LUCKY_START, ItemType::QUICK_CHANGE,
        ItemType::RETURN_POSTAGE, ItemType::SPIKE_SHIELD, ItemType::ZAP_TAP,
        // Partners (represented by dummy values).
        -1, -2, -3, -4, -5, -6, -7
    };
    static_assert(sizeof(kRewards) == 32 * sizeof(int16_t));
    
    uint8_t weights[34];
    for (int32_t i = 0; i < 34; ++i) weights[i] = 10;
    
    RandomizerState& state = g_Randomizer->state_;
    const PouchData& pouch = *ttyd::mario_pouch::pouchGetPtr();
    // Modify the chance of getting a partner based on the current floor
    // and the number of partners currently obtained.
    int32_t num_partners = 0;
    for (int32_t i = 0; i < 8; ++i) {
        num_partners += pouch.party_data[i].flags & 1;
    }
    
    if (state.floor_ == 29 && num_partners == 0) {
        // Floor 30; force a partner if you don't already have one.
        for (int32_t i = 0; i < 32; ++i) {
            if (kRewards[i] >= 0) weights[i] = 0;
        }
        weights[32] = 0;
        weights[33] = 0;
    } else {
        // Determine the weight for a partner based on how many you have
        // currently and how deep in the Pit you are.
        int32_t partner_weight = 0;
        if (num_partners < 7) {
            partner_weight =
                (60 + state.floor_ - 30 * num_partners) / (7 - num_partners);
            if (partner_weight > 100) partner_weight = 100;
            if (partner_weight < 0) partner_weight = 0;
        }
        
        // Disable rewards that shouldn't be received out of order or that
        // have already been claimed, and assign partner weight to partners.
        for (int32_t i = 0; i < 32; ++i) {
            if (state.reward_flags_ & (1 << i)) {
                weights[i] = 0;
                continue;
            }
            switch (kRewards[i]) {
                case ItemType::ULTRA_BOOTS:
                    if (pouch.jump_level < 2) weights[i] = 0;
                    break;
                case ItemType::ULTRA_HAMMER:
                    if (pouch.hammer_level < 2) weights[i] = 0;
                    break;
                case ItemType::DIAMOND_STAR:
                case ItemType::EMERALD_STAR:
                    if (pouch.max_sp < 100) weights[i] = 0;
                    break;
                case ItemType::GOLD_STAR:
                    if (pouch.max_sp < 200) weights[i] = 0;
                    break;
                case ItemType::RUBY_STAR:
                case ItemType::SAPPHIRE_STAR:
                case ItemType::GARNET_STAR:
                    if (pouch.max_sp < 300) weights[i] = 0;
                    break;
                case ItemType::CRYSTAL_STAR:
                    if (pouch.max_sp < 500) weights[i] = 0;
                    break;
                case -1:
                case -2:
                case -3:
                case -4:
                case -5:
                case -6:
                case -7:
                    weights[i] = partner_weight;
                default:
                    break;
            }
        }
        
        // Set weights for a Shine Sprite (32) or random pool badge (33).
        weights[32] = state.floor_ > 30 ? 20 : 0;
        weights[33] = 20;
    }
    
    int32_t sum_weights = 0;
    for (int32_t i = 0; i < 34; ++i) sum_weights += weights[i];
    
    int32_t weight = state.Rand(sum_weights);
    int32_t reward_idx = 0;
    for (; (weight -= weights[reward_idx]) >= 0; ++reward_idx);
    
    int16_t reward;
    if (reward_idx < 32) {
        // Assign the selected reward and mark it as collected.
        reward = kRewards[reward_idx];
        state.reward_flags_ |= (1 << reward_idx);
    } else if (reward_idx == 32) {
        // Shine Sprite item.
        reward = ItemType::GOLD_BAR_X3;
    } else {
        // Pick a random pool badge.
        reward = PickRandomItem(/* seeded = */ true, 0, 0, 1, 0);
    }
    return reward;
}

}