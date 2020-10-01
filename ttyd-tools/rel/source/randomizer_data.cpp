#include "randomizer_data.h"

#include "common_functions.h"
#include "common_types.h"
#include "randomizer.h"
#include "randomizer_state.h"

#include <ttyd/battle.h>
#include <ttyd/battle_actrecord.h>
#include <ttyd/battle_database_common.h>
#include <ttyd/battle_monosiri.h>
#include <ttyd/battle_unit.h>
#include <ttyd/item_data.h>
#include <ttyd/mario_pouch.h>
#include <ttyd/mariost.h>
#include <ttyd/msgdrv.h>
#include <ttyd/npcdrv.h>
#include <ttyd/npc_data.h>
#include <ttyd/npc_event.h>
#include <ttyd/system.h>

#include <cinttypes>
#include <cstdio>
#include <cstring>

namespace mod::pit_randomizer {

namespace {

using ::ttyd::battle_unit::BattleWorkUnit;
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
    int16_t         npc_tribe_idx;
    // How quickly the enemy's HP, ATK and DEF scale by the floor (base stats).
    int16_t         hp_scale;
    int16_t         atk_scale;
    int16_t         def_scale;
    // The reference point used as the enemy's "base" attack power; other
    // attacks will have the same difference in power as in the original game.
    // (e.g. a Hyper Goomba will charge by its attack power + 4).
    int16_t         atk_base;
    // The difference between the vanilla base ATK and the mod's atk_base.
    int16_t         atk_offset;
    // The enemy's level will be this much higher than Mario's at base.
    int16_t         level_offset;
    // Makes a type of audience member more likely to spawn (-1 = none).
    int16_t         audience_type_boosted;
    // The enemy's HP and FP drop yields (this info isn't in BattleUnitSetup).
    PointDropData*  hp_drop_table;
    PointDropData*  fp_drop_table;
};

// All data required to construct a particular enemy NPC in a particular module.
// In particular, contains the offset in the given module for an existing
// BattleUnitSetup* to use as a reference for the constructed battle.
struct EnemyModuleInfo {
    BattleUnitType::e   unit_type;
    ModuleId::e         module;
    int32_t             battle_unit_setup_offset;
    int16_t             npc_ent_type_info_idx;
    int16_t             enemy_type_stats_idx;
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
const float kEnemyPartySepX = 40.0f;
const float kEnemyPartySepZ = 10.0f;

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
    { BattleUnitType::BONETAIL, 325, 200, 8, 2, 8, 0, 100, -1, kHpTables[0], kFpTables[0] },
    { BattleUnitType::ATOMIC_BOO, 148, 100, 4, 0, 2, 2, 60, 2, kHpTables[2], kFpTables[2] },
    { BattleUnitType::BANDIT, 274, 12, 6, 0, 2, 0, 4, 5, kHpTables[0], kFpTables[0] },
    { BattleUnitType::BIG_BANDIT, 129, 15, 6, 0, 2, 1, 5, 5, kHpTables[0], kFpTables[0] },
    { BattleUnitType::BADGE_BANDIT, 275, 18, 6, 0, 3, 2, 6, 5, kHpTables[0], kFpTables[0] },
    { BattleUnitType::BILL_BLASTER, 254, 10, 0, 3, 0, 0, 6, 9, kHpTables[1], kFpTables[0] },
    { BattleUnitType::BOMBSHELL_BILL_BLASTER, 256, 15, 0, 5, 0, 0, 10, 9, kHpTables[2], kFpTables[0] },
    { BattleUnitType::BULLET_BILL, 255, 4, 7, 1, 4, 0, 0, 9, kHpTables[0], kFpTables[0] },
    { BattleUnitType::BOMBSHELL_BILL, 257, 6, 9, 2, 6, 0, 0, 9, kHpTables[0], kFpTables[0] },
    { BattleUnitType::BOB_OMB, 283, 10, 7, 2, 2, 0, 5, 9, kHpTables[1], kFpTables[0] },
    { BattleUnitType::BULKY_BOB_OMB, 304, 12, 4, 2, 2, 0, 5, 9, kHpTables[1], kFpTables[0] },
    { BattleUnitType::BOB_ULK, 305, 15, 5, 2, 4, 0, 7, 9, kHpTables[3], kFpTables[0] },
    { BattleUnitType::DULL_BONES, 39, 7, 5, 1, 1, 1, 2, 4, kHpTables[0], kFpTables[0] },
    { BattleUnitType::RED_BONES, 36, 10, 7, 2, 3, 0, 5, 4, kHpTables[0], kFpTables[0] },
    { BattleUnitType::DRY_BONES, 196, 12, 7, 3, 5, 0, 7, 4, kHpTables[0], kFpTables[2] },
    { BattleUnitType::DARK_BONES, 197, 20, 7, 3, 4, 1, 10, 4, kHpTables[1], kFpTables[2] },
    { BattleUnitType::BOO, 146, 13, 6, 0, 2, 1, 5, 2, kHpTables[0], kFpTables[1] },
    { BattleUnitType::DARK_BOO, 147, 17, 8, 0, 4, 1, 7, 2, kHpTables[0], kFpTables[1] },
    { BattleUnitType::BRISTLE, 258, 6, 6, 4, 1, 0, 4, -1, kHpTables[0], kFpTables[1] },
    { BattleUnitType::DARK_BRISTLE, 259, 9, 9, 4, 8, 0, 8, -1, kHpTables[0], kFpTables[3] },
    { BattleUnitType::HAMMER_BRO, 206, 16, 6, 2, 3, 1, 9, 3, kHpTables[2], kFpTables[1] },
    { BattleUnitType::BOOMERANG_BRO, 294, 16, 4, 2, 2, 0, 9, 3, kHpTables[2], kFpTables[1] },
    { BattleUnitType::FIRE_BRO, 293, 16, 4, 2, 1, 2, 9, 3, kHpTables[2], kFpTables[1] },
    { BattleUnitType::LAVA_BUBBLE, 302, 10, 6, 0, 3, 1, 6, 2, kHpTables[0], kFpTables[1] },
    { BattleUnitType::EMBER, 159, 13, 6, 0, 3, 0, 6, 2, kHpTables[0], kFpTables[1] },
    { BattleUnitType::PHANTOM_EMBER, 303, 16, 6, 0, 3, 2, 8, 2, kHpTables[0], kFpTables[2] },
    { BattleUnitType::BUZZY_BEETLE, 225, 8, 6, 5, 3, 0, 4, 7, kHpTables[1], kFpTables[0] },
    { BattleUnitType::SPIKE_TOP, 226, 8, 6, 5, 3, 0, 6, 7, kHpTables[1], kFpTables[0] },
    { BattleUnitType::PARABUZZY, 228, 8, 6, 5, 3, 0, 5, 7, kHpTables[1], kFpTables[0] },
    { BattleUnitType::SPIKY_PARABUZZY, 227, 8, 6, 5, 3, 0, 7, 7, kHpTables[2], kFpTables[0] },
    { BattleUnitType::RED_SPIKY_BUZZY, 230, 8, 6, 5, 3, 0, 6, 7, kHpTables[1], kFpTables[0] },
    { BattleUnitType::CHAIN_CHOMP, 301, 10, 8, 4, 6, 0, 6, -1, kHpTables[3], kFpTables[0] },
    { BattleUnitType::RED_CHOMP, 306, 12, 10, 5, 5, 0, 8, -1, kHpTables[2], kFpTables[0] },
    { BattleUnitType::CLEFT, 237, 8, 6, 5, 2, 0, 2, -1, kHpTables[1], kFpTables[0] },
    { BattleUnitType::HYPER_CLEFT, 236, 10, 6, 5, 3, 0, 6, -1, kHpTables[1], kFpTables[0] },
    { BattleUnitType::MOON_CLEFT, 235, 12, 8, 5, 5, 0, 6, -1, kHpTables[1], kFpTables[0] },
    { BattleUnitType::HYPER_BALD_CLEFT, 288, 10, 6, 5, 3, 0, 5, -1, kHpTables[1], kFpTables[0] },
    { BattleUnitType::DARK_CRAW, 308, 20, 9, 0, 6, 0, 8, -1, kHpTables[3], kFpTables[0] },
    { BattleUnitType::CRAZEE_DAYZEE, 252, 14, 5, 0, 2, 0, 6, 6, kHpTables[0], kFpTables[2] },
    { BattleUnitType::AMAZY_DAYZEE, 253, 20, 20, 1, 20, 0, 80, 6, kHpTables[2], kFpTables[4] },
    { BattleUnitType::FUZZY, 248, 11, 5, 0, 1, 0, 2, -1, kHpTables[0], kFpTables[0] },
    { BattleUnitType::GREEN_FUZZY, 249, 13, 6, 0, 2, 1, 4, -1, kHpTables[0], kFpTables[0] },
    { BattleUnitType::FLOWER_FUZZY, 250, 13, 6, 0, 2, 1, 6, -1, kHpTables[0], kFpTables[2] },
    { BattleUnitType::GOOMBA, 214, 10, 6, 0, 1, 0, 2, 10, kHpTables[0], kFpTables[0] },
    { BattleUnitType::SPIKY_GOOMBA, 215, 10, 6, 0, 1, 1, 3, 10, kHpTables[0], kFpTables[0] },
    { BattleUnitType::PARAGOOMBA, 216, 10, 6, 0, 1, 0, 3, 10, kHpTables[0], kFpTables[0] },
    { BattleUnitType::HYPER_GOOMBA, 217, 15, 6, 0, 3, -1, 5, 10, kHpTables[0], kFpTables[0] },
    { BattleUnitType::HYPER_SPIKY_GOOMBA, 218, 15, 6, 0, 3, 0, 6, 10, kHpTables[0], kFpTables[0] },
    { BattleUnitType::HYPER_PARAGOOMBA, 219, 15, 6, 0, 3, -1, 6, 10, kHpTables[0], kFpTables[0] },
    { BattleUnitType::GLOOMBA, 220, 20, 6, 0, 2, 1, 5, 10, kHpTables[0], kFpTables[0] },
    { BattleUnitType::SPIKY_GLOOMBA, 221, 20, 6, 0, 2, 2, 6, 10, kHpTables[0], kFpTables[0] },
    { BattleUnitType::PARAGLOOMBA, 222, 20, 6, 0, 2, 1, 6, 10, kHpTables[0], kFpTables[0] },
    { BattleUnitType::KOOPA_TROOPA, 242, 15, 7, 2, 2, 0, 4, 8, kHpTables[0], kFpTables[0] },
    { BattleUnitType::PARATROOPA, 243, 15, 7, 2, 2, 0, 5, 8, kHpTables[0], kFpTables[0] },
    { BattleUnitType::KP_KOOPA, 246, 15, 7, 2, 2, 0, 4, 8, kHpTables[0], kFpTables[0] },
    { BattleUnitType::KP_PARATROOPA, 247, 15, 7, 2, 2, 0, 5, 8, kHpTables[0], kFpTables[0] },
    { BattleUnitType::SHADY_KOOPA, 282, 18, 7, 2, 3, 0, 6, 8, kHpTables[0], kFpTables[0] },
    { BattleUnitType::SHADY_PARATROOPA, 291, 18, 7, 2, 3, 0, 7, 8, kHpTables[0], kFpTables[0] },
    { BattleUnitType::DARK_KOOPA, 244, 20, 8, 3, 3, 1, 6, 8, kHpTables[0], kFpTables[0] },
    { BattleUnitType::DARK_PARATROOPA, 245, 20, 8, 3, 3, 1, 7, 8, kHpTables[0], kFpTables[0] },
    { BattleUnitType::KOOPATROL, 205, 15, 8, 3, 4, 0, 6, 8, kHpTables[3], kFpTables[0] },
    { BattleUnitType::DARK_KOOPATROL, 307, 25, 10, 3, 5, 0, 10, 8, kHpTables[3], kFpTables[1] },
    { BattleUnitType::LAKITU, 280, 13, 7, 0, 2, 0, 4, -1, kHpTables[0], kFpTables[1] },
    { BattleUnitType::DARK_LAKITU, 281, 19, 9, 0, 5, 0, 8, -1, kHpTables[2], kFpTables[0] },
    { BattleUnitType::SPINY, 287, 8, 7, 4, 2, 1, 1, -1, kHpTables[0], kFpTables[0] },
    { BattleUnitType::SKY_BLUE_SPINY, -1, 10, 9, 4, 5, 1, 1, -1, kHpTables[0], kFpTables[0] },
    { BattleUnitType::RED_MAGIKOOPA, 318, 15, 7, 0, 4, 0, 7, 3, kHpTables[0], kFpTables[3] },
    { BattleUnitType::WHITE_MAGIKOOPA, 319, 15, 7, 0, 4, 0, 7, 3, kHpTables[0], kFpTables[3] },
    { BattleUnitType::GREEN_MAGIKOOPA, 320, 15, 7, 0, 4, 0, 7, 3, kHpTables[0], kFpTables[3] },
    { BattleUnitType::MAGIKOOPA, 321, 15, 7, 0, 4, 0, 7, 3, kHpTables[0], kFpTables[3] },
    { BattleUnitType::X_NAUT, 271, 12, 7, 0, 3, 0, 4, 1, kHpTables[0], kFpTables[0] },
    { BattleUnitType::X_NAUT_PHD, 273, 14, 8, 0, 4, 0, 8, 1, kHpTables[0], kFpTables[2] },
    { BattleUnitType::ELITE_X_NAUT, 272, 16, 9, 2, 5, 0, 8, 1, kHpTables[2], kFpTables[0] },
    { BattleUnitType::PIDER, 266, 14, 6, 0, 2, 0, 5, -1, kHpTables[0], kFpTables[0] },
    { BattleUnitType::ARANTULA, 267, 18, 6, 0, 5, 2, 8, -1, kHpTables[2], kFpTables[2] },
    { BattleUnitType::PALE_PIRANHA, 261, 14, 7, 0, 2, 0, 5, 11, kHpTables[0], kFpTables[1] },
    { BattleUnitType::PUTRID_PIRANHA, 262, 14, 6, 0, 2, 1, 5, 11, kHpTables[0], kFpTables[2] },
    { BattleUnitType::FROST_PIRANHA, 263, 16, 7, 0, 4, 1, 7, 11, kHpTables[0], kFpTables[2] },
    { BattleUnitType::PIRANHA_PLANT, 260, 18, 8, 0, 7, 2, 9, 11, kHpTables[0], kFpTables[4] },
    { BattleUnitType::POKEY, 233, 12, 7, 0, 3, 0, 4, -1, kHpTables[1], kFpTables[0] },
    { BattleUnitType::POISON_POKEY, 234, 15, 7, 0, 3, 1, 6, -1, kHpTables[1], kFpTables[0] },
    { BattleUnitType::DARK_PUFF, 286, 12, 7, 0, 2, 0, 3, -1, kHpTables[0], kFpTables[0] },
    { BattleUnitType::RUFF_PUFF, 284, 14, 8, 0, 4, 0, 4, -1, kHpTables[0], kFpTables[0] },
    { BattleUnitType::ICE_PUFF, 285, 16, 8, 0, 4, 0, 6, -1, kHpTables[0], kFpTables[0] },
    { BattleUnitType::POISON_PUFF, 265, 18, 8, 0, 8, 0, 8, -1, kHpTables[0], kFpTables[0] },
    { BattleUnitType::SPINIA, 310, 13, 6, 0, 1, 0, 2, -1, kHpTables[0], kFpTables[0] },
    { BattleUnitType::SPANIA, 309, 13, 6, 0, 1, 0, 3, -1, kHpTables[0], kFpTables[0] },
    { BattleUnitType::SPUNIA, 311, 16, 7, 2, 6, 1, 6, -1, kHpTables[3], kFpTables[0] },
    { BattleUnitType::SWOOPER, 239, 14, 7, 0, 3, 0, 5, -1, kHpTables[0], kFpTables[0] },
    { BattleUnitType::SWOOPULA, 240, 14, 6, 0, 4, 0, 5, -1, kHpTables[0], kFpTables[0] },
    { BattleUnitType::SWAMPIRE, 241, 20, 8, 0, 6, 0, 8, -1, kHpTables[0], kFpTables[0] },
    { BattleUnitType::WIZZERD, 295, 10, 8, 3, 7, -1, 7, -1, kHpTables[1], kFpTables[1] },
    { BattleUnitType::DARK_WIZZERD, 296, 12, 8, 4, 5, 0, 8, -1, kHpTables[2], kFpTables[2] },
    { BattleUnitType::ELITE_WIZZERD, 297, 14, 8, 5, 7, 1, 10, -1, kHpTables[3], kFpTables[3] },
    { BattleUnitType::YUX, 268, 7, 5, 0, 2, 0, 6, 1, kHpTables[0], kFpTables[0] },
    { BattleUnitType::Z_YUX, 269, 9, 6, 0, 4, 0, 8, 1, kHpTables[1], kFpTables[1] },
    { BattleUnitType::X_YUX, 270, 11, 5, 2, 3, 0, 10, 1, kHpTables[2], kFpTables[2] },
    { BattleUnitType::MINI_YUX, -1, 1, 0, 0, 0, 0, 0, 1, kHpTables[0], kFpTables[0] },
    { BattleUnitType::MINI_Z_YUX, -1, 2, 0, 0, 0, 0, 0, 1, kHpTables[0], kFpTables[0] },
    { BattleUnitType::MINI_X_YUX, -1, 1, 0, 0, 0, 0, 0, 1, kHpTables[0], kFpTables[0] },
    // Copied stats for slight variants of enemies.
    { BattleUnitType::RED_MAGIKOOPA_CLONE, 318, 15, 7, 0, 4, 0, 7, 3, kHpTables[0], kFpTables[3] },
    { BattleUnitType::WHITE_MAGIKOOPA_CLONE, 319, 15, 7, 0, 4, 0, 7, 3, kHpTables[0], kFpTables[3] },
    { BattleUnitType::GREEN_MAGIKOOPA_CLONE, 320, 15, 7, 0, 4, 0, 7, 3, kHpTables[0], kFpTables[3] },
    { BattleUnitType::MAGIKOOPA_CLONE, 321, 15, 7, 0, 4, 0, 7, 3, kHpTables[0], kFpTables[3] },
    { BattleUnitType::DARK_WIZZERD_CLONE, 296, 12, 8, 4, 5, 0, 8, -1, kHpTables[2], kFpTables[2] },
    { BattleUnitType::ELITE_WIZZERD_CLONE, 297, 14, 8, 5, 7, 1, 10, -1, kHpTables[3], kFpTables[3] },
    { BattleUnitType::GOOMBA_GLITZVILLE, 214, 10, 6, 0, 1, 0, 2, 10, kHpTables[0], kFpTables[0] },
    { /* invalid enemy */ },
};

const EnemyModuleInfo kEnemyModuleInfo[] = {
    // Bosses / special enemies.
    { BattleUnitType::BONETAIL, ModuleId::JON, 0x159d0, 34, 0 },
    { BattleUnitType::ATOMIC_BOO, ModuleId::JIN, 0x1a6a8, 24, 1 },
    { BattleUnitType::AMAZY_DAYZEE, ModuleId::JON, 0x1c7a0, -1, 39 },
    // Pit-native enemies.
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
    // Non-Pit-native enemies.
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
    { BattleUnitType::SPIKY_GOOMBA, ModuleId::TIK, 0x270d0, 1, 44 },
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
    { BattleUnitType::RED_MAGIKOOPA, ModuleId::TOU2, 0x1f510, -1, 66 },
    { BattleUnitType::WHITE_MAGIKOOPA, ModuleId::TOU2, 0x1f540, -1, 67 },
    { BattleUnitType::GREEN_MAGIKOOPA, ModuleId::TOU2, 0x1f570, -1, 68 },
    { BattleUnitType::GREEN_FUZZY, ModuleId::TOU2, 0x1f490, 8, 41 },
    { BattleUnitType::PALE_PIRANHA, ModuleId::TOU2, 0x1ef10, 21, 75 },
    { BattleUnitType::BIG_BANDIT, ModuleId::TOU2, 0x1f1b0, 1, 3 },
    { BattleUnitType::SWOOPER, ModuleId::TOU2, 0x1f790, 23, 88 },
    { BattleUnitType::BRISTLE, ModuleId::TOU2, 0x1f330, 18, 18 },
    { BattleUnitType::X_NAUT, ModuleId::AJI, 0x44914, 1, 70 },
    { BattleUnitType::ELITE_X_NAUT, ModuleId::AJI, 0x44894, 1, 72 },
    { BattleUnitType::X_NAUT_PHD, ModuleId::AJI, 0x44aa4, 27, 71 },
    { BattleUnitType::YUX, ModuleId::AJI, 0x44f44, 19, 94 },
    { BattleUnitType::Z_YUX, ModuleId::AJI, 0x44b24, 19, 95 },
    { BattleUnitType::X_YUX, ModuleId::AJI, 0x450d4, 19, 96 },
    // Non-Pit-native enemies (only used for specific loadouts).
    { BattleUnitType::GOOMBA, ModuleId::DOU, 0x163d8, 1, 43 },
    { BattleUnitType::EMBER, ModuleId::DOU, 0x16488, 25, 24 },
    { BattleUnitType::RED_BONES, ModuleId::LAS, 0x3c100, 12, 13 },
    { BattleUnitType::DARK_BONES, ModuleId::LAS, 0x3c1a0, 11, 15 },
    { BattleUnitType::BUZZY_BEETLE, ModuleId::JIN, 0x1b078, 6, 26 },
    { BattleUnitType::SPIKE_TOP, ModuleId::JIN, 0x1b148, -1, 27 },
    { BattleUnitType::SWOOPER, ModuleId::JIN, 0x1ab38, 23, 88 },
    { BattleUnitType::GREEN_FUZZY, ModuleId::MUJ, 0x358b8, 8, 41 },
    { BattleUnitType::PUTRID_PIRANHA, ModuleId::MUJ, 0x35ac8, 21, 76 },
    { BattleUnitType::EMBER, ModuleId::MUJ, 0x35218, 25, 24 },
    { BattleUnitType::GOOMBA, ModuleId::EKI, 0xff48, 1, 43 },
    { BattleUnitType::RUFF_PUFF, ModuleId::EKI, 0xfff8, 22, 82 },
};

const int32_t kPresetLoadouts[][5] = {
    { 6, 92, 34, 93, -1 },      // Bones
    { 88, 85, 87, 86, 89 },     // Yuxes + X-Nauts
    { 57, 58, 3, -1, -1 },      // Goomba, Hyper Goomba, Gloomba
    { 70, 21, 18, -1, -1 },     // Koopas
    { 58, 59, 60, -1, -1 },     // Hyper Goomba family
    { 3, 8, 13, -1, -1 },       // Gloomba family
    { 14, 81, 28, -1, -1 },     // Bandits
    { 5, 4, 43, -1, -1 },       // Spanias
    { 7, 79, 22, -1, -1 },      // Fuzzies
    { 9, 19, 32, -1, -1 },      // Clefts
    { 11, 101, 29, 48, -1 },    // Puffs
    { 71, 72, 23, -1, -1 },     // Paratroopas
    { 94, 95, 20, 27, -1 },     // Buzzy family
    { 25, 91, 40, -1, -1 },     // Embers
    { 38, 35, 47, -1, -1 },     // Wizzerds
    { 98, 36, 46, -1, -1 },     // Piranhas
    { 82, 41, 50, -1, -1 },     // Swoopers
    { 73, 74, 75, -1, -1 },     // Hammer Bros.
    { 10, 26, 10, 26, -1 },     // Pokeys
    { 83, 44, 83, 44, -1 },     // Bristles
    { 24, 49, 24, 49, -1 },     // Bob-ulks
    { 17, 30, 17, 30, -1 },     // Boos
    { 15, 33, 15, 33, -1 },     // Lakitus
    { 12, 45, 12, 45, -1 },     // Arantulas
    { 68, 39, 68, 39, -1 },     // Koopatrols
    { 42, 31, 42, 31, -1 },     // Chain Chomps
    { 68, 67, 66, -1, -1 },     // Koopatrol, Magikoopa, Hammer Bro
    { 84, 85, 86, -1, -1 },     // X-Nauts
    { 61, 2, 61, 2, 61 },       // Dayzees
};

// Base weights per floor group (00s, 10s, ...) and level_offset (2, 3, ... 10).
const int8_t kBaseWeights[11][9] = {
    { 10, 10, 5, 3, 2, 0, 0, 0, 0 },
    { 5, 10, 5, 5, 3, 0, 0, 0, 0 },
    { 3, 5, 10, 7, 5, 1, 0, 0, 0 },
    { 1, 1, 7, 10, 10, 3, 2, 0, 0 },
    { 1, 1, 5, 10, 10, 5, 3, 0, 0 },
    { 1, 1, 2, 5, 10, 10, 5, 1, 1 },
    { 0, 0, 2, 3, 10, 10, 6, 4, 2 },
    { 0, 0, 2, 3, 7, 10, 10, 5, 4 },
    { 0, 0, 1, 3, 6, 8, 10, 10, 8 },
    { 0, 0, 1, 2, 5, 7, 10, 10, 10 },
    { 0, 0, 1, 2, 5, 7, 10, 10, 10 },
};
// The target sum of enemy level_offsets for each floor group.
const int8_t kTargetLevelSums[11] = {
    12, 15, 18, 22, 25, 28, 31, 34, 37, 40, 50
};

// Global structures for holding constructed battle information.
int32_t g_NumEnemies = 0;
int32_t g_Enemies[5] = { -1, -1, -1, -1, -1 };
NpcSetupInfo g_CustomNpc[2];
BattleUnitSetup g_CustomUnits[6];
BattleGroupSetup g_CustomBattleParty;
int8_t g_CustomAudienceWeights[12];

// Look up the enemy type info w/matching unit_type (returns null if none found).
const EnemyTypeInfo* LookupEnemyTypeInfo(int32_t unit_type) {
    constexpr const int32_t kNumEnemyTypes =
        sizeof(kEnemyInfo) / sizeof(EnemyTypeInfo);
    const EnemyTypeInfo* ei = nullptr;
    for (int32_t i = 0; i < kNumEnemyTypes; ++i) {
        if (kEnemyInfo[i].unit_type == unit_type) {
            ei = kEnemyInfo + i;
            break;
        }
    }
    return ei;
}

}

ModuleId::e SelectEnemies(int32_t floor) {
    // Special cases: Floor X00 (Bonetail), X49 (Atomic Boo).
    if (floor % 100 == 99) {
        g_NumEnemies = 1;
        g_Enemies[0] = 0;
        for (int32_t i = 1; i < 4; ++i) g_Enemies[i] = -1;
        return ModuleId::INVALID_MODULE;
    }
    if (floor % 100 == 48) {
        g_NumEnemies = 1;
        g_Enemies[0] = 1;
        for (int32_t i = 1; i < 4; ++i) g_Enemies[i] = -1;
        return ModuleId::JIN;
    }
    // If a reward floor, no enemies to spawn.
    if (floor % 10 == 9) return ModuleId::INVALID_MODULE;
    
    const auto& pouch = *ttyd::mario_pouch::pouchGetPtr();
    auto& state = g_Randomizer->state_;
    
    // If floor > 50, determine whether to use one of the preset loadouts.
    if (floor >= 50 && state.Rand(100) < 15) {
        int32_t idx = state.Rand(sizeof(kPresetLoadouts) / (sizeof(int32_t)*5));
        // If the Bones or Yux variants loadout selected and player doesn't have
        // a damaging Star Power, use the Goomba / Koopa variants loadout.
        if (idx < 2 && !(pouch.star_powers_obtained & 0x92)) idx += 2;
        
        for (int32_t enemy = 0; enemy < 5; ++enemy) {
            g_Enemies[enemy] = kPresetLoadouts[idx][enemy];
        }
        // If only 3 enemies, occasionally mirror it across the center.
        if (g_Enemies[3] == -1) {
            // The higher the floor number, the more likely the 5-enemy version.
            if (static_cast<int32_t>(state.Rand(200)) < floor) {
                g_Enemies[3] = g_Enemies[1];
                g_Enemies[4] = g_Enemies[0];
            }
        }
    } else {
        // Select a background area.
        ModuleId::e secondary_area;
        if (floor < 30) {
            secondary_area = ModuleId::GON;
        } else {
            int32_t rn = state.Rand(floor < 50 ? 90 : 120);
            if (rn < 40) {
                secondary_area = ModuleId::TOU2;
            } else if (rn < 70) {
                secondary_area = ModuleId::TIK;
            } else if (rn < 90) {
                secondary_area = ModuleId::GRA;
            } else {
                secondary_area = ModuleId::AJI;
            }
        }
        
        // Put together an array of weights, scaled by the floor number and
        // enemy's level offset (such that harder enemies appear more later on).
        constexpr const int32_t kNumEnemyTypes = 
            sizeof(kEnemyModuleInfo) / sizeof(EnemyModuleInfo);
        int16_t weights[6][kNumEnemyTypes];
        for (int32_t i = 0; i < kNumEnemyTypes; ++i) {
            int32_t base_wt = 0;
            const EnemyModuleInfo& emi = kEnemyModuleInfo[i];
            const EnemyTypeInfo& ei = kEnemyInfo[emi.enemy_type_stats_idx];
            
            // If enemy is not in a loaded area, or is a special enemy, ignore.
            if (i >= 3 && ei.level_offset >= 2 && ei.level_offset <= 10 &&
                (emi.module == ModuleId::JON || emi.module == secondary_area)) {
                int32_t floor_group = floor < 110 ? floor / 10 : 10;
                base_wt = kBaseWeights[floor_group][ei.level_offset - 2];
                // Double the base weight if the enemy is from secondary area.
                if (emi.module == secondary_area && floor >= 30) base_wt <<= 1;
            }
            
            // Disable Yuxes and Dry Bones variants (other than Dull Bones)
            // if the player doesn't have a damaging Star Power, to reduce
            // the chance of a seed becoming essentially unwinnable.
            if (!(pouch.star_powers_obtained & 0x92)) {
                if (i == 34 || i == 56 || i == 92 || i == 93 ||
                    i == 87 || i == 88 || i == 89) {
                    base_wt = 0;
                }
            }
            
            // The 6th slot is used for reference as an unchanging base weight.
            for (int32_t slot = 0; slot < 6; ++slot) weights[slot][i] = base_wt;
            // Disable selecting enemies with no overworld behavior for slot 0.
            if (emi.npc_ent_type_info_idx == -1) weights[0][i] = 0;
        }
        
        // Pick enemies in weighted fashion, with preference towards repeats.
        int32_t level_sum = 0;
        const int32_t target_sum =
            floor < 100 ? kTargetLevelSums[floor / 10] : kTargetLevelSums[10];
        for (int32_t slot = 0; slot < 5; ++slot) {
            int32_t sum_weights = 0;
            for (int32_t i = 0; i < kNumEnemyTypes; ++i) 
                sum_weights += weights[slot][i];
            
            int32_t weight = state.Rand(sum_weights);
            int32_t idx = 0;
            for (; (weight -= weights[slot][idx]) >= 0; ++idx);
            
            g_Enemies[slot] = idx;
            const EnemyModuleInfo& emi = kEnemyModuleInfo[idx];
            level_sum += kEnemyInfo[emi.enemy_type_stats_idx].level_offset;
            
            // If level_sum is sufficiently high for the floor and not on the
            // fifth enemy, decide whether to add any further enemies.
            if (level_sum >= target_sum / 2 && slot < 4) {
                const int32_t end_chance = level_sum * 100 / target_sum;
                if (static_cast<int32_t>(state.Rand(100)) < end_chance) {
                    for (++slot; slot < 5; ++slot) g_Enemies[slot] = -1;
                    break;
                }
            }
            
            // Add large additional weight for repeat enemy in subsequent slots,
            // scaled by how likely it was to be chosen to begin with.
            for (int32_t j = slot + 1; j < 5; ++j) {
                weights[j][idx] += weights[5][idx] * 20;
            }
            // If the enemy is any Yux, set the next weight for all Yuxes to 0,
            // since they appear too crowded if placed 40 units apart.
            if (idx >= 87 && idx <= 89 && slot != 4) {
                weights[slot + 1][87] = 0;
                weights[slot + 1][88] = 0;
                weights[slot + 1][89] = 0;
            }
        }
        
        // If floor > 80, rarely insert an Amazy Dayzee in the loadout.
        if (floor >= 80 && state.Rand(100) < 5) {
            int32_t idx = 1;
            for (; idx < 5; ++idx) {
                if (g_Enemies[idx] == -1) break;
            }
            if (idx > 1) {
                g_Enemies[state.Rand(idx - 1) + 1] = 2;
            }
        }
    }
    
    // Count how many enemies are in the final party.
    for (g_NumEnemies = 0; g_NumEnemies < 5; ++g_NumEnemies) {
        if (g_Enemies[g_NumEnemies] == -1) break;
    }
    // Find out which secondary module needs to be loaded, if any.
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
    NpcSetupInfo** out_npc_setup_info, int32_t* out_lead_type) {

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
    
    // Set output variables.
    *out_npc_tribe_description = npc_tribe;
    *out_npc_setup_info = g_CustomNpc;
    *out_lead_type = enemy_module_info[0]->unit_type;
    
    // Construct the BattleGroupSetup from the previously selected enemies.
    
    // Return early if this is a Bonetail fight, since it needs no changes.
    if (floor % 100 == 99) return;
    
    auto& state = g_Randomizer->state_;
    
    for (int32_t i = 0; i < 12; ++i) g_CustomAudienceWeights[i] = 2;
    // Make Toads slightly likelier since they're never boosted.
    g_CustomAudienceWeights[0] = 3;
    for (int32_t i = 0; i < g_NumEnemies; ++i) {
        BattleUnitSetup& custom_unit = g_CustomUnits[i];
        memcpy(&custom_unit, unit_info[i], sizeof(BattleUnitSetup));
        
        // Special case: Goombas in modules GON and GRA need to be linked to
        // their correct unit_kind, since they weren't actually used in battles.
        int32_t etype = g_Enemies[i];
        if (etype == 51) {
            custom_unit.unit_kind_params =
                reinterpret_cast<BattleUnitKind*>(0x805ba9a0 + 0x1dcd8);
        } else if (etype == 57) {
            custom_unit.unit_kind_params =
                reinterpret_cast<BattleUnitKind*>(0x805ba9a0 + 0xa0e8);
        }
        
        // Make Swoopers never hang from ceiling, and Magikoopas sometimes fly,
        // but only if they're not the front enemy in the lineup.
        if (etype == 82 || etype == 96 || etype == 41 || etype == 50) {
            custom_unit.unit_work[0] = 1;
        } else if (etype == 76 || etype == 77 || etype == 78 || etype == 67) {
            custom_unit.unit_work[0] = state.Rand(i > 0 ? 2 : 1);
        }
        
        // Position the enemies in standard spacing.
        float offset = i - (g_NumEnemies - 1) * 0.5f;
        custom_unit.position.x = kEnemyPartyCenterX + offset * kEnemyPartySepX;
        custom_unit.position.z = offset * kEnemyPartySepZ;
        
        // If the enemy is related to a particular type of audience member,
        // increase that audience type's weight.
        if (enemy_info[i]->audience_type_boosted >= 0) {
            g_CustomAudienceWeights[enemy_info[i]->audience_type_boosted] += 2;
        }
    }
    g_CustomBattleParty.num_enemies         = g_NumEnemies;
    g_CustomBattleParty.enemy_data          = g_CustomUnits; 
    g_CustomBattleParty.random_item_weight  = 0;
    g_CustomBattleParty.no_item_weight      = 0;
    g_CustomBattleParty.hp_drop_table       = enemy_info[0]->hp_drop_table;
    g_CustomBattleParty.fp_drop_table       = enemy_info[0]->fp_drop_table;
    
    // Actually used as the index of the enemy whose item should be dropped.
    g_CustomBattleParty.held_item_weight = state.Rand(g_NumEnemies);
    
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
    if (floor >= 100) {
        battle_setup->flag_off_loadouts[0].stage_data =
            pit_battle_setups[50].flag_off_loadouts[0].stage_data;
    }
    // Set the battle's audience weights based on the custom ones set above.
    for (int32_t i = 0; i < 12; ++i) {
        auto& weights = battle_setup->audience_type_weights[i];
        weights.min_weight = g_CustomAudienceWeights[i];
        weights.max_weight = g_CustomAudienceWeights[i];
    }
}

// Stat weights as percentages for certain Pit floors (00s, 10s, 20s, ... 90s).
// After floor 100, HP, ATK and DEF rise by 5% every 10 floors.
const int8_t kStatPercents[10] = { 20, 25, 35, 40, 50, 55, 65, 75, 90, 100 };

bool GetEnemyStats(
    int32_t unit_type, int32_t* out_hp, int32_t* out_atk, int32_t* out_def,
    int32_t* out_level, int32_t* out_coinlvl, int32_t base_attack_power) {
    // Look up the enemy type info w/matching unit_type.
    const EnemyTypeInfo* ei = LookupEnemyTypeInfo(unit_type);
    if (!ei) return false;
    
    int32_t floor_group = g_Randomizer->state_.floor_ / 10;
    
    int32_t hp_scale = g_Randomizer->state_.GetOptionValue(
        RandomizerState::POST_100_HP_SCALING) ? 10 : 5;
    int32_t atk_scale = g_Randomizer->state_.GetOptionValue(
        RandomizerState::POST_100_ATK_SCALING) ? 10 : 5;
        
    int32_t base_hp_pct = 
        floor_group > 9 ?
            100 + (floor_group - 9) * hp_scale : kStatPercents[floor_group];
    int32_t base_atk_pct = 
        floor_group > 9 ?
            100 + (floor_group - 9) * atk_scale : kStatPercents[floor_group];
    int32_t base_def_pct = 
        floor_group > 9 ?
            100 + (floor_group - 9) * 5 : kStatPercents[floor_group];
            
    if (out_hp) {
        int32_t hp = Min(ei->hp_scale * base_hp_pct, 1000000);
        hp *= g_Randomizer->state_.hp_multiplier_;
        *out_hp = Clamp((hp + 5000) / 10000, 1, 9999);
    }
    if (out_atk) {
        int32_t atk = Min(ei->atk_scale * base_atk_pct, 1000000);
        atk += (base_attack_power - ei->atk_base) * 100;
        atk *= g_Randomizer->state_.atk_multiplier_;
        *out_atk = Clamp((atk + 5000) / 10000, 1, 99);
    }
    if (out_def) {
        if (ei->def_scale == 0) {
            *out_def = 0;
        } else {
            // Enemies with def_scale > 0 should always have at least 1 DEF.
            int32_t def = (ei->def_scale * base_def_pct + 50) / 100;
            def = def < 1 ? 1 : def;
            *out_def = def > 99 ? 99 : def;
        }
    }
    if (out_level) {
        if (ttyd::mario_pouch::pouchGetPtr()->level >= 99) {
            *out_level = 0;
        } else if (ei->level_offset == 0) {
            // Enemies like Mini-Yuxes should never grant EXP.
            *out_level = 0;
        } else {
            // Enemies' level will always be the same relative to than Mario,
            // typically giving 3 ~ 10 EXP depending on strength and group size.
            // (The EXP gained will reduce slightly after floor 100.)
            if (ei->level_offset <= 10) {
                int32_t level_offset = 
                    ei->level_offset + (g_Randomizer->state_.floor_ < 100 ? 5 : 2);
                *out_level =
                    ttyd::mario_pouch::pouchGetPtr()->level + level_offset;
            } else {
                // Bosses / special enemies get fixed bonus Star Points instead.
                *out_level = -ei->level_offset / 2;
            }
        }
    }
    if (out_coinlvl) {
        // "Coin level" = expected number of coins x 2.
        // Return level_offset for normal enemies (lv 2 ~ 10 / 1 ~ 5 coins),
        // or 20 (10 coins) for boss / special enemies.
        *out_coinlvl = ei->level_offset > 10 ? 20 : ei->level_offset;
    }
    
    return true;
}

char g_TattleTextBuf[512];

const char* GetCustomTattle() { return g_TattleTextBuf; }

const char* SetCustomTattle(
    BattleWorkUnit* unit, const char* original_tattle_msg) {
    int32_t unit_type = unit->current_kind;
    const EnemyTypeInfo* ei = LookupEnemyTypeInfo(unit_type);
    if (ei == nullptr) {
        // No stats to pull from; just use the original message.
        return original_tattle_msg;
    }
    // Take the first paragraph from the original tattle 
    // (ignore the first few characters in case there's a <p> there).
    const char* original_tattle = ttyd::msgdrv::msgSearch(original_tattle_msg);
    const char* p1_end_ptr = strstr(original_tattle + 4, "<p>");
    int32_t p1_len =
        p1_end_ptr ? p1_end_ptr - original_tattle : strlen(original_tattle);
    strncpy(g_TattleTextBuf, original_tattle, p1_len);
    
    // Append a paragraph with the enemy's base stats.
    char* p2_ptr = g_TattleTextBuf + p1_len;
    char atk_offset_buf[8];
    sprintf(atk_offset_buf, " (%+" PRId16 ")", ei->atk_offset);
    sprintf(p2_ptr,
            "<p>Its base stats are:\n"
            "Max HP: %" PRId16 ", ATK: %" PRId16 "%s,\n"
            "DEF: %" PRId16 ", Level: %" PRId16 ".\n<k>",
            ei->hp_scale, ei->atk_scale, ei->atk_offset ? atk_offset_buf : "",
            ei->def_scale, ei->level_offset);
    
    // Append one more paragraph with the enemy's current stats
    // (using its standard attack's power as reference for ATK).
    int32_t hp, atk, def;
    int32_t base_atk_power = ei->atk_offset + ei->atk_base;
    if(GetEnemyStats(
        unit_type, &hp, &atk, &def, nullptr, nullptr, base_atk_power)) {
        char* p3_ptr = g_TattleTextBuf + strlen(g_TattleTextBuf);
        sprintf(p3_ptr,
                "<p>Currently, its stats are:\n"
                "Max HP: %" PRId32 ", ATK: %" PRId32 ", DEF: %" PRId32 ".\n<k>",
                hp, atk, def);
    }
    
    // Return a key that looks up g_TattleTextBuf from randomizer_strings.
    return "custom_tattle_battle";
}

const char* SetCustomMenuTattle(const char* original_tattle_msg) {
    const auto* kTattleInfo = 
        ttyd::battle_monosiri::battleGetUnitMonosiriPtr(0);
        
    // Look for the enemy type with a matching message name.
    bool found_match = false;
    int32_t unit_type = 1;
    for (; unit_type <= BattleUnitType::BONETAIL; ++unit_type) {
        const char* tattle = kTattleInfo[unit_type].menu_tattle;
        if (tattle && !strcmp(tattle, original_tattle_msg)) {
            found_match = true;
            break;
        }
    }
    if (!found_match) return original_tattle_msg;
    
    // Print a simple base stat string to g_TattleTextBuf.
    const EnemyTypeInfo* ei = LookupEnemyTypeInfo(unit_type);
    if (ei) {
        char atk_offset_buf[8];
        sprintf(atk_offset_buf, " (%+" PRId16 ")", ei->atk_offset);
        sprintf(g_TattleTextBuf,
                "Base HP: %" PRId16 ", Base ATK: %" PRId16 "%s,\n"
                "Base DEF: %" PRId16 ", Level: %" PRId16 "",
                ei->hp_scale, ei->atk_scale, 
                ei->atk_offset ? atk_offset_buf : "",
                ei->def_scale, ei->level_offset);
    } else {
        sprintf(g_TattleTextBuf, "No info known on this enemy.");
    }
    
    // Return a key that looks up g_TattleTextBuf from randomizer_strings.
    return "custom_tattle_menu";
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
    { "Use fewer than %" PRId32 " Jump moves!", JUMP_LESS, 2, 3 },
    { "Don't ever use Hammer moves!", HAMMER_LESS, 1, -1 },
    { "Use fewer than %" PRId32 " Hammer moves!", HAMMER_LESS, 2, 3 },
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
    { "Have Mario attack an audience member!", MARIO_ATTACK_AUDIENCE_MORE, 1, -1, 3 },
    { "Appeal to the crowd at least %" PRId32 " times!", APPEAL_MORE, 3, 5 },
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
    // Set conditions on about 1 in 4 battles randomly (and not on Bonetail).
    RandomizerState& state = g_Randomizer->state_;
    if (state.floor_ % 10 == 9 || state.Rand(4)) return;
    
    // Use the unused "random_item_weight" field to store the item reward.
    int32_t* item_reward = &npc_info->pConfiguration->random_item_weight;
    *item_reward = PickRandomItem(
        /* seeded = */ true, 10, 10, 40, state.floor_ < 30 ? 0 : 30);
    // If the "none" case was picked, make it a Shine Sprite.
    if (*item_reward <= 0) *item_reward = ItemType::GOLD_BAR_X3;
    
    // Make a copy of the conditions array so the weights can be mutated.
    constexpr const int32_t kNumConditions = 
        sizeof(kBattleConditions) / sizeof(BattleCondition);
    BattleCondition conditions[kNumConditions];
    memcpy(conditions, kBattleConditions, sizeof(kBattleConditions));
    
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
        case MARIO_ATTACK_AUDIENCE_MORE:
            // Guarantee a Shine Sprite.
            *item_reward = ItemType::GOLD_BAR_X3;
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
    auto* fbat_info = ttyd::battle::g_BattleWork->fbat_info;
    const int32_t item_reward =
        fbat_info->wBattleInfo->pConfiguration->random_item_weight;
    const char* item_name = ttyd::msgdrv::msgSearch(
        ttyd::item_data::itemDataTable[item_reward].name);
    sprintf(out_buf, "Bonus reward (%s):\n%s", item_name, g_ConditionTextBuf);
}

int32_t PickRandomItem(
    bool seeded, int32_t normal_item_weight, int32_t recipe_item_weight,
    int32_t badge_weight, int32_t no_item_weight, bool force_no_partner) {
    // Bitfields of whether each item is included in the pool or not;
    // item X is enabled if kItemPool[X / 16 - offset] & (1 << (X % 16)) != 0.
    static constexpr const uint16_t kNormalItems[] = {
        0xffff, 0xffff, 0x000f, 0x0006
    };
    static constexpr const uint16_t kRecipeItems[] = {
        0x2020, 0xffb8, 0x3edf, 0x97ef, 0x0bff
    };
    static constexpr const uint16_t kStackableBadges[] = {
        0x3fff, 0xffff, 0x0fff, 0xfff7, 0x018f, 0x0030, 0x0006
    };
    static constexpr const uint16_t kStackableBadgesNoP[] = {
        0x3fff, 0x5555, 0x0b55, 0xaad7, 0x0186, 0x0030, 0x0002
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
                // Count available partners.
                const PouchData& pouch = *ttyd::mario_pouch::pouchGetPtr();
                int32_t num_partners = 0;
                for (int32_t i = 0; i < 8; ++i) {
                    num_partners += pouch.party_data[i].flags & 1;
                }
                // Exclude 'P' badges if no partners unlocked yet.
                bitfield = (num_partners && !force_no_partner)
                    ? kStackableBadges : kStackableBadgesNoP;
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
        // Mario / inventory upgrades (items 0 - 4).
        ItemType::STRANGE_SACK, ItemType::SUPER_BOOTS, ItemType::ULTRA_BOOTS,
        ItemType::SUPER_HAMMER, ItemType::ULTRA_HAMMER,
        // Star Powers (items 5 - 12).
        ItemType::MAGICAL_MAP, ItemType::DIAMOND_STAR, ItemType::EMERALD_STAR,
        ItemType::GOLD_STAR, ItemType::RUBY_STAR, ItemType::SAPPHIRE_STAR,
        ItemType::GARNET_STAR, ItemType::CRYSTAL_STAR,
        // Unique badges (items 13 - 24).
        ItemType::CHILL_OUT, ItemType::DOUBLE_DIP, ItemType::DOUBLE_DIP,
        ItemType::DOUBLE_DIP_P, ItemType::DOUBLE_DIP_P, ItemType::FEELING_FINE,
        ItemType::FEELING_FINE_P, ItemType::LUCKY_START, ItemType::QUICK_CHANGE,
        ItemType::RETURN_POSTAGE, ItemType::ZAP_TAP, ItemType::SPIKE_SHIELD,
        // Partners (represented by dummy values); (items 25 - 31).
        -1, -2, -3, -4, -5, -6, -7
    };
    static_assert(sizeof(kRewards) == 32 * sizeof(int16_t));
    
    uint8_t weights[34];
    for (int32_t i = 0; i < 32; ++i) weights[i] = 10;
    
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
        // Set Strange Sack's weight to be decently high.
        weights[0] = 40;
        // Set Mario's upgrade weights to be moderately likely.
        for (int32_t i = 1; i < 4; ++i) weights[i] = 15;
        
        // Determine the weight for a partner based on how many you have
        // currently and how deep in the Pit you are.
        int32_t partner_weight = 0;
        if (num_partners < 7) {
            partner_weight = 10;
            if (state.floor_ > 30) {
                // Decrease average weight, but increase total considerably 
                // if behind expected count (2 by floor 50, 3 by floor 70, etc.)
                partner_weight = 5;
                int32_t expected_partners = (state.floor_ - 9) / 20;
                if (num_partners < expected_partners) {
                    partner_weight += 
                        30 * (expected_partners - num_partners) 
                           / (7 - num_partners);
                }
                if (partner_weight > 100) partner_weight = 100;
            }
        }
        
        // Determine the weight for Crystal Stars based on how many are yet to
        // be obtained; the total weight should be 10x unobtained count, split
        // among all the ones that can be afforded.
        int32_t sp_weight = 0;
        const int32_t number_star_powers = state.StarPowersObtained();
        switch (8 - number_star_powers) {
            case 8: sp_weight = 80 / 1; break;
            case 7: sp_weight = 70 / 2; break;
            case 6: sp_weight = 60 / 2; break;
            case 5: sp_weight = 50 / 4; break;
            case 4: sp_weight = 40 / 3; break;
            default:    sp_weight = 10; break;
        }
        for (int32_t i = 5; i <= 12; ++i) weights[i] = sp_weight;
        
        // The unique badges should take up about as much weight as Star Powers;
        // this makes them less likely than the average reward early on but
        // more likely the fewer there are remaining.
        int32_t num_unique_badges_remaining =
            12 - CountSetBits(GetBitMask(13, 24) & state.reward_flags_);
        int32_t unique_badge_weight = 15 + 5 * num_unique_badges_remaining;
        if (num_unique_badges_remaining > 0) {
            for (int32_t i = 13; i <= 24; ++i) {
                weights[i] = unique_badge_weight / num_unique_badges_remaining;
            }
            // Make Spike Shield specifically a bit more likely.
            weights[24] *= 3;
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
                    if (pouch.jump_level < 2) {
                        weights[i] = 0;
                        weights[i-1] += 10;     // Make Super a bit more likely.
                    }
                    break;
                case ItemType::ULTRA_HAMMER:
                    if (pouch.hammer_level < 2) {
                        weights[i] = 0;
                        weights[i-1] += 10;     // Make Super a bit more likely.
                    }
                    break;
                case ItemType::DIAMOND_STAR:
                case ItemType::EMERALD_STAR:
                    if (number_star_powers < 1) weights[i] = 0;
                    break;
                case ItemType::GOLD_STAR:
                    if (number_star_powers < 2) weights[i] = 0;
                    break;
                case ItemType::RUBY_STAR:
                case ItemType::SAPPHIRE_STAR:
                case ItemType::GARNET_STAR:
                    if (number_star_powers < 3) weights[i] = 0;
                    break;
                case ItemType::CRYSTAL_STAR:
                    if (number_star_powers < 5) weights[i] = 0;
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
        weights[32] = state.floor_ > 30 ? (state.floor_ < 100 ? 20 : 10) : 0;
        weights[33] = state.floor_ < 100 ? 10 : 20;
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
    
    // If partners were unlocked from beginning and a partner was selected,
    // replace the reward with an extra Shine Sprite.
    if (reward < 0 && g_Randomizer->state_.GetOptionValue(
        RandomizerState::START_WITH_PARTNERS)) {
        reward = ItemType::GOLD_BAR_X3;
    }
    
    return reward;
}

}