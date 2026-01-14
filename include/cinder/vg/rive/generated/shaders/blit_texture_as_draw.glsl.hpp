#pragma once

#include "blit_texture_as_draw.glsl.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char blit_texture_as_draw[] = R"===(D1
#ifdef WC
q0 N(0,c,r0);
#endif
z1
#ifdef BB
j3 k3 g4 h4 g1(c0)h1 p1(QE,c0,D,p,O){c O1;O1.x=(p&1)==0?-1.:1.;O1.y=(p&2)==0?-1.:1.;
#ifdef WC
L(r0,c);r0.x=O1.x*.5+.5;r0.y=O1.y*-.5+.5;W(r0);
#endif
g J=g(O1,0,1);l1(J);}
#endif
#ifdef GB
W2 z2(l3,w6,DD);X2
#ifdef WC
i4 A3(l3,x6,Sd)j4
#endif
U1(i,UD){i q9;
#ifdef WC
I(r0,c);q9=d2(DD,Sd,r0,.0);
#else
q9=G1(DD,Y(floor(v0.xy)));
#endif
V1(q9);}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive