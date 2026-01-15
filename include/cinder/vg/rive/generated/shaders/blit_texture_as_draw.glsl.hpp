#pragma once

#include "blit_texture_as_draw.glsl.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char blit_texture_as_draw[] = R"===(V1
#ifdef UC
B0 X(0,c,I0);
#endif
N1
#ifdef BB
n3 o3 U3 V3 w1(c0)x1 y1(RE,c0,F,B,O){c J1;J1.x=(B&1)==0?-1.:1.;J1.y=(B&2)==0?-1.:1.;
#ifdef UC
T(I0,c);I0.x=J1.x*.5+.5;I0.y=J1.y*-.5+.5;g0(I0);
#endif
g P=g(J1,0,1);z1(P);}
#endif
#ifdef GB
c3 K2(W3,I7,DD);d3
#ifdef UC
N4 X3(W3,J7,Sd)O4
#endif
d2(i,UD){i i9;
#ifdef UC
H(I0,c);i9=F2(DD,Sd,I0,.0);
#else
i9=D1(DD,S(floor(q0.xy)));
#endif
e2(i9);}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive