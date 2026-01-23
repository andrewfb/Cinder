#pragma once

#include "draw_clockwise_atomic_path.frag.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char draw_clockwise_atomic_path_frag[] = R"===(#ifdef GB
D4 E4(S8,Fa,QC);F4(T8,Ga,PB);jf(se,ih,v1);G4
#ifdef JB
d void dg(e eg,uint U1){uint Yc=uint(abs(eg)*E9+.5);uint Zc=A.p3|(K6-Yc);uint i3=Z9(v1,U1,Zc);if(i3>=A.p3){uint fg=i3-max(i3,Zc);rc(v1,U1,fg-Yc);}}
#endif
d void gg(H4(float) D3,e a1,uint U1){if(min(D3,a1)>=1.){return;}e o;uint hg=uint(abs(a1)*E9+.5);uint i3=Z9(v1,U1,A.p3|hg);if(i3<A.p3){o=a1;}else{e I1=H5(i3&D9)*F9;e E3=max(I1,a1);o=(E3-I1)/(1.-I1*D3);}D3*=o;}d void ig(H4(float) D3,e w4,uint U1){uint pa=kf(v1,U1);if(min(D3,w4)>=1.&&(pa<A.p3||pa>=(A.p3|K6))){return;}e o=.0;uint qa=uint(abs(w4)*E9+.5);if(pa<A.p3){uint ad=A.p3|(K6+qa);uint i3=Z9(v1,U1,ad);if(i3<=A.p3){o=w4;
#ifdef DB
o=min(o,1.);
#endif
w4=.0;}else if(i3<ad){uint bd=(i3&D9)-K6;e I1=H5(bd)*F9;e E3=w4;
#ifdef DB
E3=min(E3,1.);
#endif
o=(E3-I1)/(1.-I1*D3);qa=bd;w4=I1;}}if(w4>.0){uint jg=rc(v1,U1,qa);e I1=p9(int((jg&D9)-K6))*F9;e E3=I1+w4;I1=clamp(I1,.0,1.);E3=clamp(E3,.0,1.);e cd=1.-I1*D3;if(cd<=.0)discard;o+=(1.-o*D3)*(E3-I1)/cd;}D3*=o;}d2(i,IB){H(Z0,g);
#ifdef DB
T(A1,e);
#else
T(J,N2);
#endif
H(m0,e);
#ifdef L
H(M1,E);
#endif
#ifdef Z
H(J0,g);
#endif
#ifdef FB
H(T1,e);
#endif
H(Z5,j1);H(k7,c);i e1;
#if defined(DB)&&defined(JB)
if(!JB)
#endif
{e1=m7(Z0,1. H2);}if(e1.w==.0){discard;}e a1=
#ifdef DB
A1;
#else
Xc(J);
#endif
uint U1=Z5.x;uint kg=Z5.y;j1 a6=j1(floor(k7));U1+=(a6.y>>5)*(kg<<5)+(a6.x>>5)*(32<<5);U1+=((a6.x&0x1f)>>2)*(32<<2)+((a6.y&0x1f)>>2)*(4<<2);U1+=(a6.y&0x3)*4+(a6.x&0x3);
#ifdef JB
if(JB){dg(-a1,U1);discard;}
#endif
#ifndef DB
if(y5(J)){a1=clamp(a1,.0,1.);gg(e1.w,a1,U1);}else
#endif
{ig(e1.w,a1,U1);}e2(e1);}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive