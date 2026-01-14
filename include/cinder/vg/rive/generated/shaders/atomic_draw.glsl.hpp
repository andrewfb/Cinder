#pragma once

#include "atomic_draw.glsl.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char atomic_draw[] = R"===(#ifdef TC
#ifdef BB
g1(c0)i0(0,g,PB);i0(1,g,QB);h1
#endif
D1
#ifdef IB
q0 N(0,g,C);
#else
q0 N(0,G,C);
#endif
e3 N(1,V,e0);z1
#ifdef BB
p1(TB,c0,D,p,O){j0(p,D,PB,g);j0(p,D,QB,g);
#ifdef IB
L(C,g);
#else
L(C,G);
#endif
L(e0,V);g J;uint Z;c R;g B;if(p7(PB,QB,O,Z,R,B o2)){
#ifdef IB
C=B;
#else
C.xy=q7(B.xy);
#endif
e0=R1(Z);J=R2(R);}else{J=g(q.M1,q.M1,q.M1,q.M1);}W(C);W(e0);l1(J);}
#endif
#endif
#if defined(EB)||defined(AB)
#ifdef BB
g1(c0)i0(0,R3,KB);h1
#endif
D1
#ifdef AB
q0 N(0,c,i1);
#else
RB N(0,d,W0);
#endif
e3 N(1,V,e0);z1
#ifdef BB
p1(TB,c0,D,p,O){j0(p,D,KB,X);
#ifdef AB
L(i1,c);
#else
L(W0,d);
#endif
L(e0,V);uint Z;c R;
#ifdef AB
R=R8(KB,Z,i1 o2);
#else
R=S8(KB,Z,W0 o2);
#endif
e0=R1(Z);g J=R2(R);
#ifdef AB
W(i1);
#else
W(W0);
#endif
W(e0);l1(J);}
#endif
#endif
#ifdef BD
#ifdef BB
g1(c0)i0(0,g,AC);h1
#endif
D1 q0 N(0,c,r0);q0 N(1,d,I4);
#ifdef DB
q0 N(2,g,S0);
#endif
z1
#ifdef BB
r7(TB,c0,D,p,O){j0(p,D,AC,g);L(r0,c);L(I4,d);
#ifdef DB
L(S0,g);
#endif
bool T8=AC.z==.0||AC.w==.0;I4=T8?.0:1.;c R=AC.xy;d0 G0=N1(l0.v7);d0 g6=transpose(inverse(G0));if(!T8){float U8=S3*V8(g6[1])/dot(G0[1],g6[1]);if(U8>=.5){R.x=.5;I4*=J4(.5/U8);}else{R.x+=U8*AC.z;}float W8=S3*V8(g6[0])/dot(G0[0],g6[0]);if(W8>=.5){R.y=.5;I4*=J4(.5/W8);}else{R.y+=W8*AC.w;}}r0=R;R=H0(G0,R)+l0.j1;if(T8){c K4=H0(g6,AC.zw);K4*=V8(K4)/dot(K4,K4);R+=S3*K4;}
#ifdef DB
if(DB){S0=w7(N1(l0.c2),l0.p2,R);}
#endif
g J=R2(R);W(r0);W(I4);
#ifdef DB
W(S0);
#endif
l1(J);}
#endif
#elif defined(MB)
#ifdef BB
g1(q2)i0(0,c,XB);h1 g1(J2)i0(1,c,YB);h1
#endif
D1 q0 N(0,c,r0);
#ifdef DB
q0 N(1,g,S0);
#endif
z1
#ifdef BB
w5(TB,q2,r2,J2,K2,p){j0(p,r2,XB,c);j0(p,K2,YB,c);L(r0,c);
#ifdef DB
L(S0,g);
#endif
d0 G0=N1(l0.v7);c R=H0(G0,XB)+l0.j1;r0=YB;
#ifdef DB
if(DB){S0=w7(N1(l0.c2),l0.p2,R);}
#endif
g J=R2(R);W(r0);
#ifdef DB
W(S0);
#endif
l1(J);}
#endif
#endif
#ifdef ME
#ifdef BB
g1(c0)h1
#endif
D1 z1
#ifdef BB
p1(TB,c0,D,p,O){Y O1;O1.x=(p&1)==0?q.x7.x:q.x7.z;O1.y=(p&2)==0?q.x7.y:q.x7.w;g J=R2(c(O1));l1(J);}
#endif
#endif
#ifdef CD
#endif
#ifdef GB
v2
#ifndef FB
#ifdef TD
#define X8 TD
#else
#define X8 T3
#endif
#ifdef UC
U3(X8,z0);
#else
F0(X8,z0);
#endif
#endif
#ifdef BC
#define V3 i
#define Y8 X0
#define y7 T0(.0)
#define Ja(o) ((o).w!=.0)
#ifdef Q
#ifndef QC
F0(L4,M0);
#else
U3(L4,M0);
#endif
#endif
#else
#define V3 uint
#define y7 0u
#define Y8 Y0
#define Ja(o) ((o)!=0u)
#ifdef Q
Z0(L4,M0);
#endif
#endif
W3(h6,X3);w2 Y3 M4(Z8,Ka,HC);N4(a9,La,NB);Z3 e uint Dd(float x){return uint(round(x*c9+d9));}e d z7(uint x){return J4(float(x)*Ma+(-d9*Ma));}
#ifdef Q
e void Na(uint N0,V3 I0,O4(d)m){
#ifdef BC
if(all(lessThan(abs(I0.xy-unpackUnorm4x8(N0).xy),f3(.25/255.))))m=min(m,I0.z);else m=.0;
#else
if(N0==I0>>16)m=min(m,unpackHalf2x16(I0).x);else m=.0;
#endif
}
#endif
e void A7(uint Z,d L2,A1(i)T
#if defined(Q)&&!defined(QC)
,O4(V3)q1
#endif
i6 a4){a1 O0=Q4(HC,Z);d m=L2;if((O0.x&(Ed|e9))!=0u){m=abs(m);
#ifdef IC
if(IC&&(O0.x&e9)!=0u){m=1.-abs(fract(m*.5)*2.+-1.);}
#endif
}m=clamp(m,k1(.0),k1(1.));
#ifdef Q
if(Q){uint N0=O0.x>>16u;if(N0!=0u){Na(N0,Y8(M0),m);}}
#endif
#ifdef DB
if(DB&&(O0.x&Fd)!=0u){d0 G0=N1(A0(NB,Z*4u+2u));g j1=A0(NB,Z*4u+3u);c Gd=H0(G0,v0)+j1.xy;G Oa=q7(abs(Gd)*j1.zw-j1.zw);d R4=clamp(min(Oa.x,Oa.y)+.5,.0,1.);m=min(m,R4);}
#endif
uint S1=O0.x&0xfu;if(S1<=f9){T=unpackUnorm4x8(O0.y);
#ifdef Q
if(Q&&S1==B7){
#ifndef QC
#ifdef BC
q1.xy=T.zw;q1.z=m;q1.w=1.;
#else
q1=O0.y|packHalf2x16(f3(m,.0));
#endif
#endif
T=T0(.0);}
#endif
}else{d0 G0=N1(A0(NB,Z*4u));g j1=A0(NB,Z*4u+1u);c S2=H0(G0,v0)+j1.xy;float t=S1==C7?S2.x:length(S2);t=clamp(t,.0,1.);float x=t*j1.z+j1.w;float y=uintBitsToFloat(O0.y);T=d2(VC,g9,c(x,y),.0);}T.w*=m;
#if!defined(FB)&&defined(HB)
V e2;if(HB&&T.w!=.0&&(e2=R1((O0.x>>4)&0xfu))!=0u){i E1=X0(z0);T.xyz=v5(T.xyz,E1,e2);}
#endif
#ifndef BC
T.xyz*=T.w;
#endif
}
#if!defined(FB)&&!defined(UC)
e void D7(i T a4){
#ifndef BC
if(T.w==.0)return;float j6=1.-T.w;if(j6!=.0)T+=X0(z0)*j6;
#endif
c1(z0,T);}
#endif
#if defined(Q)&&!defined(QC)
e void h9(V3 q1 a4){
#ifdef BC
c1(M0,q1);
#else
if(q1!=0u)d1(M0,q1);
#endif
}
#endif
#ifdef FB
#define k6 M2
#define Qa c4
#define x5 S4
#else
#define k6 F1
#define Qa y5
#define x5 x2
#endif
#ifdef TC
k6(JB){
#ifdef IB
I(C,g);
#else
I(C,G);
#endif
I(e0,V);d E7;
#ifdef IB
if(IB&&i9(C)){E7=w3(C P0);}else if(IB&&F7(C)){E7=T2(C P0);}else
#endif
{E7=min(min(k1(C.x),abs(k1(C.y))),k1(1.));}i T=T0(.0);
#ifdef Q
V3 q1=y7;
#endif
uint G7=Dd(E7);uint Ra=(Sa(e0)<<z5)|G7;uint f2=A5(X3,Ra);V g3=R1(f2>>z5);if(g3==e0){if(!B5(C)){G7+=f2-max(Ra,f2);G7-=j9;C5(X3,G7);}}else{d L2=z7(f2&H7);A7(g3,L2,T
#ifdef Q
,q1
#endif
U2 P1);}
#ifdef FB
r1=T;
#else
D7(T P1);
#endif
#ifdef Q
h9(q1 P1);
#endif
x5}
#endif
#if defined(EB)||defined(AB)
k6(JB){
#ifdef AB
I(i1,c);
#else
I(W0,d);
#endif
I(e0,V);uint f2=x3(X3);V g3=R1(f2>>z5);uint k9;
#ifndef AB
if(g3==e0){k9=f2;}else
#endif
{k9=(Sa(e0)<<z5)+j9;}d m;
#ifdef AB
m=I7(i1,q.d4 P0);
#else
m=W0;
#endif
int Hd=int(round(m*c9));y3(X3,k9+uint(Hd));i T=T0(.0);
#ifdef Q
V3 q1=y7;
#endif
#ifndef AB
if(g3!=e0)
#endif
{d l9=z7(f2&H7);A7(g3,l9,T
#ifdef Q
,q1
#endif
U2 P1);}
#ifdef FB
r1=T;
#else
D7(T P1);
#endif
#ifdef Q
h9(q1 P1);
#endif
x5}
#endif
#ifdef CD
Qa(JB){I(r0,c);
#ifdef BD
I(I4,d);
#endif
#ifdef DB
I(S0,g);
#endif
i D5=l6(UB,z3,r0);d E5=1.;
#ifdef BD
E5=min(I4,E5);
#endif
#ifdef DB
if(DB){d R4=J7(F5(S0));E5=clamp(R4,k1(.0),E5);}
#endif
uint f2=x3(X3);V g3=R1(f2>>z5);d l9=z7(f2&H7);i T;
#ifdef Q
V3 q1=y7;
#endif
A7(g3,l9,T
#ifdef Q
,q1
#endif
U2 P1);
#ifdef BC
T.xyz*=T.w;
#endif
#ifdef Q
if(Q&&l0.N0!=0u){V3 I0=Ja(q1)?q1:Y8(M0);Na(l0.N0,I0,E5);}
#endif
#if!defined(FB)&&defined(HB)
if(HB&&l0.e2!=r5){i E1=X0(z0)*(1.-T.w)+T;D5.xyz=v5(H4(D5),E1,R1(l0.e2))*D5.w;}
#endif
D5*=E5*J4(l0.V2);T=T*(1.-D5.w)+D5;
#ifdef FB
r1=T;
#else
D7(T P1);
#endif
#ifdef Q
h9(q1 P1);
#endif
y3(X3,j9);x5}
#endif
#ifdef NE
k6(JB){
#ifdef OE
c1(z0,unpackUnorm4x8(q.Id));
#endif
#ifdef PE
i j=X0(z0);c1(z0,j.zyxw);
#endif
y3(X3,q.Jd);
#ifdef Q
if(Q){d1(M0,0u);}
#endif
#ifdef FB
discard;
#endif
x5}
#endif
#ifdef QC
#ifdef UC
M2(JB)
#else
k6(JB)
#endif
{uint f2=x3(X3);d L2=z7(f2&H7);V g3=R1(f2>>z5);i T;A7(g3,L2,T U2 P1);
#ifdef CC
T=h3(T);
#endif
#ifdef UC
#ifdef BC
T.xyz*=T.w;
#endif
float j6=1.-T.w;if(j6!=.0)T+=X0(z0)*j6;r1=T;S4
#else
#ifdef FB
r1=T;
#else
D7(T P1);
#endif
x5
#endif
}
#endif
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive