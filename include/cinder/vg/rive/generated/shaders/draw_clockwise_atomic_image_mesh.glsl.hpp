#pragma once

#include "draw_clockwise_atomic_image_mesh.glsl.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char draw_clockwise_atomic_image_mesh[] = R"===(#ifdef BB
g1(q2)i0(0,c,XB);h1 g1(J2)i0(1,c,YB);h1
#endif
D1 q0 N(0,c,r0);z1
#ifdef BB
j3 k3 w5(TB,q2,r2,J2,K2,p){j0(p,r2,XB,c);j0(p,K2,YB,c);L(r0,c);c R=H0(N1(l0.v7),XB)+l0.j1;r0=YB;g J=R2(R);W(r0);l1(J);}
#endif
#ifdef GB
W2 z2(l3,w6,UB);X2 i4 A3(l3,x6,z3)j4 Y3 Z3 U1(i,JB){I(r0,c);i c8=l6(UB,z3,r0);c8=T0(H4(c8),c8.w*l0.V2);V1(c8);}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive