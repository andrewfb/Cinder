#pragma once

#include "atomic_draw.glsl.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char atomic_draw[] = R"===(#ifdef ZC
#ifdef BB
w1(c0)l0(0,g,SB);l0(1,g,TB);x1
#endif
V1
#ifdef HB
B0 X(0,g,J);
#else
B0 X(0,E,J);
#endif
x4 X(1,Y,m0);N1
#ifdef BB
y1(DC,c0,F,B,O){n0(B,F,SB,g);n0(B,F,TB,g);
#ifdef HB
T(J,g);
#else
T(J,E);
#endif
T(m0,Y);g P;uint o0;c f0;g v;if(K8(SB,TB,O,o0,f0,v U2)){
#ifdef HB
J=v;
#else
J.xy=n7(v.xy);
#endif
m0=W1(o0);P=H3(f0);}else{P=g(A.D2,A.D2,A.D2,A.D2);}g0(J);g0(m0);z1(P);}
#endif
#endif
#if defined(DB)||defined(CB)
#ifdef BB
w1(c0)l0(0,y4,MB);x1
#endif
V1
#ifdef CB
B0 X(0,c,l2);
#else
OB X(0,e,A1);
#endif
x4 X(1,Y,m0);N1
#ifdef BB
y1(DC,c0,F,B,O){n0(B,F,MB,U);
#ifdef CB
T(l2,c);
#else
T(A1,e);
#endif
T(m0,Y);uint o0;c f0;
#ifdef CB
f0=Ca(MB,o0,l2 U2);
#else
f0=Da(MB,o0,A1 U2);
#endif
m0=W1(o0);g P=H3(f0);
#ifdef CB
g0(l2);
#else
g0(A1);
#endif
g0(m0);z1(P);}
#endif
#endif
#ifdef AD
#ifdef BB
w1(c0)l0(0,g,WB);x1
#endif
V1 B0 X(0,c,I0);B0 X(1,e,z4);
#ifdef Z
B0 X(2,g,J0);
#endif
N1
#ifdef BB
o7(DC,c0,F,B,O){n0(B,F,WB,g);T(I0,c);T(z4,e);
#ifdef Z
T(J0,g);
#endif
bool L8=WB.z==.0||WB.w==.0;z4=L8?.0:1.;c f0=WB.xy;W O0=X1(p0.M8);W f6=transpose(inverse(O0));if(!L8){float N8=I3*O8(f6[1])/dot(O0[1],f6[1]);if(N8>=.5){f0.x=.5;z4*=A4(.5/N8);}else{f0.x+=N8*WB.z;}float P8=I3*O8(f6[0])/dot(O0[0],f6[0]);if(P8>=.5){f0.y=.5;z4*=A4(.5/P8);}else{f0.y+=P8*WB.w;}}I0=f0;f0=P0(O0,f0)+p0.O1;if(L8){c B4=P0(f6,WB.zw);B4*=O8(B4)/dot(B4,B4);f0+=I3*B4;}
#ifdef Z
if(Z){J0=p7(X1(p0.Y1),p0.m2,f0);}
#endif
g P=H3(f0);g0(I0);g0(z4);
#ifdef Z
g0(J0);
#endif
z1(P);}
#endif
#elif defined(KB)
#ifdef BB
w1(O2)l0(0,c,FC);x1 w1(V2)l0(1,c,GC);x1
#endif
V1 B0 X(0,c,I0);
#ifdef Z
B0 X(1,g,J0);
#endif
N1
#ifdef BB
g6(DC,O2,P2,V2,W2,B){n0(B,P2,FC,c);n0(B,W2,GC,c);T(I0,c);
#ifdef Z
T(J0,g);
#endif
W O0=X1(p0.M8);c f0=P0(O0,FC)+p0.O1;I0=GC;
#ifdef Z
if(Z){J0=p7(X1(p0.Y1),p0.m2,f0);}
#endif
g P=H3(f0);g0(I0);
#ifdef Z
g0(J0);
#endif
z1(P);}
#endif
#endif
#ifdef NE
#ifdef BB
w1(c0)x1
#endif
V1 N1
#ifdef BB
y1(DC,c0,F,B,O){S J1;J1.x=(B&1)==0?A.q7.x:A.q7.z;J1.y=(B&2)==0?A.q7.y:A.q7.w;g P=H3(c(J1));z1(P);}
#endif
#endif
#ifdef BD
#endif
#ifdef GB
n2
#ifndef EB
#ifdef TD
#define Q8 TD
#else
#define Q8 J3
#endif
#ifdef TC
K3(Q8,x0);
#else
C0(Q8,x0);
#endif
#endif
#ifdef XB
#define L3 i
#define R8 S0
#define r7 c1(.0)
#define Ea(o) ((o).w!=.0)
#ifdef L
#ifndef PC
C0(C4,K0);
#else
K3(C4,K0);
#endif
#endif
#else
#define L3 uint
#define r7 0u
#define R8 T0
#define Ea(o) ((o)!=0u)
#ifdef L
U0(C4,K0);
#endif
#endif
M3(h6,N3);o2 D4 E4(S8,Fa,QC);F4(T8,Ga,PB);G4 d uint Dd(float x){return uint(round(x*U8+V8));}d e v7(uint x){return A4(float(x)*Ha+(-V8*Ha));}
#ifdef L
d void Ia(uint L0,L3 D0,H4(e)l){
#ifdef XB
if(all(lessThan(abs(D0.xy-unpackUnorm4x8(L0).xy),X2(.25/255.))))l=min(l,D0.z);else l=.0;
#else
if(L0==D0>>16)l=min(l,unpackHalf2x16(D0).x);else l=.0;
#endif
}
#endif
d void w7(uint o0,e E2,q1(i)Q
#if defined(L)&&!defined(PC)
,H4(L3)i1
#endif
i6 O3){j1 k1=q5(QC,o0);e l=E2;if((k1.x&(Ed|W8))!=0u){l=abs(l);
#ifdef HC
if(HC&&(k1.x&W8)!=0u){l=1.-abs(fract(l*.5)*2.+-1.);}
#endif
}l=clamp(l,d1(.0),d1(1.));
#ifdef L
if(L){uint L0=k1.x>>16u;if(L0!=0u){Ia(L0,R8(K0),l);}}
#endif
#ifdef Z
if(Z&&(k1.x&Fd)!=0u){W O0=X1(E0(PB,o0*4u+2u));g O1=E0(PB,o0*4u+3u);c Gd=P0(O0,q0)+O1.xy;E Ja=n7(abs(Gd)*O1.zw-O1.zw);e J4=clamp(min(Ja.x,Ja.y)+.5,.0,1.);l=min(l,J4);}
#endif
uint Q2=k1.x&0xfu;if(Q2<=Ka){Q=unpackUnorm4x8(k1.y);
#ifdef L
if(L&&Q2==x7){
#ifndef PC
#ifdef XB
i1.xy=Q.zw;i1.z=l;i1.w=1.;
#else
i1=k1.y|packHalf2x16(X2(l,.0));
#endif
#endif
Q=c1(.0);}
#endif
}else{W O0=X1(E0(PB,o0*4u));g O1=E0(PB,o0*4u+1u);c K4=P0(O0,q0)+O1.xy;float t=Q2==X8?K4.x:length(K4);t=clamp(t,.0,1.);float x=t*O1.z+O1.w;float y=uintBitsToFloat(k1.y);Q=F2(CD,Ma,c(x,y),.0);}Q.w*=l;
#if!defined(EB)&&defined(FB)
Y Z1;if(FB&&Q.w!=.0&&(Z1=W1((k1.x>>4)&0xfu))!=0u){i B1=S0(x0);Q.xyz=p5(Q.xyz,B1,Z1);}
#endif
#ifndef XB
Q.xyz*=Q.w;
#endif
}
#if!defined(EB)&&!defined(TC)
d void y7(i Q O3){
#ifndef XB
if(Q.w==.0)return;float j6=1.-Q.w;if(j6!=.0)Q+=S0(x0)*j6;
#endif
V0(x0,Q);}
#endif
#if defined(L)&&!defined(PC)
d void Y8(L3 i1 O3){
#ifdef XB
V0(K0,i1);
#else
if(i1!=0u)W0(K0,i1);
#endif
}
#endif
#ifdef EB
#define k6 G2
#define Na P3
#define r5 L4
#else
#define k6 C1
#define Na v5
#define r5 p2
#endif
#ifdef ZC
k6(IB){
#ifdef HB
H(J,g);
#else
H(J,E);
#endif
H(m0,Y);e z7;
#ifdef HB
if(HB&&Oa(J)){z7=Q3(J f1);}else if(HB&&Pa(J)){z7=k3(J f1);}else
#endif
{z7=min(min(d1(J.x),abs(d1(J.y))),d1(1.));}i Q=c1(.0);
#ifdef L
L3 i1=r7;
#endif
uint A7=Dd(z7);uint Qa=(Ra(m0)<<w5)|A7;uint a2=x5(N3,Qa);Y Y2=W1(a2>>w5);if(Y2==m0){if(!y5(J)){A7+=a2-max(Qa,a2);A7-=Z8;z5(N3,A7);}}else{e E2=v7(a2&B7);w7(Y2,E2,Q
#ifdef L
,i1
#endif
H2 K1);}
#ifdef EB
l1=Q;
#else
y7(Q K1);
#endif
#ifdef L
Y8(i1 K1);
#endif
r5}
#endif
#if defined(DB)||defined(CB)
k6(IB){
#ifdef CB
H(l2,c);
#else
H(A1,e);
#endif
H(m0,Y);uint a2=l3(N3);Y Y2=W1(a2>>w5);uint a9;
#ifndef CB
if(Y2==m0){a9=a2;}else
#endif
{a9=(Ra(m0)<<w5)+Z8;}e l;
#ifdef CB
l=c9(l2,A.M4 f1);
#else
l=A1;
#endif
int Hd=int(round(l*U8));m3(N3,a9+uint(Hd));i Q=c1(.0);
#ifdef L
L3 i1=r7;
#endif
#ifndef CB
if(Y2!=m0)
#endif
{e d9=v7(a2&B7);w7(Y2,d9,Q
#ifdef L
,i1
#endif
H2 K1);}
#ifdef EB
l1=Q;
#else
y7(Q K1);
#endif
#ifdef L
Y8(i1 K1);
#endif
r5}
#endif
#ifdef BD
Na(IB){H(I0,c);
#ifdef AD
H(z4,e);
#endif
#ifdef Z
H(J0,g);
#endif
i A5=C7(EC,B5,I0);e C5=1.;
#ifdef AD
C5=min(z4,C5);
#endif
#ifdef Z
if(Z){e J4=D7(D5(J0));C5=clamp(J4,d1(.0),C5);}
#endif
uint a2=l3(N3);Y Y2=W1(a2>>w5);e d9=v7(a2&B7);i Q;
#ifdef L
L3 i1=r7;
#endif
w7(Y2,d9,Q
#ifdef L
,i1
#endif
H2 K1);
#ifdef XB
Q.xyz*=Q.w;
#endif
#ifdef L
if(L&&p0.L0!=0u){L3 D0=Ea(i1)?i1:R8(K0);Ia(p0.L0,D0,C5);}
#endif
#if!defined(EB)&&defined(FB)
if(FB&&p0.Z1!=o5){i B1=S0(x0)*(1.-Q.w)+Q;A5.xyz=p5(d6(A5),B1,W1(p0.Z1))*A5.w;}
#endif
A5*=C5*A4(p0.R3);Q=Q*(1.-A5.w)+A5;
#ifdef EB
l1=Q;
#else
y7(Q K1);
#endif
#ifdef L
Y8(i1 K1);
#endif
m3(N3,Z8);r5}
#endif
#ifdef OE
k6(IB){
#ifdef PE
V0(x0,unpackUnorm4x8(A.Id));
#endif
#ifdef QE
i j=S0(x0);V0(x0,j.zyxw);
#endif
m3(N3,A.Jd);
#ifdef L
if(L){W0(K0,0u);}
#endif
#ifdef EB
discard;
#endif
r5}
#endif
#ifdef PC
#ifdef TC
G2(IB)
#else
k6(IB)
#endif
{uint a2=l3(N3);e E2=v7(a2&B7);Y Y2=W1(a2>>w5);i Q;w7(Y2,E2,Q H2 K1);
#ifdef YB
Q=Z2(Q);
#endif
#ifdef TC
#ifdef XB
Q.xyz*=Q.w;
#endif
float j6=1.-Q.w;if(j6!=.0)Q+=S0(x0)*j6;l1=Q;L4
#else
#ifdef EB
l1=Q;
#else
y7(Q K1);
#endif
r5
#endif
}
#endif
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive