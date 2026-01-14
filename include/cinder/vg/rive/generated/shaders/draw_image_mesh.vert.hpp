#pragma once

#include "draw_image_mesh.vert.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char draw_image_mesh_vert[] = R"===(#ifdef BB
g1(q2)i0(0,c,XB);h1 g1(J2)i0(1,c,YB);h1
#endif
D1 q0 N(0,c,r0);
#ifdef Q
RB N(1,d,r3);
#endif
#if defined(DB)&&!defined(CB)
q0 N(2,g,S0);
#endif
z1
#ifdef BB
j3 k3 w5(TB,q2,r2,J2,K2,p){j0(p,r2,XB,c);j0(p,K2,YB,c);L(r0,c);
#ifdef Q
L(r3,d);
#endif
#if defined(DB)&&!defined(CB)
L(S0,g);
#endif
c R=H0(N1(l0.v7),XB)+l0.j1;r0=YB;
#ifdef Q
if(Q){r3=S7(l0.N0,q.L5);}
#endif
#ifdef DB
if(DB){
#ifndef CB
S0=w7(N1(l0.c2),l0.p2,R f5);
#else
yb(N1(l0.c2),l0.p2,R f5);
#endif
}
#endif
g J=R2(R);
#ifdef JC
J.y=-J.y;
#endif
#ifdef CB
J.z=H9(l0.H6);
#endif
W(r0);
#ifdef Q
W(r3);
#endif
#if defined(DB)&&!defined(CB)
W(S0);
#endif
l1(J);}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive