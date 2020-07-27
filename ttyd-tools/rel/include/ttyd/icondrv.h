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
// iconGetTexObj
// iconDispGxCol
// iconDispGx2
// iconDispGx
// iconDispGxAlpha
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