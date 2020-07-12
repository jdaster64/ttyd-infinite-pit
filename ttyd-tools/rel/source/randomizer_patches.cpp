#include "randomizer_patches.h"

#include "common_functions.h"
#include "common_types.h"
#include "evt_cmd.h"
#include "patch.h"

#include <gc/OSLink.h>
#include <ttyd/evt_eff.h>
#include <ttyd/evt_item.h>
#include <ttyd/evt_mario.h>
#include <ttyd/evt_mobj.h>
#include <ttyd/evt_msg.h>
#include <ttyd/evt_party.h>
#include <ttyd/evt_pouch.h>
#include <ttyd/evt_snd.h>

#include <cstdint>
#include <cstring>

namespace mod::pit_randomizer {

namespace {

using ::gc::OSLink::OSModuleInfo;

// Global variables.
uintptr_t g_PitModulePtr;
uintptr_t g_AdditionalModulePtr;

// Event that plays "get partner" fanfare.
EVT_BEGIN(PartnerFanfareEvt)
USER_FUNC(ttyd::evt_snd::evt_snd_bgmoff, 0x400)
USER_FUNC(ttyd::evt_snd::evt_snd_bgmon, 1, PTR("BGM_FF_GET_PARTY1"))
WAIT_MSEC() 2000,
RETURN()
EVT_END()

// Event that handles a chest being opened.
EVT_BEGIN(ChestOpenEvt)
USER_FUNC(ttyd::evt_mario::evt_mario_key_onoff, 0)
USER_FUNC(ttyd::evt_mobj::evt_mobj_wait_animation_end, PTR("box"))
// TODO: Call a user func to select the "item" to spawn.
// In the event the "item" is actually a partner...
USER_FUNC(ttyd::evt_mario::evt_mario_normalize)
USER_FUNC(ttyd::evt_mario::evt_mario_goodbye_party, 0)
WAIT_MSEC() 500,
// TODO: Parameterize which partner to spawn.
USER_FUNC(ttyd::evt_pouch::evt_pouch_party_join, 1)
// TODO: Reposition partner by box? At origin is fine...
USER_FUNC(ttyd::evt_mario::evt_mario_set_party_pos, 0, 1, 0, 0, 0)
RUN_EVT_ID(PartnerFanfareEvt, LW(11))
USER_FUNC(ttyd::evt_eff::evt_eff,
          PTR("sub_bg"), PTR("itemget"), 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
USER_FUNC(ttyd::evt_msg::evt_msg_toge, 1, 0, 0, 0)
// Prints a custom message; the "joined party" messages aren't loaded anyway.
USER_FUNC(ttyd::evt_msg::evt_msg_print, 0, PTR("pit_reward_party_join"), 0, 0)
EVT_HELPER_CMD(2, 106), LW(11), LW(12),  // checks if thread LW(11) running
IF_EQUAL(LW(12), 1)
    DELETE_EVT(LW(11))
    USER_FUNC(ttyd::evt_snd::evt_snd_bgmoff, 0x201)
END_IF()
USER_FUNC(ttyd::evt_eff::evt_eff_softdelete, PTR("sub_bg"))
USER_FUNC(ttyd::evt_snd::evt_snd_bgmon, 0x120, 0)
USER_FUNC(ttyd::evt_snd::evt_snd_bgmon_f, 0x300, PTR("BGM_STG0_100DN1"), 1500)
WAIT_MSEC() 500,
USER_FUNC(ttyd::evt_party::evt_party_run, 0)
USER_FUNC(ttyd::evt_party::evt_party_run, 1)
USER_FUNC(ttyd::evt_mario::evt_mario_key_onoff, 1)
RETURN()
EVT_END()

// Short wrapper that calls the above event.
EVT_BEGIN(ChestOpenEvtHook) 
RUN_CHILD_EVT(ChestOpenEvt)
RETURN()
EVT_END()

}

void OnModuleLoaded(OSModuleInfo* module) {
    if (module == nullptr) return;
    int32_t module_id = module->id;
    uintptr_t module_ptr = reinterpret_cast<uintptr_t>(module);
    
    if (module_id == ModuleId::JON) {
        // Apply custom logic to box opening event to allow spawning partners.
        mod::patch::writePatch(
            reinterpret_cast<void*>(module_ptr + kPitEvtOpenBoxOffset),
            ChestOpenEvtHook, sizeof(ChestOpenEvtHook));
        
        g_PitModulePtr = module_ptr;
    } else {
        g_AdditionalModulePtr = module_ptr;
    }
}

const char* GetReplacementMessage(const char* msg_key) {
    if (!strcmp(msg_key, "pit_reward_party_join")) {
        return "<system>\n<p>\nGoombella joined your party!\n<k>";
    }
    return nullptr;
}

}