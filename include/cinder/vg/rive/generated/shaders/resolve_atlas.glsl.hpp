#pragma once

#include "resolve_atlas.glsl.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char resolve_atlas[] = R"===(#ifdef BB
y1(HF,c0,F,B,O){g P=g(mix(c(-1,1),c(1,-1),equal(B&S(1,2),S(0))),.0,1.);z1(P);}
#endif
#ifdef GB
#ifdef MD
__pixel_local_outEXT g1{layout(r32f)highp float M2;};
#else
__pixel_local_inEXT g1{layout(r32f)highp float M2;};layout(location=0)out highp uvec4 W5;
#endif
void main(){
#ifdef MD
M2=.0;
#else
W5.x=floatBitsToUint(M2);
#endif
}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive