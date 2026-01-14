#pragma once

#include "draw_clockwise_atomic_path.glsl.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char draw_clockwise_atomic_path[] = R"===(#ifdef TC
#ifdef BB
g1(c0)i0(0,g,PB);i0(1,g,QB);h1
#endif
D1 e3 N(0,V,e0);q0 N(1,g,C);q0 N(2,c,i1);e3 N(3,a1,n3);N(4,c,D3);z1
#ifdef BB
p1(TB,c0,D,p,O){j0(p,D,PB,g);j0(p,D,QB,g);L(e0,V);L(C,g);L(n3,a1);L(D3,c);g J;uint Z;c R;if(p7(PB,QB,O,Z,R,C o2)){P Z2=A0(LB,Z*4u+3u);e0=Z;n3=Z2.xy;D3=R+uintBitsToFloat(Z2.zw);J=R2(R);}else{J=g(q.M1,q.M1,q.M1,q.M1);}W(e0);W(C);W(n3);W(D3);l1(J);}
#endif
#endif
#ifdef EB
#ifdef BB
g1(c0)i0(0,R3,KB);h1
#endif
D1 e3 N(0,V,e0);
#ifdef AB
q0 N(1,c,i1);
#else
RB N(1,d,W0);e3 N(2,a1,n3);N(3,c,D3);
#endif
z1
#ifdef BB
p1(TB,c0,D,p,O){j0(p,D,KB,X);
#ifdef AB
L(i1,c);
#else
#endif
L(e0,V);
#ifdef AB
L(i1,c);
#else
L(W0,d);L(n3,a1);L(D3,c);
#endif
uint Z;c R;
#ifdef AB
R=R8(KB,Z,i1 o2);
#else
R=S8(KB,Z,W0 o2);P Z2=A0(LB,Z*4u+3u);n3=Z2.xy;D3=R+uintBitsToFloat(Z2.zw);
#endif
e0=R1(Z);g J=R2(R);W(e0);
#ifdef AB
W(i1);
#else
W(W0);W(n3);W(D3);
#endif
l1(J);}
#endif
#endif
#ifdef GB
Y3 M4(Z8,Ka,HC);N4(a9,La,NB);Ie(te,Ug,B1);Z3
#ifdef OB
e void Je(d d8,uint X1){uint Qb=uint(abs(d8)*M9+.5);uint Rb=q.B3|(N6-Qb);uint o3=O9(B1,X1,Rb);if(o3>=q.B3){uint Ke=o3-max(o3,Rb);Sb(B1,X1,Ke-Qb);}}
#endif
e void Le(O4(float)E3,d m0,uint X1){if(min(E3,m0)>=1.){return;}d o;uint Me=uint(abs(m0)*M9+.5);uint o3=O9(B1,X1,q.B3|Me);if(o3<q.B3){o=m0;}else{d H1=J5(o3&L9)*N9;d F3=max(H1,m0);o=(F3-H1)/(1.-H1*E3);}E3*=o;}e void Ne(O4(float)E3,d r4,uint X1){uint P9=Oe(B1,X1);if(min(E3,r4)>=1.&&(P9<q.B3||P9>=(q.B3|N6))){return;}d o=.0;uint Q9=uint(abs(r4)*M9+.5);if(P9<q.B3){uint Tb=q.B3|(N6+Q9);uint o3=O9(B1,X1,Tb);if(o3<=q.B3){o=r4;
#ifdef EB
o=min(o,1.);
#endif
r4=.0;}else if(o3<Tb){uint Ub=(o3&L9)-N6;d H1=J5(Ub)*N9;d F3=r4;
#ifdef EB
F3=min(F3,1.);
#endif
o=(F3-H1)/(1.-H1*E3);Q9=Ub;r4=H1;}}if(r4>.0){uint Pe=Sb(B1,X1,Q9);d H1=A9(int((Pe&L9)-N6))*N9;d F3=H1+r4;H1=clamp(H1,.0,1.);F3=clamp(F3,.0,1.);d Vb=1.-H1*E3;if(Vb<=.0)discard;o+=(1.-o*E3)*(F3-H1)/Vb;}E3*=o;}U1(i,JB){I(e0,V);
#ifdef Vg
I(C,g);
#elif defined(AB)
I(i1,c);
#else
I(W0,d);
#endif
#ifndef AB
I(n3,a1);I(D3,c);
#endif
i D0;uint Z=e0;a1 O0=Q4(HC,Z);uint S1=O0.x&0xfu;if(S1<=f9){D0=unpackUnorm4x8(O0.y);}else{d0 G0=N1(A0(NB,Z*4u));g j1=A0(NB,Z*4u+1u);c S2=H0(G0,v0)+j1.xy;if(S1!=se){float t=S1==C7?S2.x:length(S2);t=clamp(t,.0,1.);float x=t*j1.z+j1.w;float y=uintBitsToFloat(O0.y);D0=d2(VC,g9,c(x,y),.0);}else{float V2=uintBitsToFloat(O0.y);float O6=j1.z;D0=d2(UB,z3,S2,O6);D0=T0(H4(D0),D0.w*V2);}}if(D0.w==.0){discard;}
#ifdef AB
D0.w*=I7(i1,q.d4 P0);
#else
uint X1=n3.x;uint Qe=n3.y;a1 P5=a1(floor(D3));X1+=(P5.y>>5)*(Qe<<5)+(P5.x>>5)*(32<<5);X1+=((P5.x&0x1f)>>2)*(32<<2)+((P5.y&0x1f)>>2)*(4<<2);X1+=(P5.y&0x3)*4+(P5.x&0x3);
#ifdef OB
if(OB){
#ifdef EB
d d8=-W0;
#else
d m0;
#ifdef IB
if(IB&&F7(C)){m0=T2(C P0);}else
#endif
{m0=C.x;}d d8=max(-m0,.0);
#endif
Je(d8,X1);discard;}
#endif
#ifndef EB
if(B5(C)){d m0;
#ifdef IB
if(IB&&i9(C)){m0=w3(C P0);}else
#endif
{m0=min(C.x,C.y);}m0=clamp(m0,.0,1.);Le(D0.w,m0,X1);}else
#endif
{
#ifdef EB
d m0=W0;
#else
d m0;
#ifdef IB
if(IB&&F7(C)){m0=T2(C P0);}else
#endif
{m0=C.x;}m0=clamp(m0,.0,1.);
#endif
Ne(D0.w,m0,X1);}
#endif
V1(D0);}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive