#pragma once

#include <gc/types.h>

#include <cstdint>

namespace ttyd::icondrv {

extern "C" {

// iconNumberDispGx3D
uint32_t iconNumberDispGx(
    gc::mtx34* matrix, int32_t number, int32_t is_small, int32_t* unk_param);
// iconSetAlpha
// iconSetScale
// iconFlagOff
// iconFlagOn
// iconSetPos
// iconNameToPtr
// iconGX
// iconGetWidthHight
void iconGetTexObj(void* tex_obj, uint16_t icon_id);
// iconDispGxCol
// iconDispGx2
void iconDispGx(double unk0, gc::vec3* pos, int16_t unk2, uint16_t icon);
void iconDispGxAlpha
    (double unk0, gc::vec3* pos, int16_t unk2, uint16_t icon, uint8_t alpha);
// iconDisp
// iconChange
// iconDelete
// iconEntry2D
// iconEntry
// iconMain
// iconReInit
// iconTexSetup
// iconInit
// _callback_bin
// _callback_tpl

}

}