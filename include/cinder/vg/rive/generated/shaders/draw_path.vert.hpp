#pragma once

#include "draw_path.vert.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char draw_path_vert[] = R"===(#undef j5
#ifdef MF
#define j5 false
#elif defined(FB)
#define j5 !FB
#else
#define j5 true
#endif
#undef N2
#ifdef HB
#define N2 g
#else
#define N2 E
#endif
#ifdef BB
w1(c0)
#if defined(DB)||defined(CB)
l0(0,y4,MB);
#else
l0(0,g,SB);l0(1,g,TB);
#endif
x1
#endif
V1 B0 X(0,g,Z0);
#ifdef CB
B0 X(1,c,l2);
#elif!defined(AB)
#ifdef DB
OB X(1,e,A1);
#else
B0 X(2,N2,J);
#endif
OB X(3,e,m0);
#endif
#ifdef L
#ifdef CB
OB X(4,e,h3);
#else
OB X(4,E,M1);
#endif
#endif
#if defined(Z)&&!defined(AB)
B0 X(5,g,J0);
#endif
#ifdef FB
OB X(6,e,T1);
#endif
#ifdef VB
x4 X(7,j1,Z5);X(8,c,k7);
#endif
N1
#ifdef BB
y1(DC,c0,F,B,O){
#if defined(DB)||defined(CB)
n0(B,F,MB,U);
#else
n0(B,F,SB,g);n0(B,F,TB,g);
#endif
T(Z0,g);
#ifdef CB
T(l2,c);
#elif!defined(AB)
#ifdef DB
T(A1,e);
#else
T(J,N2);
#endif
T(m0,e);
#endif
#ifdef L
#ifdef CB
T(h3,e);
#else
T(M1,E);
#endif
#endif
#if defined(Z)&&!defined(AB)
T(J0,g);
#endif
#ifdef FB
T(T1,e);
#endif
#ifdef VB
T(Z5,j1);T(k7,c);
#endif
bool Tc=false;uint o0;c f0;
#ifdef AB
Y B8;
#endif
#ifdef CB
f0=Ca(MB,o0,
#ifdef AB
B8,
#endif
l2 U2);
#elif defined(DB)
f0=Da(MB,o0
#ifdef AB
,B8
#else
,A1
#endif
U2);
#else
g v;Tc=!K8(SB,TB,O,o0,f0
#ifndef AB
,v
#else
,B8
#endif
U2);
#ifndef AB
#ifdef HB
J=v;
#else
J.xy=n7(v.xy);
#endif
#endif
#endif
j1 k1=q5(QC,o0);
#if!defined(CB)&&!defined(AB)
m0=O7(o0,A.J5);if((k1.x&W8)!=0u)m0=-m0;
#endif
uint Q2=k1.x&0xfu;
#ifdef L
if(L){uint Yf=(Q2==x7?k1.y:k1.x)>>16;e L0=O7(Yf,A.J5);if(Q2==x7)L0=-L0;
#ifdef CB
h3=L0;
#else
M1.x=L0;
#endif
}
#endif
#ifdef FB
if(FB){T1=float((k1.x>>4)&0xfu);}
#endif
c l7=f0;
#ifdef NF
l7.y=float(A.ee)-l7.y;
#endif
#ifdef Z
if(Z){W Y1=X1(E0(PB,o0*4u+2u));g m2=E0(PB,o0*4u+3u);
#ifndef AB
J0=p7(Y1,m2.xy,l7);
#else
xb(Y1,m2.xy,l7 a5);
#endif
}
#endif
if(Q2==Ka){i j=unpackUnorm4x8(k1.y);if(j5)j.xyz*=j.w;Z0=g(j);}
#if defined(L)&&!defined(CB)
else if(L&&Q2==x7){e k5=O7(k1.x>>16,A.J5);M1.y=k5;}
#endif
else{W Zf=X1(E0(PB,o0*4u));g C8=E0(PB,o0*4u+1u);c K4=P0(Zf,l7)+C8.xy;if(Q2==X8||Q2==re){Z0.w=-uintBitsToFloat(k1.y);float ag=C8.z;if(ag>.9){Z0.z=2.;}else{Z0.z=C8.w;}if(Q2==X8){Z0.y=.0;Z0.x=K4.x;}else{Z0.z=-Z0.z;Z0.xy=K4.xy;}}else{float R3=uintBitsToFloat(k1.y);float oa=C8.z;Z0=g(K4.x,K4.y,R3,-2.-oa);}}g P;if(!Tc){P=H3(f0);
#ifdef IC
P.y=-P.y;
#endif
#ifdef AB
P.z=z9(B8);
#elif defined(VB)
M r4=E0(LB,o0*4u+3u);Z5=r4.xy;k7=f0+uintBitsToFloat(r4.zw);
#endif
}else{P=g(A.D2,A.D2,A.D2,A.D2);}g0(Z0);
#ifdef CB
g0(l2);
#elif!defined(AB)
#ifdef DB
g0(A1);
#else
g0(J);
#endif
g0(m0);
#endif
#ifdef L
#ifdef CB
g0(h3);
#else
g0(M1);
#endif
#endif
#if defined(Z)&&!defined(AB)
g0(J0);
#endif
#ifdef FB
g0(T1);
#endif
#ifdef VB
g0(Z5);g0(k7);
#endif
z1(P);}
#endif
#ifdef GB
D4 G4 d i m7(g T2,float l i6){i j;if(T2.w>=.0){j=D5(T2);if(j5)j*=l;else j.w*=l;}else if(T2.w>-1.){float t=T2.z>.0?T2.x:length(T2.xy);t=clamp(t,.0,1.);float Uc=abs(T2.z);float x=Uc>1.?(1.-1./A9)*t+(.5/A9):(1./A9)*t+Uc;float bg=-T2.w;j=F2(CD,Ma,c(x,bg),.0);j.w*=l;if(j5)j.xyz*=j.w;}else{e oa=-T2.w-2.;j=l8(EC,B5,T2.xy,oa);e R3=T2.z*l;if(j5)j*=R3;else j=c1(d6(j),j.w*R3);}return j;}
#if!defined(DB)&&!defined(CB)
d e Vc(N2 v f3){
#ifdef HB
if(HB&&Oa(v))return Q3(v f1);else
#endif
return min(v.x,v.y);}d e Wc(N2 v f3){
#if defined(HB)
if(HB&&Pa(v))return k3(v f1);else
#endif
return v.x;}d e Xc(N2 v f3){if(y5(v))return Vc(v f1);else return Wc(v f1);}d e cg(e v4,N2 v f3){if(y5(v)){e a1=Vc(v f1);return max(a1,v4);}else{e a1=Wc(v f1);return v4+a1;}}
#endif
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive