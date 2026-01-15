#pragma once

#include "draw_image_mesh.vert.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char draw_image_mesh_vert[] = R"===(#ifdef BB
w1(O2)l0(0,c,FC);x1 w1(V2)l0(1,c,GC);x1
#endif
V1 B0 X(0,c,I0);
#ifdef L
OB X(1,e,h3);
#endif
#if defined(Z)&&!defined(AB)
B0 X(2,g,J0);
#endif
N1
#ifdef BB
n3 o3 g6(DC,O2,P2,V2,W2,B){n0(B,P2,FC,c);n0(B,W2,GC,c);T(I0,c);
#ifdef L
T(h3,e);
#endif
#if defined(Z)&&!defined(AB)
T(J0,g);
#endif
c f0=P0(X1(p0.M8),FC)+p0.O1;I0=GC;
#ifdef L
if(L){h3=O7(p0.L0,A.J5);}
#endif
#ifdef Z
if(Z){
#ifndef AB
J0=p7(X1(p0.Y1),p0.m2,f0 a5);
#else
xb(X1(p0.Y1),p0.m2,f0 a5);
#endif
}
#endif
g P=H3(f0);
#ifdef IC
P.y=-P.y;
#endif
#ifdef AB
P.z=z9(p0.E6);
#endif
g0(I0);
#ifdef L
g0(h3);
#endif
#if defined(Z)&&!defined(AB)
g0(J0);
#endif
z1(P);}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive