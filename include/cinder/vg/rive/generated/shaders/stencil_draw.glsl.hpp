#pragma once

#include "stencil_draw.glsl.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char stencil_draw[] = R"===(#ifdef BB
g1(c0)i0(0,R3,KB);h1 j3 k3 g4 h4 p1(IF,c0,D,p,O){j0(p,D,KB,R3);g J=R2(KB.xy);uint H6=floatBitsToUint(KB.z)&0xffffu;J.z=H9(H6);l1(J);}
#endif
#ifdef GB
W2 X2 U1(i,UD){V1(T0(.0));}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive