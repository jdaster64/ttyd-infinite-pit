#pragma once

#include "types.h"

#include <cstdint>

namespace gc {

void PSMTXIdentity(mtx34* out);
void PSMTXCopy(mtx34* src, mtx34* dst);
// PSMTXConcat
// PSMTXInverse
// PSMTXInvXpose
// PSMTXRotRad
// PSMTXRotTrig
// __PSMTXRotAxisRadInternal
// PSMTXRotAxisRad
void PSMTXTrans(mtx34* out, double x, double y, double z);
// PSMTXTransApply
// PSMTXScale
// PSMTXScaleApply
// C_MTXLookAt
// C_MTXLightFrustum
// C_MTXLightPerspective
// C_MTXLightOrtho
// PSMTXMultVec
// PSMTXMultVecSR
// C_MTXFrustum
// C_MTXPerspective
// C_MTXOrtho
// PSMTX44Copy
// PSMTX44Concat
// PSMTX44Trans
// PSMTX44Scale
// PSMTX44MultVec
// PSVECAdd
// PSVECSubtract
// PSVECScale
// PSVECNormalize
// PSVECSquareMag
// PSVECMag
// PSVECDotProduct
// PSVECCrossProduct
// C_VECHalfAngle
// C_VECReflect
// PSVECSquareDistance
// PSVECDistance

}