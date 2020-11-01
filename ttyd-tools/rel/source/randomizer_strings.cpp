#include "randomizer_strings.h"

#include "randomizer.h"
#include "randomizer_data.h"
#include "randomizer_state.h"

#include <ttyd/mario_pouch.h>
#include <ttyd/mariost.h>

#include <cinttypes>
#include <cstdio>
#include <cstring>

namespace mod::pit_randomizer {

// Enum representing indices of various strings in kKeyLookups.
namespace MsgKey {
    enum e {
        BTL_HLP_CMD_OPERATION_SUPER_CHARGE = 0,
        CUSTOM_TATTLE_BATTLE,
        CUSTOM_TATTLE_MENU,
        IN_2BAI_DAMAGE,
        IN_CAKE,
        IN_TOUGHEN_UP,
        IN_TOUGHEN_UP_P,
        LIST_ICE_CANDY,
        LIST_NANCY_FRAPPE,
        MENU_2BAI_DAMAGE,
        MENU_CHARGE,
        MENU_CHARGE_P,
        MENU_DAMAGE_FLOWER,
        MENU_DAMAGE_FLOWER_P,
        MENU_DAMAGE_GAESHI,
        MENU_FIRE_NAGURI,
        MENU_ICE_NAGURI,
        MENU_KIKEN_DE_POWER,
        MENU_KIKEN_DE_POWER_P,
        MENU_PINCH_DE_GANBARU,
        MENU_PINCH_DE_GANBARU_P,
        MENU_TAMATSUKI_JUMP,
        MENU_TSURANUKI_NAGURI,
        MSG_2BAI_DAMAGE,
        MSG_CAKE,
        MSG_CUSTOM_SUPER_BOOTS,
        MSG_CUSTOM_SUPER_HAMMER,
        MSG_CUSTOM_ULTRA_BOOTS,
        MSG_CUSTOM_ULTRA_HAMMER,
        MSG_DAMAGE_FLOWER,
        MSG_DAMAGE_FLOWER_P,
        MSG_DAMAGE_GAESHI,
        MSG_ICE_CANDY,
        MSG_JON_KANBAN_1,
        MSG_JON_KANBAN_2,
        MSG_JON_KANBAN_3,
        MSG_KAME_NO_NOROI,
        MSG_KIKEN_DE_POWER,
        MSG_KIKEN_DE_POWER_P,
        MSG_NANCY_FRAPPE,
        MSG_PINCH_DE_GANBARU,
        MSG_PINCH_DE_GANBARU_P,
        MSG_PKR_MONOSIRI,
        MSG_PTR_MEROMERO_KISS,
        MSG_PWD_KUMOGAKURE,
        MSG_PYS_NOMIKOMI,
        MSG_SHIKAESHI_NO_KONA,
        MSG_SUPER_COIN,
        MSG_TEKI_KYOUKA,
        MSG_TOUGHEN_UP,
        MSG_TOUGHEN_UP_MENU,
        MSG_TOUGHEN_UP_P,
        MSG_TOUGHEN_UP_P_MENU,
        PIT_CHARLIETON_FULL_INV,
        PIT_CHEST_UNCLAIMED,
        PIT_DISABLED_RETURN,
        PIT_REWARD_PARTY_JOIN,
        TIK_06_02,
    };
}
    
namespace {

// Keys for strings to be replaced / added to msgSearch. Should be kept
// in sync with the above enum, and always maintain alphabetical order.
constexpr const char* kKeyLookups[] = {
    "btl_hlp_cmd_operation_super_charge",
    "custom_tattle_battle",
    "custom_tattle_menu",
    "in_2bai_damage",
    "in_cake",
    "in_toughen_up",
    "in_toughen_up_p",
    "list_ice_candy",
    "list_nancy_frappe",
    "menu_2bai_damage",
    "menu_charge",
    "menu_charge_p",
    "menu_damage_flower",
    "menu_damage_flower_p",
    "menu_damage_gaeshi",
    "menu_fire_naguri",
    "menu_ice_naguri",
    "menu_kiken_de_power",
    "menu_kiken_de_power_p",
    "menu_pinch_de_ganbaru",
    "menu_pinch_de_ganbaru_p",
    "menu_tamatsuki_jump",
    "menu_tsuranuki_naguri",
    "msg_2bai_damage",
    "msg_cake",
    "msg_custom_super_boots",
    "msg_custom_super_hammer",
    "msg_custom_ultra_boots",
    "msg_custom_ultra_hammer",
    "msg_damage_flower",
    "msg_damage_flower_p",
    "msg_damage_gaeshi",
    "msg_ice_candy",
    "msg_jon_kanban_1",
    "msg_jon_kanban_2",
    "msg_jon_kanban_3",
    "msg_kame_no_noroi",
    "msg_kiken_de_power",
    "msg_kiken_de_power_p",
    "msg_nancy_frappe",
    "msg_pinch_de_ganbaru",
    "msg_pinch_de_ganbaru_p",
    "msg_pkr_monosiri",
    "msg_ptr_meromero_kiss",
    "msg_pwd_kumogakure",
    "msg_pys_nomikomi",
    "msg_shikaeshi_no_kona",
    "msg_super_coin",
    "msg_teki_kyouka",
    "msg_toughen_up",
    "msg_toughen_up_menu",
    "msg_toughen_up_p",
    "msg_toughen_up_p_menu",
    "pit_charlieton_full_inv",
    "pit_chest_unclaimed",
    "pit_disabled_return",
    "pit_reward_party_join",
    "tik_06_02",
};

const char* GetYoshiTextColor() {
    const char* kYoshiColorStrings[] = {
        "00c100", "e50000", "0000e5", "d07000",
        "e080d0", "404040", "90b0c0", "000000",
    };
    if (!g_Randomizer->state_.GetOptionValue(
        RandomizerState::YOSHI_COLOR_SELECT)) {
        return kYoshiColorStrings[7];
    } else {
        return kYoshiColorStrings[ttyd::mario_pouch::pouchGetPartyColor(4)];
    }
}

}
    
const char* RandomizerStrings::LookupReplacement(const char* msg_key) {
    // Do not use for more than one custom message at a time!
    static char buf[512];
    
    // Handle journal Tattle entries.
    if (strstr(msg_key, "menu_enemy_")) {
        msg_key = SetCustomMenuTattle(msg_key);
    }
    
    // Binary search on all possible message replacements.
    constexpr const int32_t kNumMsgKeys =
        sizeof(kKeyLookups) / sizeof(const char*);
    int32_t idx_min = 0;
    int32_t idx_max = kNumMsgKeys - 1;
    int32_t idx;
    int32_t strcmp_result;
    bool found = false;
    while (idx_min <= idx_max) {
        idx = (idx_min + idx_max) / 2;
        strcmp_result = strcmp(msg_key, kKeyLookups[idx]);
        if (strcmp_result < 0) {
            idx_max = idx - 1;
        } else if (strcmp_result > 0) {
            idx_min = idx + 1;
        } else {
            found = true;
            break;
        }
    }
    
    if (!found) return nullptr;
    
    // TODO: Order of case statements shouldn't matter, but consider either
    // ordering them alphabetically or putting logically similar ones together?
    switch (idx) {
        case MsgKey::CUSTOM_TATTLE_BATTLE:
        case MsgKey::CUSTOM_TATTLE_MENU:
            return GetCustomTattle();
        case MsgKey::PIT_CHARLIETON_FULL_INV:
            return "<p>\n"
                   "Ooh, you can't carry any more \n"
                   "stuff, my man. You sure you\n"
                   "want to buy this anyway?\n<o>";
        case MsgKey::PIT_REWARD_PARTY_JOIN:
            return "<system>\n<p>\nYou got a new party member!\n<k>";
        case MsgKey::PIT_DISABLED_RETURN:
            return "<system>\n<p>\nYou can't leave the Infinite Pit!\n<k>";
        case MsgKey::PIT_CHEST_UNCLAIMED:
            return "<system>\n<p>\nYou haven't claimed your\nreward!\n<k>";
        case MsgKey::MSG_JON_KANBAN_1: {
            sprintf(buf, "<kanban>\n<pos 150 25>\nFloor %" PRId32 "\n<k>", 
                    g_Randomizer->state_.floor_ + 1);
            return buf;
        }
        case MsgKey::MSG_JON_KANBAN_2: {
            if (g_Randomizer->state_.GetPlayStatsString(buf)) return buf;
            return "<kanban>\n"
                   "Start a new file to see some\n"
                   "of your play stats here!\n<k>";
        }
        case MsgKey::MSG_JON_KANBAN_3: {
            sprintf(buf, "<kanban>\nYour seed: <col %sff>%s\n</col>"
                "Currently selected options:\n<col 0000ffff>%s\n</col><k>",
                GetYoshiTextColor(),
                ttyd::mariost::g_MarioSt->saveFileName, 
                g_Randomizer->state_.GetEncodedOptions());
            return buf;
        }
        case MsgKey::TIK_06_02: {
            sprintf(buf, "<kanban>\n"
                "Thanks for playing the PM:TTYD\n"
                "Infinite Pit mod! Check the \n"
                "sign in back for your seed,\n<k><p>\n"
                "and currently selected options.\n"
                "If you want a random seed,\n"
                "name your file \"random\" or \"\xde\".\n<k>");
            return buf;
        }
        case MsgKey::IN_2BAI_DAMAGE:
            return "No Pain, No Gain";
        case MsgKey::IN_CAKE:
            return "Strawberry Cake";
        case MsgKey::IN_TOUGHEN_UP:
            return "Toughen Up";
        case MsgKey::IN_TOUGHEN_UP_P:
            return "Toughen Up P";
        case MsgKey::MSG_2BAI_DAMAGE:
        case MsgKey::MENU_2BAI_DAMAGE:
            return "Doubles the damage Mario \n"
                   "takes, but doubles coin drops.";
        case MsgKey::MSG_CAKE:
            return "Scrumptious strawberry cake.\n"
                   "Heals 5 to 30 HP and FP.";
        case MsgKey::MSG_SHIKAESHI_NO_KONA:
            return "Direct attackers take back\n"
                   "the same damage they deal.";
        case MsgKey::MSG_KAME_NO_NOROI:
            return "Has a chance of inducing Slow \n"
                   "status on all foes.";
        case MsgKey::MSG_TEKI_KYOUKA:
            return "Boosts foes' level by 5, but \n"
                   "temporarily gives them +3 ATK.";
        case MsgKey::MSG_ICE_CANDY:
            return "A dessert made by Zess T.\n"
                   "Gives 15 FP, but might freeze!";
        case MsgKey::LIST_ICE_CANDY:
            return "A dessert made by Zess T.\n"
                   "Gives 15 FP, but might freeze!\n"
                   "Made by mixing Honey Syrup \n"
                   "with an Ice Storm.";
        case MsgKey::MSG_NANCY_FRAPPE:
            return "A dessert made by Zess T.\n"
                   "Gives 20 FP, but might freeze!";
        case MsgKey::LIST_NANCY_FRAPPE:
            return "A dessert made by Zess T.\n"
                   "Gives 20 FP, but might freeze!\n"
                   "Made by mixing Maple Syrup \n"
                   "with an Ice Storm.";
        case MsgKey::MSG_TOUGHEN_UP:
            return "Wear this to add Toughen Up\n"
                   "to Mario's Tactics menu.";
        case MsgKey::MSG_TOUGHEN_UP_P:
            return "Wear this to add Toughen Up\n"
                   "to partners' Tactics menu.";
        case MsgKey::MSG_TOUGHEN_UP_MENU:
            return "Wear this to add Toughen Up\n"
                   "to Mario's Tactics menu.\n"
                   "This uses 1 FP to raise DEF\n"
                   "by 2 points for a turn.\n"
                   "Wearing more copies raises\n"
                   "the effect and FP cost.";
        case MsgKey::MSG_TOUGHEN_UP_P_MENU:
            return "Wear this to add Toughen Up\n"
                   "to partners' Tactics menu.\n"
                   "This uses 1 FP to raise DEF\n"
                   "by 2 points for a turn.\n"
                   "Wearing more copies raises\n"
                   "the effect and FP cost.";
        case MsgKey::BTL_HLP_CMD_OPERATION_SUPER_CHARGE:
            return "Briefly increases DEF by\n"
                   "more than Defending.";
        case MsgKey::MSG_PKR_MONOSIRI:
            return "A super-stylish move that\n"
                   "describes an enemy's stats.";
        case MsgKey::MSG_PTR_MEROMERO_KISS:
            return "Blow a kiss to an enemy to try\n"
                   "to win them to your side.";
        case MsgKey::MSG_PWD_KUMOGAKURE:
            return "Makes your team dodgy for \n"
                   "a time so foes frequently miss.";
        case MsgKey::MSG_PYS_NOMIKOMI:
            return "Spit the front enemy into all\n"
                   "ground-bound enemies behind it.";
        case MsgKey::MSG_SUPER_COIN:
            return "A mysterious, powerful object.\n"
                   "Use it to power up your partner!";
        case MsgKey::MSG_CUSTOM_SUPER_BOOTS:
            return "A stronger pair of boots.";
        case MsgKey::MSG_CUSTOM_ULTRA_BOOTS:
            return "An even stronger pair of boots.";
        case MsgKey::MSG_CUSTOM_SUPER_HAMMER:
            return "A more powerful hammer.";
        case MsgKey::MSG_CUSTOM_ULTRA_HAMMER:
            return "An even more powerful hammer.";
        case MsgKey::MENU_TAMATSUKI_JUMP:
            return "Wear this to use Tornado\n"
                   "Jump, a 2-FP move which can\n"
                   "damage and dizzy airborne\n"
                   "enemies if executed well.\n"
                   "Wearing more copies of the\n"
                   "badge increases its FP\n"
                   "cost and attack power.";
        case MsgKey::MENU_TSURANUKI_NAGURI:
            return "Wear this to use Piercing\n"
                   "Blow, a 2-FP move which\n"
                   "deals damage that pierces\n"
                   "enemy defenses.\n"
                   "Wearing more copies of the\n"
                   "badge increases its FP\n"
                   "cost and attack power.";
        case MsgKey::MENU_FIRE_NAGURI:
            return "Wear this to use Fire Drive,\n"
                   "a 3-FP move which deals \n"
                   "fire damage and Burn status\n"
                   "to all grounded enemies.\n"
                   "Wearing more copies of the\n"
                   "badge increases its FP\n"
                   "cost and attack power.";
        case MsgKey::MENU_ICE_NAGURI:
            return "Wear this to use Ice Smash,\n"
                   "a 2-FP move which deals\n"
                   "ice damage and may give its\n"
                   "target the Freeze status.\n"
                   "Wearing more copies of the\n"
                   "badge increases its FP\n"
                   "cost and status duration.";
        case MsgKey::MENU_CHARGE:
            return "Wear this to add Charge\n"
                   "to Mario's Tactics menu.\n"
                   "This uses 2 FP to increase\n"
                   "the next move's ATK by 2.\n"
                   "Wearing more copies raises\n"
                   "the effect and FP cost.";
        case MsgKey::MENU_CHARGE_P:
            return "Wear this to add Charge\n"
                   "to partners' Tactics menu.\n"
                   "This uses 2 FP to increase\n"
                   "the next move's ATK by 2.\n"
                   "Wearing more copies raises\n"
                   "the effect and FP cost.";
        case MsgKey::MSG_DAMAGE_GAESHI:
        case MsgKey::MENU_DAMAGE_GAESHI:
            return "Make direct-attackers take\n"
                   "the same damage they deal.";
        case MsgKey::MSG_DAMAGE_FLOWER:
        case MsgKey::MENU_DAMAGE_FLOWER:
            return "Recover 1 FP whenever\n"
                   "Mario receives damage.";
        case MsgKey::MSG_DAMAGE_FLOWER_P:
        case MsgKey::MENU_DAMAGE_FLOWER_P:
            return "Recover 1 FP whenever your\n"
                   "partner receives damage.";
        case MsgKey::MSG_KIKEN_DE_POWER:
        case MsgKey::MENU_KIKEN_DE_POWER:
            if (g_Randomizer->state_.GetOptionValue(
                RandomizerState::WEAKER_RUSH_BADGES)) {
                return "Increase Attack power by 2\n"
                       "when Mario is in Peril.";
            }
            return nullptr;
        case MsgKey::MSG_KIKEN_DE_POWER_P:
        case MsgKey::MENU_KIKEN_DE_POWER_P:
            if (g_Randomizer->state_.GetOptionValue(
                RandomizerState::WEAKER_RUSH_BADGES)) {
                return "Increase Attack power by 2\n"
                       "when your partner is in Peril.";
            }
            return nullptr;
        case MsgKey::MSG_PINCH_DE_GANBARU:
        case MsgKey::MENU_PINCH_DE_GANBARU:
            if (g_Randomizer->state_.GetOptionValue(
                RandomizerState::WEAKER_RUSH_BADGES)) {
                return "Increase Attack power by 1\n"
                       "when Mario is in Danger.";
            }
            return nullptr;
        case MsgKey::MSG_PINCH_DE_GANBARU_P:
        case MsgKey::MENU_PINCH_DE_GANBARU_P:
            if (g_Randomizer->state_.GetOptionValue(
                RandomizerState::WEAKER_RUSH_BADGES)) {
                return "Increase Attack power by 1\n"
                       "when your ally is in Danger.";
            }
            return nullptr;
    }
    // Should not be reached.
    return nullptr;
}

}