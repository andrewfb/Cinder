#pragma once

#include "draw_clockwise_clip.frag.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char draw_clockwise_clip_frag[] = R"===(#ifdef GB
v2
#ifndef FB
F0(T3,z0);
#endif
Z0(L4,M0);
#ifndef FB
F0(M6,H2);
#endif
Z0(h6,B1);w2 F1(JB){I(a2,G);d N0=-a2.x;
#ifdef EB
L(W0,d);d m0=W0;
#else
L(C,c3);d m0=C.x;
#endif
i2;G I0;d o5,p5;
#if defined(EB)&&defined(OB)
if(OB){p5=m0;}else
#endif
{I0=unpackHalf2x16(Y0(M0));o5=I0.y;d G4=o5==N0?I0.x:k1(.0);p5=G4+m0;}
#ifdef OC
d n5=a2.y;if(OC&&n5!=.0){d P3=.0;
#if defined(EB)&&defined(OB)
if(OB){I0=unpackHalf2x16(Y0(M0));o5=I0.y;}
#endif
if(o5!=N0){P3=o5==n5?I0.x:.0;d1(B1,packHalf2x16(f3(P3,qe)));}else{P3=unpackHalf2x16(Y0(B1)).x;Q1(B1);}p5=min(p5,P3);}else
#endif
{Q1(B1);}d1(M0,packHalf2x16(f3(p5,N0)));
#ifndef FB
h2(z0);
#endif
j2;x2;}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive