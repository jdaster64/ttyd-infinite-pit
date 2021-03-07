#include "randomizer_menu.h"

#include "common_functions.h"
#include "common_types.h"
#include "common_ui.h"
#include "randomizer.h"
#include "randomizer_state.h"

#include <ttyd/mariost.h>
#include <ttyd/system.h>

#include <cinttypes>
#include <cstdio>
#include <cstring>

namespace mod::pit_randomizer {

namespace {

// Pre-Pit room name.
const char kStartRoomName[] = "tik_06";

// Menu constants.
const int32_t kMenuX                = -260;
const int32_t kMenuY                = -85;
const int32_t kMenuWidth            = 378;
const int32_t kMenuPadding          = 15;
const int32_t kNumOptionPages       = 7;
const int32_t kOptionsPerPage       = 5;
const int32_t kFadeoutStartTime     = 600;
const int32_t kFadeoutEndTime       = 600 + 15;
const int32_t kFadeinTextStartTime  = 600 + 105;
const int32_t kFadeinTextEndTime    = 600 + 120;
const uint16_t kMenuUpCommand       = ButtonId::Z | ButtonId::DPAD_UP;
const uint16_t kMenuDownCommand     = ButtonId::Z | ButtonId::DPAD_DOWN;
const uint16_t kMenuLeftCommand     = ButtonId::Z | ButtonId::DPAD_LEFT;
const uint16_t kMenuRightCommand    = ButtonId::Z | ButtonId::DPAD_RIGHT;
const uint16_t kMenuSelectCommand   = ButtonId::Z | ButtonId::L;

// For automatically ticking options +1/-1 at a time if a direction is held.
const int32_t kMenuCommandSlowTickStart     = 24;
const int32_t kMenuCommandSlowTickRate      = 12;
const int32_t kMenuCommandFastTickStart     = 96;
const int32_t kMenuCommandFastTickRate      = 2;

// Global menu state variables.
int32_t pages_unlocked_ = 0b11110;
uint16_t last_command_ = 0;
int32_t time_button_held_ = kFadeoutEndTime;
int32_t menu_selection_ = 1;
int32_t menu_page_ = 1;
int32_t menu_state_ = 0;

bool ShouldDisplayMenu() {
    return InMainGameModes() && !strcmp(GetCurrentMap(), kStartRoomName) &&
           !((ttyd::mariost::marioStGetSystemLevel() & 0xf) == 0xf);  // !paused
}

bool ShouldControlMenu() {
    return ShouldDisplayMenu() && !strcmp(GetNextMap(), kStartRoomName);
}

bool ShouldTickOrAutotick(int32_t time_held) {
    if (time_held == 0) return true;
    if (time_held >= kMenuCommandFastTickStart &&
        time_held % kMenuCommandFastTickRate == 0) return true;
    if (time_held >= kMenuCommandSlowTickStart &&
        time_held % kMenuCommandSlowTickRate == 0) return true;
    return false;
}

int32_t GetMenuState(int32_t page, int32_t selection) {
    switch (100 * page + selection) {
        case 101: return RandomizerState::NUM_CHEST_REWARDS;
        case 102: return RandomizerState::BATTLE_REWARD_MODE;
        case 103: return RandomizerState::START_WITH_PARTNERS;
        case 104: return RandomizerState::START_WITH_SWEET_TREAT;
        
        case 201: return RandomizerState::NO_EXP_MODE;
        case 202: return RandomizerState::START_WITH_NO_ITEMS;
        case 203: return RandomizerState::SHINE_SPRITES_MARIO;
        case 204: return RandomizerState::ALWAYS_ENABLE_AUDIENCE;
        
        case 301: return RandomizerState::MERLEE;
        case 302: return RandomizerState::SUPERGUARDS_COST_FP;
        case 303: return RandomizerState::SWITCH_PARTY_COST_FP;
        case 304: return RandomizerState::INVALID_OPTION;
        
        case 401: return RandomizerState::HP_MODIFIER;
        case 402: return RandomizerState::ATK_MODIFIER;
        case 403: return RandomizerState::POST_100_HP_SCALING;
        case 404: return RandomizerState::POST_100_ATK_SCALING;
        
        case 501: return RandomizerState::WEAKER_RUSH_BADGES;
        case 502: return RandomizerState::CAP_BADGE_EVASION;
        case 503: return RandomizerState::HP_FP_DRAIN_PER_HIT;
        case 504: return RandomizerState::SWAP_CO_PL_SP_COST;
        
        case 601: return RandomizerState::STAGE_HAZARD_OPTIONS;
        case 602: return RandomizerState::DAMAGE_RANGE;
        case 603: return RandomizerState::AUDIENCE_ITEMS_RANDOM;
        case 604: return RandomizerState::INVALID_OPTION;
        
        case 701: return RandomizerState::PARTNER_STARTING_RANK;
        case 702: return RandomizerState::DANGER_PERIL_BY_PERCENT;
        case 703: return RandomizerState::MAX_BADGE_MOVE_LEVEL;
        case 704: return RandomizerState::RANK_UP_REQUIREMENT;
        
        default:  return RandomizerState::CHANGE_PAGE;
    }
}

uint32_t GetActiveColor(int32_t selection, uint8_t alpha) {
    return (menu_selection_ == selection ? -0xffffU : -0xffU) | alpha;
}

void DrawMenuString(
    const char* str, float x, float y, uint32_t color, int32_t alignment) {
    DrawText(str, x, y, 0xffu, true, color, 0.75f, alignment);
}

void ChangePage(int32_t direction) {
    for (int32_t i = 0; i < kNumOptionPages; ++i) {
        menu_page_ += direction;
        if (menu_page_ < 1) menu_page_ += kNumOptionPages;
        if (menu_page_ > kNumOptionPages) menu_page_ -= kNumOptionPages;
        if (pages_unlocked_ & (1 << menu_page_)) break;
    }
}

void ChangeSelection(int32_t direction) {
    for (int32_t i = 1; i <= kOptionsPerPage; ++i) {
        menu_selection_ += direction;
        if (menu_selection_ < 1) menu_selection_ += kOptionsPerPage;
        if (menu_selection_ > kOptionsPerPage) menu_selection_ -= kOptionsPerPage;
        if (GetMenuState(menu_page_, menu_selection_)
            != RandomizerState::INVALID_OPTION) break;
    }
}

}

RandomizerMenu::RandomizerMenu() {}

void RandomizerMenu::Init() {}

void RandomizerMenu::Update() {
    // Not in / leaving Pre-Pit room; prevent input and fade menu / text out.
    if (!ShouldControlMenu()) {
        last_command_ = 0;
        if (time_button_held_ < kFadeoutStartTime) {
            time_button_held_ = kFadeoutStartTime;
        } else if (time_button_held_ < kFadeoutEndTime) {
            ++time_button_held_;
        } else if (time_button_held_ > kFadeoutEndTime) {
            --time_button_held_;
        }
        return;
    }
    
    uint16_t buttons = ttyd::system::keyGetButton(0);
    
    if (last_command_ && (buttons & last_command_) == last_command_) {
        // If last command is non-empty and still held, increment timer.
        ++time_button_held_;
    } else {
        const uint16_t prev_command = last_command_;
        uint16_t new_command = 0;
        if ((buttons & kMenuSelectCommand) == kMenuSelectCommand) {
            new_command = kMenuSelectCommand;
        } else if ((buttons & kMenuUpCommand) == kMenuUpCommand) {
            new_command = kMenuUpCommand;
        } else if ((buttons & kMenuDownCommand) == kMenuDownCommand) {
            new_command = kMenuDownCommand;
        } else if ((buttons & kMenuLeftCommand) == kMenuLeftCommand) {
            new_command = kMenuLeftCommand;
        } else if ((buttons & kMenuRightCommand) == kMenuRightCommand) {
            new_command = kMenuRightCommand;
        }
        last_command_ = new_command;
        
        if (new_command && !prev_command && time_button_held_ >= kFadeoutEndTime) {
            // Command input when menu was previously not displayed;
            // no action should be taken until the next command is input.
            time_button_held_ = -999999999;
        } else if (new_command != prev_command) {
            // New command input; reset timer.
            time_button_held_ = 0;
        } else {
            // Still no command input; increment timer anyway.
            if (++time_button_held_ > kFadeinTextEndTime)
                time_button_held_ = kFadeinTextEndTime;
        }
    }
        
    menu_state_ = GetMenuState(menu_page_, menu_selection_);
    RandomizerState& state = g_Randomizer->state_;
    
    if (time_button_held_ < 0) return;
    switch (last_command_) {
        case kMenuUpCommand: {
            if (time_button_held_ == 0) ChangeSelection(-1);
            break;
        }
        case kMenuDownCommand: {
            if (time_button_held_ == 0) ChangeSelection(1);
            break;
        }
        case kMenuLeftCommand:
        case kMenuRightCommand:
        case kMenuSelectCommand: {
            int32_t direction = 0;
            if (last_command_ == kMenuRightCommand) direction = 1;
            if (last_command_ == kMenuLeftCommand) direction = -1;
            
            switch (menu_state_) {
                case RandomizerState::HP_MODIFIER:
                case RandomizerState::ATK_MODIFIER: {
                    if (ShouldTickOrAutotick(time_button_held_)) {
                        state.ChangeOption(menu_state_, direction);
                    }
                    break;
                }
                case RandomizerState::CHANGE_PAGE: {
                    if (time_button_held_ == 0) {
                        ChangePage(direction ? direction : 1);
                    }
                    break;
                }
                default: {
                    if (time_button_held_ == 0) {
                        state.ChangeOption(menu_state_, direction);
                    }
                    break;
                }
            }
            break;
        }
    }
}

void RandomizerMenu::Draw() {
    if (!ShouldDisplayMenu()) return;
    
    uint32_t alpha = 0xff;
    
    // Show text displaying how to open the options menu.
    if (!last_command_ && time_button_held_ >= kFadeinTextStartTime) {
        alpha = 0xff * (time_button_held_ - kFadeinTextStartTime)
                     / (kFadeinTextEndTime - kFadeinTextStartTime);
        if (alpha > 0xff) alpha = 0xff;
        DrawText(
            "Press L+Z to open the options menu, then\n"
            "hold Z and press L or the D-Pad to make selections.",
            0, -150, alpha, true, ~0U, 0.75f, /* alignment = center */ 4);
        return;
    }
    
    // If menu hasn't been touched, but not time for text to pop up, just return.
    if (!last_command_ && time_button_held_ >= kFadeoutEndTime) {
        return;
    }
    
    // Otherwise, draw the menu.
    if (!last_command_ && time_button_held_ >= kFadeoutStartTime) {
        alpha = 0xff * (kFadeoutEndTime - time_button_held_)
                     / (kFadeoutEndTime - kFadeoutStartTime);
    }

    const uint32_t window_color = static_cast<uint8_t>(alpha * 4 / 5);
    
    const uint32_t menu_height = 19 * kOptionsPerPage - 4 + kMenuPadding * 2;
    const uint32_t menu_width = menu_page_ > 4 ? kMenuWidth + 20 : kMenuWidth;
    
    DrawWindow(window_color, kMenuX, kMenuY, menu_width, menu_height, 10);
    
    const int32_t kTextX = kMenuX + kMenuPadding;
    const int32_t kValueX = kMenuX + menu_width - kMenuPadding;
    int32_t kRowY = kMenuY - kMenuPadding - 8;
    
    char name_buf[128];
    char value_buf[32];
    uint32_t color, special_color;
    
    const RandomizerState& state = g_Randomizer->state_;
    
    for (int32_t selection = 1; selection < kOptionsPerPage; ++selection) {
        // Get text strings & color for options on the current page.
        color = GetActiveColor(selection, alpha);
        int32_t menu_state = GetMenuState(menu_page_, selection);
        state.GetOptionStrings(menu_state, name_buf, value_buf, &special_color);
        // Draw the row's description and current value.
        DrawMenuString(name_buf, kTextX, kRowY, color, /* left-center */ 3);
        if (special_color) color = special_color;
        DrawMenuString(value_buf, kValueX, kRowY, color, /* right-center */ 5);
        // Advance to the next row's Y position.
        kRowY -= 19;
    }
    
    // Print the current page information in the bottom row.
    color = GetActiveColor(kOptionsPerPage, alpha);
    if (menu_page_ <= 4) {
        sprintf(
            name_buf, "Change Page (%" PRId32 "/%" PRId32 ")", menu_page_, 4);
    } else {
        sprintf(
            name_buf, "Change Page (Bonus %" PRId32 ")", menu_page_ - 4);
    }
    DrawMenuString(name_buf, kTextX, kRowY, color, /* left-center */ 3);
    
    // Print a warning over selections that change seeding.
    if ((menu_page_ == 1 && menu_selection_ != kOptionsPerPage) ||
        (menu_page_ == 7 && menu_selection_ == 1)) {
        sprintf(name_buf, "*Affects seeding");
        DrawText(
            name_buf, kValueX, kRowY + 1, 0xffu, true, 
            /* color = red */ 0xff0000ffU, 0.575f, /* right-top */ 2);
    }
}

void RandomizerMenu::SetMenuPageVisibility(int32_t page, bool enabled) {
    if (page < 1 || page > kNumOptionPages) return;
    if (enabled) {
        pages_unlocked_ |= (1 << page);
    } else {
        pages_unlocked_ &= ~(1 << page);
    }
}

}