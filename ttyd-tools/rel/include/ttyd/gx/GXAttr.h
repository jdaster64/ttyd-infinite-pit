#pragma once

#include <cstdint>

namespace ttyd::gx::GXAttr {

extern "C" {
    
// GXSetVtxDesc
// GXSetVtxDescv
// __GXSetVCD
// __GXCalculateVLim
// GXGetVtxDesc
// GXGetVtxDescv
// GXClearVtxDesc
// GXSetVtxAttrFmt
// GXSetVtxAttrFmtv
// __GXSetVAT
// GXGetVtxAttrFmt
// GXGetVtxAttrFmtv
// GXSetArray
// GXInvalidateVtxCache
void GXSetTexCoordGen2(
    int32_t unk0, int32_t unk1, int32_t unk2,
    int32_t unk3, int32_t unk4, int32_t unk5);
void GXSetNumTexGens(int32_t unk0);

}

}