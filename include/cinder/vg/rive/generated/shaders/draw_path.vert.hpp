#pragma once

#include "draw_path.vert.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char draw_path_vert[] = R"===(#undef d6
#ifdef HB
#define d6 !HB
#else
#define d6 true
#endif
#undef c3
#ifdef IB
#define c3 g
#else
#define c3 G
#endif
#ifdef BB
g1(c0)
#if defined(EB)||defined(AB)
i0(0,R3,KB);
#else
i0(0,g,PB);i0(1,g,QB);
#endif
h1
#endif
D1 q0 N(0,g,o1);
#ifdef AB
q0 N(1,c,i1);
#elif!defined(CB)
#ifdef EB
RB N(1,d,W0);
#else
q0 N(2,c3,C);
#endif
RB N(3,d,e0);
#endif
#ifdef Q
#ifdef AB
RB N(4,d,r3);
#else
RB N(4,G,a2);
#endif
#endif
#if defined(DB)&&!defined(CB)
q0 N(5,g,S0);
#endif
#ifdef HB
RB N(6,d,m2);
#endif
z1
#ifdef BB
p1(TB,c0,D,p,O){
#if defined(EB)||defined(AB)
j0(p,D,KB,X);
#else
j0(p,D,PB,g);j0(p,D,QB,g);
#endif
L(o1,g);
#ifdef AB
L(i1,c);
#elif!defined(CB)
#ifdef EB
L(W0,d);
#else
L(C,c3);
#endif
L(e0,d);
#endif
#ifdef Q
#ifdef AB
L(r3,d);
#else
L(a2,G);
#endif
#endif
#if defined(DB)&&!defined(CB)
L(S0,g);
#endif
#ifdef HB
L(m2,d);
#endif
bool Zc=false;uint Z;c R;
#ifdef CB
V H8;
#endif
#ifdef AB
R=R8(KB,Z,
#ifdef CB
H8,
#endif
i1 o2);
#elif defined(EB)
R=S8(KB,Z
#ifdef CB
,H8
#else
,W0
#endif
o2);
#else
g B;Zc=!p7(PB,QB,O,Z,R
#ifndef CB
,B
#else
,H8
#endif
o2);
#ifndef CB
#ifdef IB
C=B;
#else
C.xy=q7(B.xy);
#endif
#endif
#endif
a1 O0=Q4(HC,Z);
#if!defined(AB)&&!defined(CB)
e0=S7(Z,q.L5);if((O0.x&e9)!=0u)e0=-e0;
#endif
uint S1=O0.x&0xfu;
#ifdef Q
if(Q){uint hg=(S1==B7?O0.y:O0.x)>>16;d N0=S7(hg,q.L5);if(S1==B7)N0=-N0;
#ifdef AB
r3=N0;
#else
a2.x=N0;
#endif
}
#endif
#ifdef HB
if(HB){m2=float((O0.x>>4)&0xfu);}
#endif
c o7=R;
#ifdef LF
o7.y=float(q.ee)-o7.y;
#endif
#ifdef DB
if(DB){d0 c2=N1(A0(NB,Z*4u+2u));g p2=A0(NB,Z*4u+3u);
#ifndef CB
S0=w7(c2,p2.xy,o7);
#else
yb(c2,p2.xy,o7 f5);
#endif
}
#endif
if(S1==f9){i j=unpackUnorm4x8(O0.y);if(d6)j.xyz*=j.w;o1=g(j);}
#if defined(Q)&&!defined(AB)
else if(Q&&S1==B7){d n5=S7(O0.x>>16,q.L5);a2.y=n5;}
#endif
else{d0 ig=N1(A0(NB,Z*4u));g I8=A0(NB,Z*4u+1u);c S2=H0(ig,o7)+I8.xy;if(S1==C7||S1==re){o1.w=-uintBitsToFloat(O0.y);float jg=I8.z;if(jg>.9){o1.z=2.;}else{o1.z=I8.w;}if(S1==C7){o1.y=.0;o1.x=S2.x;}else{o1.z=-o1.z;o1.xy=S2.xy;}}else{float V2=uintBitsToFloat(O0.y);float O6=I8.z;o1=g(S2.x,S2.y,V2,-2.-O6);}}g J;if(!Zc){J=R2(R);
#ifdef JC
J.y=-J.y;
#endif
#ifdef CB
J.z=H9(H8);
#endif
}else{J=g(q.M1,q.M1,q.M1,q.M1);}W(o1);
#ifdef AB
W(i1);
#elif!defined(CB)
#ifdef EB
W(W0);
#else
W(C);
#endif
W(e0);
#endif
#ifdef Q
#ifdef AB
W(r3);
#else
W(a2);
#endif
#endif
#if defined(DB)&&!defined(CB)
W(S0);
#endif
#ifdef HB
W(m2);
#endif
l1(J);}
#endif
#ifdef GB
Y3 Z3 e i J8(g d3,float m i6){i j;if(d3.w>=.0){j=F5(d3);if(d6)j*=m;else j.w*=m;}else if(d3.w>-1.){float t=d3.z>.0?d3.x:length(d3.xy);t=clamp(t,.0,1.);float ad=abs(d3.z);float x=ad>1.?(1.-1./I9)*t+(.5/I9):(1./I9)*t+ad;float kg=-d3.w;j=d2(VC,g9,c(x,kg),.0);j.w*=m;if(d6)j.xyz*=j.w;}else{d O6=-d3.w-2.;j=r8(UB,z3,d3.xy,O6);d V2=d3.z*m;if(d6)j*=V2;else j=T0(H4(j),j.w*V2);}return j;}
#if!defined(EB)&&!defined(AB)
e d bd(c3 B p3){
#ifdef IB
if(IB&&i9(B))return w3(B P0);else
#endif
return min(B.x,B.y);}e d cd(c3 B p3){
#if defined(IB)
if(IB&&F7(B))return T2(B P0);else
#endif
return B.x;}e d lg(c3 B p3){if(B5(B))return bd(B P0);else return cd(B P0);}e d mg(d G4,c3 B p3){if(B5(B)){d m0=bd(B P0);return max(m0,G4);}else{d m0=cd(B P0);return G4+m0;}}
#endif
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive