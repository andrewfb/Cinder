#pragma once

#include "draw_mesh.frag.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char draw_mesh_frag[] = R"===(#ifdef GB
#if(defined(EB)&&!defined(L))||defined(VB)
#undef ta
#else
#define ta
#endif
#ifndef VB
n2
#ifndef EB
C0(J3,x0);
#endif
U0(C4,K0);
#ifndef EB
C0(J6,B2);
#endif
U0(h6,v1);o2
#endif
#ifdef KB
c3 K2(W3,I7,EC);d3 N4 X3(W3,J7,B5)O4 D4 G4
#endif
#ifdef EB
#ifdef KB
P3(IB)
#else
G2(IB)
#endif
#else
#ifdef KB
v5(IB)
#else
C1(IB)
#endif
#endif
{
#ifdef CB
H(Z0,g);H(l2,c);
#endif
#ifdef KB
H(I0,c);
#endif
#ifdef L
H(h3,e);
#endif
#ifdef Z
H(J0,g);
#endif
#if defined(CB)&&defined(FB)
H(T1,e);
#endif
#ifdef CB
i j=m7(Z0,1. H2);e l=c9(l2,A.M4 f1);
#endif
#ifdef KB
i j=X6(EC,B5,I0,A.ub);e l=1.;
#endif
#ifdef Z
if(Z){e J4=max(D7(D5(J0)),d1(.0));l=min(J4,l);}
#endif
#ifdef ta
g2;
#endif
#if defined(L)&&!defined(VB)
if(L&&h3!=.0){E D0=unpackHalf2x16(T0(K0));e c6=D0.y;e m5=max(c6==h3?D0.x:d1(.0),d1(.0));l=min(l,m5);}
#endif
#ifdef KB
l*=p0.R3;
#endif
#if!defined(EB)&&!defined(VB)
i B1=S0(x0);
#ifdef FB
if(FB){
#ifdef CB
Y Z1=z6(T1);
#endif
#ifdef KB
j.xyz=d6(j);Y Z1=W1(p0.Z1);
#endif
if(Z1!=o5){j.xyz=p5(j.xyz,B1,Z1);}j.w*=l;j.xyz*=j.w;}else
#endif
{j*=l;}
#ifdef YB
if(YB){j=Z2(j);}
#endif
V0(x0,B1*(1.-j.w)+j);
#endif
#ifndef VB
L1(K0);L1(v1);
#endif
#ifdef ta
h2;
#endif
#ifdef EB
l1=j*l;
#endif
p2;}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive