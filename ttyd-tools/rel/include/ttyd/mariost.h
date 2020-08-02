#pragma once

#include <gc/types.h>

#include <cstdint>

namespace gc::OSLink {
struct OSModuleInfo;
}

namespace ttyd::mariost {
    
struct MarioSt_Globals {
    int8_t      unk_0000[0x11c];
    char        currentBeroName[16];
    char        currentMapName[16];
    char        currentAreaName[16];
    char        unk_14c[16];
    gc::OSLink::OSModuleInfo* pRelFileBase;     // Used for tst, jon modules
    gc::OSLink::OSModuleInfo* pMapAlloc;        // Used for others
    uint32_t    mapInitEvtId;
    void*       fbatData;
    uint32_t    language;
    uint16_t    fbWidth;
    uint16_t    fbHeight;
    int32_t     gsw0;                           // Story sequence position
    uint8_t     gswf[0x400];
    uint8_t     gsw[0x800];
    uint8_t     lswf[0x40];
    uint8_t     lsw[0x400];
    uint32_t    unk_11b8;
    const char  saveFileName[20];
    int32_t     saveFileNumber;
    gc::vec3    saveLastPlayerPosition;
    int32_t     saveParty0Id;
    int32_t     saveParty1Id;
    uint64_t    lastSaveTime;
    int32_t     saveCounter;
    int8_t      unk_11f4[0x1e4];
} __attribute__((__packed__));

static_assert(sizeof(MarioSt_Globals) == 0x13d8);
	
extern "C" {

void marioStInit();
void marioStMain();
void marioStDisp();
void marioStSystemLevel(uint32_t level);
uint32_t marioStGetSystemLevel();
void viPostCallback(uint32_t retraceCount);
void gcDvdCheckThread();
void gcRumbleCheck();

extern MarioSt_Globals* g_MarioSt;

}

}