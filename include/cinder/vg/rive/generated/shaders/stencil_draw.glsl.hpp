#pragma once

#include "stencil_draw.glsl.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char stencil_draw[] = R"===(#ifdef BB
w1(c0)l0(0,y4,MB);x1 n3 o3 U3 V3 y1(JF,c0,F,B,O){n0(B,F,MB,y4);g P=H3(MB.xy);uint E6=floatBitsToUint(MB.z)&0xffffu;P.z=z9(E6);z1(P);}
#endif
#ifdef GB
c3 d3 d2(i,UD){e2(c1(.0));}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive