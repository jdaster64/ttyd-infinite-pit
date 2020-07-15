#pragma once

#include <gc/types.h>

#include <cstdint>

namespace ttyd::battle_database_common {

struct BattleWeapon {
    const char* name;
    int16_t     icon;
    int8_t      unk_06[2];
    int32_t     item_id;
    const char* description;
    int8_t      base_accuracy;
    int8_t      base_fp_cost;
    int8_t      base_sp_cost;
    int8_t      superguards_allowed;  // not sure of difference between 1 and 2
    float       unk_14;
    int8_t      stylish_multiplier;
    int8_t      unk_19;
    int8_t      bingo_card_chance;
    int8_t      unk_1b;
    void*       damage_function;
    uint32_t    damage_function_params[8];
    void*       fp_damage_function;
    uint32_t    fp_damage_function_params[8];
    uint32_t    target_class_flags;
    uint32_t    target_property_flags;
    int8_t      element;
    int8_t      damage_pattern;     // used for hit effects (spinning, crushing...)
    int8_t      weapon_ac_level;
    int8_t      unk_6f;
    const char* ac_help_msg;
    uint32_t    special_property_flags;
    uint32_t    counter_resistance_flags;
    uint32_t    target_weighting_flags;

    int8_t      sleep_chance;
    int8_t      sleep_time;
    int8_t      stop_chance;
    int8_t      stop_time;
    int8_t      dizzy_chance;
    int8_t      dizzy_time;
    int8_t      poison_chance;
    int8_t      poison_time;
    
    int8_t      poison_strength;
    int8_t      confuse_chance;
    int8_t      confuse_time;
    int8_t      electric_chance;
    int8_t      electric_time;
    int8_t      dodgy_chance;
    int8_t      dodgy_time;
    int8_t      burn_chance;
    
    int8_t      burn_time;
    int8_t      freeze_chance;
    int8_t      freeze_time;
    int8_t      size_change_change;
    int8_t      size_change_time;
    int8_t      size_change_strength;
    int8_t      atk_change_chance;
    int8_t      atk_change_time;
    
    int8_t      atk_change_strength;
    int8_t      def_change_chance;
    int8_t      def_change_time;
    int8_t      def_change_strength;
    int8_t      allergic_chance;
    int8_t      allergic_time;
    int8_t      ohko_chance;
    int8_t      charge_strength;
    
    int8_t      fast_chance;
    int8_t      fast_time;
    int8_t      slow_chance;
    int8_t      slow_time;
    int8_t      fright_chance;
    int8_t      gale_force_chance;
    int8_t      payback_time;
    int8_t      hold_fast_time;
    
    int8_t      invisible_chance;
    int8_t      invisible_time;
    int8_t      hp_regen_time;
    int8_t      hp_regen_strength;
    int8_t      fp_regen_time;
    int8_t      fp_regen_strength;
    int8_t      pad_ae[2];
    
    void*       attack_evt_code;
    int8_t      bg_a1_a2_fall_weight;
    int8_t      bg_a1_fall_weight;
    int8_t      bg_a2_fall_weight;
    int8_t      bg_no_a_fall_weight;
    int8_t      bg_b_fall_weight;
    int8_t      nozzle_turn_chance;
    int8_t      nozzle_fire_chance;
    int8_t      ceiling_fall_chance;
    int8_t      object_fall_chance;
    int8_t      unused_stage_hazard_chance;
    int8_t      pad_be[2];
} __attribute__((__packed__));

static_assert(sizeof(BattleWeapon) == 0xc0);

struct ItemDropData {
    int32_t item_id;
    int16_t hold_weight;
    int16_t drop_weight;
} __attribute__((__packed__));

static_assert(sizeof(ItemDropData) == 0x8);
    
struct PointDropData {
    int32_t max_stat_percent;
    int32_t overall_drop_rate;
    int32_t drop_count;
    int32_t individual_drop_rate;
} __attribute__((__packed__));

static_assert(sizeof(PointDropData) == 0x10);

struct StatusVulnerability {
    uint8_t sleep;
    uint8_t stop;
    uint8_t dizzy;
    uint8_t poison;
    uint8_t confuse;
    uint8_t electric;
    uint8_t burn;
    uint8_t freeze;
    uint8_t huge;
    uint8_t tiny;
    uint8_t attack_up;
    uint8_t attack_down;
    uint8_t defense_up;
    uint8_t defense_down;
    uint8_t allergic;
    uint8_t fright;
    uint8_t gale_force;
    uint8_t fast;
    uint8_t slow;
    uint8_t dodgy;
    uint8_t invisible;
    uint8_t ohko;
} __attribute__((__packed__));

static_assert(sizeof(StatusVulnerability) == 0x16);

struct BattleUnitKindPart {
    int32_t     index;  // one-indexed
    const char* name;
    const char* model_name;
    gc::vec3    part_offset_pos;
    gc::vec3    part_hit_base_offset;
    gc::vec3    part_hit_cursor_base_offset;
    int16_t     unk_30;
    int16_t     unk_32;
    int16_t     base_alpha;
    int16_t     pad_36;
    // Default status when spawning a unit of this kind.
    int8_t*     defense;
    int8_t*     defense_attr;
    uint32_t    attribute_flags;
    uint32_t    counter_attribute_flags;
    void*       pose_table;
} __attribute__((__packed__));

static_assert(sizeof(BattleUnitKindPart) == 0x4c);

struct BattleUnitKind {
    int32_t     unit_type;
    const char* unit_name;
    int16_t     max_hp;
    int16_t     max_fp;
    int8_t      danger_hp;
    int8_t      peril_hp;
    int8_t      level;
    int8_t      bonus_exp;
    int8_t      bonus_coin;
    int8_t      bonus_coin_rate;
    int8_t      base_coin;
    int8_t      run_rate;
    int16_t     pb_soft_cap;
    int16_t     width;
    int16_t     height;
    int16_t     hit_offset[2];
    int8_t      pad_1e[2];
    gc::vec3    center_offset;
    int16_t     hp_gauge_offset[2];
    gc::vec3    talk_toge_base_offset;
    gc::vec3    held_item_base_offset;
    gc::vec3    burn_flame_offset;
    float       unk_54;
    float       unk_58;
    gc::vec3    binta_hit_offset;   // Love Slap, other grounded moves
    gc::vec3    kiss_hit_offset;    // Lip Lock
    gc::vec3    cut_base_offset;    // Art Attack
    float       cut_width;
    float       cut_height;
    int8_t      turn_order;
    int8_t      turn_order_variance;
    int8_t      swallow_chance;
    int8_t      swallow_attributes;
    int8_t      hammer_knockback_chance;
    int8_t      itemsteal_param;
    int8_t      pad_8e[2];
    gc::vec3    star_point_disp_offset;
    const char* damage_sfx_name;
    const char* fire_damage_sfx_name;
    const char* ice_damage_sfx_name;
    const char* explosion_damage_sfx_name;
    // Default attribute_flags & status chances.
    uint32_t    attribute_flags;
    StatusVulnerability* status_vulnerability;
    int8_t      num_parts;
    int8_t      pad_b5[3];
    BattleUnitKindPart* parts;
    void*       init_evt_code;
    void*       data_table;
} __attribute__((__packed__));

static_assert(sizeof(BattleUnitKind) == 0xc4);

struct BattleUnitSetup {
    BattleUnitKind* unit_kind_params;
    int8_t          alliance;
    int8_t          pad_05[3];
    int32_t         attack_phase;
    gc::vec3        position;
    uint32_t        addl_target_offset_x;
    uint32_t        unit_work[4];
    ItemDropData*   item_drop_table;        // Zero-terminated
} __attribute__((__packed__));

static_assert(sizeof(BattleUnitSetup) == 0x30);
    
struct BattleGroupSetup {
    int32_t         num_enemies;
    BattleUnitSetup* enemy_data;
    int32_t         held_item_weight;
    int32_t         random_item_weight;
    int32_t         no_item_weight;
    PointDropData*  hp_drop_table;
    PointDropData*  fp_drop_table;
    uint32_t        unk_1c;    
} __attribute__((__packed__));

static_assert(sizeof(BattleGroupSetup) == 0x20);

struct BattleStageObjectData {
    const char* name;
    int16_t     type;  // Generally 2; 0, 1 used for unused actors, 5 for Glitzville ceiling
    int16_t     layer;
    gc::vec3    position;
    int8_t      num_frames_before_falling;
    int8_t      num_frames_to_fall;
    int8_t      pad_16[2];
} __attribute__((__packed__));

static_assert(sizeof(BattleStageObjectData) == 0x18);

struct BattleStageData {
    const char*     global_stage_data_dir;      // ?
    const char*     current_stage_data_dir;     // ?
    int32_t         num_props;
    BattleStageObjectData* props;
    BattleWeapon    bg_a_weapon;
    BattleWeapon    bg_b_weapon;
    void*           init_evt_code;
    void*           destroy_bg_a1_evt_code;
    void*           destroy_bg_a2_evt_code;
    void*           destroy_bg_b_evt_code;
    void*           bg_a1_evt_code;
    void*           bg_a2_evt_code;
    void*           bg_b_scroll_evt_code;
    void*           bg_b_rotate_evt_code;
    uint8_t         unk_1b0;
    uint8_t         disallow_destroying_bg_a1;
    uint8_t         disallow_destroying_bg_a2;
    uint8_t         disallow_destroying_bg_b;
} __attribute__((__packed__));

static_assert(sizeof(BattleStageData) == 0x1b4);
    
struct BattleSetupWeightedLoadout {
    int32_t             weight;
    BattleGroupSetup*   group_data;
    BattleStageData*    stage_data;
} __attribute__((__packed__));

static_assert(sizeof(BattleSetupWeightedLoadout) == 0xc);
    
struct AudienceTypeWeights {
    int8_t min_weight;
    int8_t max_weight;
} __attribute__((__packed__));

static_assert(sizeof(AudienceTypeWeights) == 0x2);

struct BattleSetupData {
    const char*     battle_name;
    const char*     secondary_name;  // often a room code
    uint32_t        unk_08;
    int32_t         different_loadouts_flag;
    BattleSetupWeightedLoadout* flag_on_loadouts;
    BattleSetupWeightedLoadout* flag_off_loadouts;
    // 0x10 = cannot flee
    // 0x20 = if set, makes ATK/DEF Merlee curses more frequent
    // 0x40 = enforces BtlActRec conditions?
    // 0x1000'0000 = set on Pit battles; disables First/Bump Attack
    uint32_t        battle_setup_flags;
    // 0 = normal; others are used for bosses w/set audience makeup
    uint32_t        audience_setting_mode;
    AudienceTypeWeights audience_type_weights[16];
    const char*     music_name;    
} __attribute__((__packed__));

static_assert(sizeof(BattleSetupData) == 0x44);

extern "C" {
    
// .data
// All actually arrays.
extern ItemDropData battle_item_NONE;
extern ItemDropData battle_item_gundan_zako;
extern ItemDropData battle_item_gundan_zako_magician;
extern ItemDropData battle_item_gundan_zako_elite;
extern ItemDropData battle_item_hinnya;
extern ItemDropData battle_item_hannya;
extern ItemDropData battle_item_hennya;
extern ItemDropData battle_item_kuriboo;
extern ItemDropData battle_item_togekuri;
extern ItemDropData battle_item_patakuri;
extern ItemDropData battle_item_hyper_kuriboo;
extern ItemDropData battle_item_hyper_togekuri;
extern ItemDropData battle_item_hyper_patakuri;
extern ItemDropData battle_item_yami_kuriboo;
extern ItemDropData battle_item_yami_togekuri;
extern ItemDropData battle_item_yami_patakuri;
extern ItemDropData battle_item_nokonoko;
extern ItemDropData battle_item_patapata;
extern ItemDropData battle_item_nokonoko_fighter;
extern ItemDropData battle_item_patapata_fighter;
extern ItemDropData battle_item_yami_noko;
extern ItemDropData battle_item_ura_noko;
extern ItemDropData battle_item_ura_pata;
extern ItemDropData battle_item_togenoko;
extern ItemDropData battle_item_togenoko_ace;
extern ItemDropData battle_item_hammer_bros;
extern ItemDropData battle_item_boomerang_bros;
extern ItemDropData battle_item_fire_bros;
extern ItemDropData battle_item_met;
extern ItemDropData battle_item_togemet;
extern ItemDropData battle_item_patamet;
extern ItemDropData battle_item_patatogemet;
extern ItemDropData battle_item_hyper_togemet;
extern ItemDropData battle_item_jyugem;
extern ItemDropData battle_item_hyper_jyugem;
extern ItemDropData battle_item_togezo;
extern ItemDropData battle_item_kamec;
extern ItemDropData battle_item_chorobon;
extern ItemDropData battle_item_green_chorobon;
extern ItemDropData battle_item_flower_chorobon;
extern ItemDropData battle_item_honenoko;
extern ItemDropData battle_item_red_honenoko;
extern ItemDropData battle_item_karon;
extern ItemDropData battle_item_monochrome_kurokumorn;
extern ItemDropData battle_item_kurokumorn;
extern ItemDropData battle_item_bllizard;
extern ItemDropData battle_item_dokugassun;
extern ItemDropData battle_item_pakkun_flower;
extern ItemDropData battle_item_poison_pakkun;
extern ItemDropData battle_item_ice_pakkun;
extern ItemDropData battle_item_monochrome__pakkun;
extern ItemDropData battle_item_piders;
extern ItemDropData battle_item_churantalar;
extern ItemDropData battle_item_barriern;
extern ItemDropData battle_item_barriern_z;
extern ItemDropData battle_item_barriern_custom;
extern ItemDropData battle_item_monochrome_sinemon;
extern ItemDropData battle_item_sinemon;
extern ItemDropData battle_item_hyper_sinnosuke;
extern ItemDropData battle_item_hyper_sinemon;
extern ItemDropData battle_item_iron_sinemon;
extern ItemDropData battle_item_iron_sinemon2;
extern ItemDropData battle_item_basabasa;
extern ItemDropData battle_item_basabasa_chururu;
extern ItemDropData battle_item_basabasa_green;
extern ItemDropData battle_item_teresa;
extern ItemDropData battle_item_purple_teresa;
extern ItemDropData battle_item_atmic_teresa;
extern ItemDropData battle_item_sambo;
extern ItemDropData battle_item_sambo_mummy;
extern ItemDropData battle_item_killer;
extern ItemDropData battle_item_super_killer;
extern ItemDropData battle_item_killer_cannon;
extern ItemDropData battle_item_super_killer_cannon;
extern ItemDropData battle_item_bomhei;
extern ItemDropData battle_item_heavy_bom;
extern ItemDropData battle_item_giant_bomb;
extern ItemDropData battle_item_borodo;
extern ItemDropData battle_item_borodo_king;
extern ItemDropData battle_item_badge_borodo;
extern ItemDropData battle_item_mahorn;
extern ItemDropData battle_item_super_mahorn;
extern ItemDropData battle_item_mahorn_custom;
extern ItemDropData battle_item_wanwan;
extern ItemDropData battle_item_burst_wanwan;
extern ItemDropData battle_item_bubble;
extern ItemDropData battle_item_hermos;
extern ItemDropData battle_item_phantom;
extern ItemDropData battle_item_pansy;
extern ItemDropData battle_item_twinkling_pansy;
extern ItemDropData battle_item_dark_keeper;
extern ItemDropData battle_item_monban;
extern ItemDropData battle_item_gold_chorobon;
extern ItemDropData battle_item_sinnosuke;
extern ItemDropData battle_item_togedaruma;
extern ItemDropData battle_item_yamitogedaruma;
extern ItemDropData battle_item_vivian1;
extern ItemDropData battle_item_gesso;
extern ItemDropData battle_item_gonbaba;
extern ItemDropData battle_item_moamoa;
extern ItemDropData battle_item_rampell;
extern ItemDropData battle_item_faker_mario;
extern ItemDropData battle_item_cortez;
extern ItemDropData battle_item_kurikuri;
extern ItemDropData battle_item_magnum_battender;
extern ItemDropData battle_item_macho_gance;
extern ItemDropData battle_item_champion;
extern ItemDropData battle_item_kanbu1;
extern ItemDropData battle_item_zonbaba;
extern ItemDropData battle_item_default_param_list;
extern PointDropData battle_heart_drop_param_default;
extern PointDropData battle_heart_drop_param_default2;
extern PointDropData battle_heart_drop_param_default3;
extern PointDropData battle_heart_drop_param_default4;
extern PointDropData battle_heart_drop_param_default5;
extern PointDropData battle_flower_drop_param_default;
extern PointDropData battle_flower_drop_param_default2;
extern PointDropData battle_flower_drop_param_default3;
extern PointDropData battle_flower_drop_param_default4;
extern PointDropData battle_flower_drop_param_default5;

}

}