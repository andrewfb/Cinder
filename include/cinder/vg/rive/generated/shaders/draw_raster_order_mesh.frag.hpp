#pragma once

#include "draw_raster_order_mesh.frag.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char draw_raster_order_mesh_frag[] = R"===(#ifdef GB
#if defined(FB)&&!defined(Q)
#undef Aa
#else
#define Aa
#endif
v2
#ifndef FB
F0(T3,z0);
#endif
Z0(L4,M0);
#ifndef FB
F0(M6,H2);
#endif
Z0(h6,B1);w2
#ifdef MB
W2 z2(l3,w6,UB);X2 i4 A3(l3,x6,z3)j4 Y3 Z3
#endif
#ifdef FB
#ifdef MB
c4(JB)
#else
M2(JB)
#endif
#else
#ifdef MB
y5(JB)
#else
F1(JB)
#endif
#endif
{
#ifdef AB
I(o1,g);I(i1,c);
#endif
#ifdef MB
I(r0,c);
#endif
#ifdef Q
I(r3,d);
#endif
#ifdef DB
I(S0,g);
#endif
#if defined(AB)&&defined(HB)
I(m2,d);
#endif
#ifdef AB
i j=J8(o1,1. U2);d m=I7(i1,q.d4 P0);
#endif
#ifdef MB
i j=c7(UB,z3,r0,q.vb);d m=1.;
#endif
#ifdef DB
if(DB){d R4=max(J7(F5(S0)),k1(.0));m=min(R4,m);}
#endif
#ifdef Aa
i2;
#endif
#ifdef Q
if(Q&&r3!=.0){G I0=unpackHalf2x16(Y0(M0));d e6=I0.y;d p5=max(e6==r3?I0.x:k1(.0),k1(.0));m=min(m,p5);}
#endif
#ifdef MB
m*=l0.V2;
#endif
#ifndef FB
i E1=X0(z0);
#ifdef HB
if(HB){
#ifdef AB
V e2=C6(m2);
#endif
#ifdef MB
j.xyz=H4(j);V e2=R1(l0.e2);
#endif
if(e2!=r5){j.xyz=v5(j.xyz,E1,e2);}j.w*=m;j.xyz*=j.w;}else
#endif
{j*=m;}
#ifdef CC
if(CC){j=h3(j);}
#endif
c1(z0,E1*(1.-j.w)+j);
#endif
Q1(M0);Q1(B1);
#ifdef Aa
j2;
#endif
#ifdef FB
r1=j*m;
#endif
x2;}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive