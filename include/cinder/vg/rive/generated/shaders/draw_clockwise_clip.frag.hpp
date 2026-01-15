#pragma once

#include "draw_clockwise_clip.frag.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char draw_clockwise_clip_frag[] = R"===(#ifdef GB
n2
#ifndef EB
C0(J3,x0);
#endif
U0(C4,K0);
#ifndef EB
C0(J6,B2);
#endif
U0(h6,v1);o2 C1(IB){H(M1,E);e L0=-M1.x;
#ifdef DB
T(A1,e);e a1=A1;
#else
T(J,N2);e a1=J.x;
#endif
g2;E D0;e l5,m5;
#if defined(DB)&&defined(JB)
if(JB){m5=a1;}else
#endif
{D0=unpackHalf2x16(T0(K0));l5=D0.y;e v4=l5==L0?D0.x:d1(.0);m5=v4+a1;}
#ifdef NC
e k5=M1.y;if(NC&&k5!=.0){e F3=.0;
#if defined(DB)&&defined(JB)
if(JB){D0=unpackHalf2x16(T0(K0));l5=D0.y;}
#endif
if(l5!=L0){F3=l5==k5?D0.x:.0;W0(v1,packHalf2x16(X2(F3,qe)));}else{F3=unpackHalf2x16(T0(v1)).x;L1(v1);}m5=min(m5,F3);}else
#endif
{L1(v1);}W0(K0,packHalf2x16(X2(m5,L0)));
#ifndef EB
f2(x0);
#endif
h2;p2;}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive