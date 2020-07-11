#pragma once

#include <cstdint>

namespace ttyd::seq_mapchange {

extern "C" {

// _relUnLoad
// _load
void _unload(const char *currentMap, const char *nextMap, const char *nextBero);
// seq_mapChangeMain
// seq_mapChangeExit
// seq_mapChangeInit

extern char NextBero[];
extern char NextMap[];
extern char NextArea[];

}

}