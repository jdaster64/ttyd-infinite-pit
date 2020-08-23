#include "randomizer_menu.h"

#include "common_functions.h"
#include "common_types.h"
#include "common_ui.h"
#include "randomizer.h"
#include "randomizer_state.h"

#include <ttyd/mario_pouch.h>
#include <ttyd/mariost.h>
#include <ttyd/system.h>

#include <cinttypes>
#include <cstdio>
#include <cstring>

namespace mod::pit_randomizer {

namespace {
    
using ::ttyd::mario_pouch::PouchData;
    
// Menu states.
namespace MenuState {
    enum e {
        INVALID_MENU_STATE = 0,
        CHANGE_PAGE,
        NUM_CHEST_REWARDS,
        HP_MODIFIER,
        ATK_MODIFIER,
        MERLEE,
        SUPERGUARDS_COST_FP,
        NO_EXP_MODE,
    };
}

// Pre-Pit room name.
const char kStartRoomName[] = "tik_06";

// Menu constants.
const int32_t kMenuX                = -260;
const int32_t kMenuY                = -85;
const int32_t kMenuWidth            = 320;
const int32_t kMenuPadding          = 15;
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
uint16_t last_command_ = 0;
int32_t time_button_held_ = kFadeoutEndTime;
int32_t menu_selection_ = 1;
int32_t menu_page_ = 1;
int32_t menu_state_ = MenuState::NUM_CHEST_REWARDS;

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

uint32_t GetActiveColor(int32_t state, uint8_t alpha) {
    return (menu_state_ == state ? -0xffffU : -0xffU) | alpha;
}

void DrawMenuString(
    const char* str, float x, float y, uint32_t color, int32_t alignment) {
    DrawText(str, x, y, 0xffu, true, color, 0.75f, alignment);
}

void DrawOnOffString(bool onoff, float x, float y, uint8_t alpha) {
    DrawText(onoff ? "On" : "Off", x, y, alpha, true,
             onoff ? 0xc100ffU : 0xe50000ffU, 0.75f, /* right-center */ 5);
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
        
    switch (100 * menu_page_ + menu_selection_) {
        case 101: {
            menu_state_ = MenuState::NUM_CHEST_REWARDS;
            break;
        }
        case 102: {
            menu_state_ = MenuState::HP_MODIFIER;
            break;
        }
        case 103: {
            menu_state_ = MenuState::ATK_MODIFIER;
            break;
        }
        case 201: {
            menu_state_ = MenuState::MERLEE;
            break;
        }
        case 202: {
            menu_state_ = MenuState::SUPERGUARDS_COST_FP;
            break;
        }
        case 203: {
            menu_state_ = MenuState::NO_EXP_MODE;
            break;
        }
        default: {
            menu_state_ = MenuState::CHANGE_PAGE;
            break;
        }
    }
    
    RandomizerState& state = g_Randomizer->state_;
    PouchData& pouch = *ttyd::mario_pouch::pouchGetPtr();
    
    if (time_button_held_ < 0) return;
    switch (last_command_) {
        case kMenuUpCommand: {
            if (time_button_held_ == 0) {
                if (menu_selection_ == 1) {
                    menu_selection_ = 4;
                } else {
                    --menu_selection_;
                }
            }
            break;
        }
        case kMenuDownCommand: {
            if (time_button_held_ == 0) {
                if (menu_selection_ == 4) {
                    menu_selection_ = 1;
                } else {
                    ++menu_selection_;
                }
            }
            break;
        }
        case kMenuSelectCommand: {
        lblSelectCommand:
            if (time_button_held_ == 0) {
                switch (menu_state_) {
                    case MenuState::CHANGE_PAGE: {
                        menu_page_ = menu_page_ ^ 3;
                        break;
                    }
                    case MenuState::MERLEE: {
                        state.options_ ^= RandomizerState::MERLEE;
                        if (state.options_ & RandomizerState::MERLEE) {
                            pouch.merlee_curse_uses_remaining = 99;
                            pouch.turns_until_merlee_activation = -1;
                        } else {
                            pouch.merlee_curse_uses_remaining = 0;
                            pouch.turns_until_merlee_activation = 0;
                        }
                        break;
                    }
                    case MenuState::SUPERGUARDS_COST_FP: {
                        state.options_ ^= RandomizerState::SUPERGUARDS_COST_FP;
                        break;
                    }
                    case MenuState::NO_EXP_MODE: {
                        state.options_ ^= RandomizerState::NO_EXP_MODE;
                        if (state.options_ & RandomizerState::NO_EXP_MODE) {
                            pouch.rank = 3;
                            pouch.level = 99;
                            pouch.unallocated_bp += 90;
                            pouch.total_bp += 90;
                        } else {
                            pouch.rank = 0;
                            pouch.level = 1;
                            pouch.unallocated_bp -= 90;
                            pouch.total_bp -= 90;
                        }
                        break;
                    }
                    default: break;
                }
            }
            break;
        }
        case kMenuLeftCommand: {
            if (ShouldTickOrAutotick(time_button_held_)) {
                switch (menu_state_) {
                    case MenuState::NUM_CHEST_REWARDS: {
                        if ((state.options_ & 
                            RandomizerState::NUM_CHEST_REWARDS) > 0) {
                            --state.options_;
                        } else {
                            state.options_ = 5;
                        }
                        break;
                    }
                    case MenuState::HP_MODIFIER: {
                        if (state.hp_multiplier_ > 1)
                            --state.hp_multiplier_;
                        break;
                    }
                    case MenuState::ATK_MODIFIER: {
                        if (state.atk_multiplier_ > 1)
                            --state.atk_multiplier_;
                        break;
                    }
                    default: break;
                }
            }
            // Can also be treated as a "select" input.
            goto lblSelectCommand;
        }
        case kMenuRightCommand: {
            if (ShouldTickOrAutotick(time_button_held_)) {
                switch (menu_state_) {
                    case MenuState::NUM_CHEST_REWARDS: {
                        if ((state.options_ & 
                            RandomizerState::NUM_CHEST_REWARDS) < 5) {
                            ++state.options_;
                        } else {
                            state.options_ = 0;
                        }
                        break;
                    }
                    case MenuState::HP_MODIFIER: {
                        if (state.hp_multiplier_ < 1000)
                            ++state.hp_multiplier_;
                        break;
                    }
                    case MenuState::ATK_MODIFIER: {
                        if (state.atk_multiplier_ < 1000)
                            ++state.atk_multiplier_;
                        break;
                    }
                    default: break;
                }
            }
            // Can also be treated as a "select" input.
            goto lblSelectCommand;
        }
        default: break;
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
    
    const uint32_t kMenuHeight = 19 * 4 - 4 + kMenuPadding * 2;
    DrawWindow(window_color, kMenuX, kMenuY, kMenuWidth, kMenuHeight, 10);
    
    const int32_t kTextX = kMenuX + kMenuPadding;
    const int32_t kValueX = kMenuX + kMenuWidth - kMenuPadding;
    int32_t kRowY = kMenuY - kMenuPadding - 8;
    
    char buf[128];
    uint32_t color;
    
    const RandomizerState& state = g_Randomizer->state_;
    
    if (menu_page_ == 1) {
        const uint32_t num_rewards =
            state.options_ & RandomizerState::NUM_CHEST_REWARDS;
        const char kChestRewardNumString[2] = { 
            static_cast<char>('0' + num_rewards), '\0'
        };
        color = GetActiveColor(MenuState::NUM_CHEST_REWARDS, alpha);
        sprintf(buf, "Rewards per chest:");
        DrawMenuString(buf, kTextX, kRowY, color, /* left-center */ 3);
        sprintf(buf, num_rewards > 0 ? kChestRewardNumString : "Random");
        DrawMenuString(buf, kValueX, kRowY, color, /* right-center */ 5);
        kRowY -= 19;
        
        color = GetActiveColor(MenuState::HP_MODIFIER, alpha);
        sprintf(buf, "Enemy HP multiplier:");
        DrawMenuString(buf, kTextX, kRowY, color, /* left-center */ 3);
        sprintf(buf, "%" PRId32 "%s", state.hp_multiplier_, "%");
        DrawMenuString(buf, kValueX, kRowY, color, /* right-center */ 5);
        kRowY -= 19;
        
        color = GetActiveColor(MenuState::ATK_MODIFIER, alpha);
        sprintf(buf, "Enemy ATK multiplier:");
        DrawMenuString(buf, kTextX, kRowY, color, /* left-center */ 3);
        sprintf(buf, "%" PRId32 "%s", state.atk_multiplier_, "%");
        DrawMenuString(buf, kValueX, kRowY, color, /* right-center */ 5);
        kRowY -= 19;
    } else if (menu_page_ == 2) {
        color = GetActiveColor(MenuState::MERLEE, alpha);
        sprintf(buf, "Infinite Merlee curses:");
        DrawMenuString(buf, kTextX, kRowY, color, /* left-center */ 3);
        DrawOnOffString(state.options_ & RandomizerState::MERLEE, 
                        kValueX, kRowY, alpha);
        kRowY -= 19;
        
        color = GetActiveColor(MenuState::SUPERGUARDS_COST_FP, alpha);
        sprintf(buf, "Superguards cost 1 FP:");
        DrawMenuString(buf, kTextX, kRowY, color, /* left-center */ 3);
        DrawOnOffString(state.options_ & RandomizerState::SUPERGUARDS_COST_FP, 
                        kValueX, kRowY, alpha);
        kRowY -= 19;
        
        color = GetActiveColor(MenuState::NO_EXP_MODE, alpha);
        sprintf(buf, "No EXP, Max BP mode:");
        DrawMenuString(buf, kTextX, kRowY, color, /* left-center */ 3);
        DrawOnOffString(state.options_ & RandomizerState::NO_EXP_MODE, 
                        kValueX, kRowY, alpha);
        kRowY -= 19;
    }
    
    color = GetActiveColor(MenuState::CHANGE_PAGE, alpha);
    sprintf(buf, "Next Page (%" PRId32 " of 2)", menu_page_);
    DrawMenuString(buf, kTextX, kRowY, color, /* left-center */ 3);
}

}