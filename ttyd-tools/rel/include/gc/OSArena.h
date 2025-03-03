#pragma once

#include <cstdint>

namespace ttyd::OSArena
{
    extern "C"
    {
        extern void *__OSArenaLo;
        extern void *__OSArenaHi;
    }
} // namespace ttyd::OSArena