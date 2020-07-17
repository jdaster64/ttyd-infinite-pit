#pragma once

#include "evtmgr.h"

#include <cstdint>
    
namespace ttyd::npcdrv {
struct NpcTribeDescription;
}

namespace ttyd::npc_data {

extern "C" {

extern ttyd::npcdrv::NpcTribeDescription npcTribe[1];

// npc_ai_type_table
// npc_define_territory_type

}

}