#pragma once

#include "resolve_atlas.glsl.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char resolve_atlas[] = R"===(#ifdef BB
p1(GF,c0,D,p,O){g J=g(mix(c(-1,1),c(1,-1),equal(p&Y(1,2),Y(0))),.0,1.);l1(J);}
#endif
#ifdef GB
#ifdef MD
__pixel_local_outEXT m1{layout(r32f)highp float Q2;};
#else
__pixel_local_inEXT m1{layout(r32f)highp float Q2;};layout(location=0)out highp uvec4 Z5;
#endif
void main(){
#ifdef MD
Q2=.0;
#else
Z5.x=floatBitsToUint(Q2);
#endif
}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive