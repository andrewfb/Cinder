#pragma once

#include "draw_clockwise_path.frag.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char draw_clockwise_path_frag[] = R"===(#ifdef GB
n2
#ifndef EB
C0(J3,x0);
#endif
U0(C4,K0);
#ifndef EB
C0(J6,B2);
#endif
U0(h6,v1);o2
#ifdef EB
G2(IB)
#else
C1(IB)
#endif
{H(Z0,g);
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
e a1=
#ifdef DB
A1;
#else
Xc(J);
#endif
i e1;e j3;
#if defined(DB)&&defined(JB)
if(!JB)
#endif
{e1=m7(Z0,1. H2);j3=1.;
#ifdef Z
if(Z){e lg=D7(D5(J0));j3=min(lg,j3);}
#endif
}g2;
#if defined(DB)&&defined(JB)
if(JB){W0(v1,packHalf2x16(X2(a1,m0)));
#ifndef EB
f2(x0);
#endif
}else
#endif
{E r4=unpackHalf2x16(T0(v1));e D8=r4.y;e v4=D8==m0?r4.x:d1(.0);e dd=
#ifndef DB
y5(J)?max(v4,a1):
#endif
v4+a1;
#ifdef L
if(L&&M1.x!=.0){E D0=unpackHalf2x16(T0(K0));e l5=D0.y;e mg=l5==M1.x?D0.x:d1(.0);j3=min(mg,j3);}
#endif
j3=max(j3,.0);e ra=r9(v4,.0,j3);e I1=r9(dd,.0,j3);
#ifndef EB
i B1=S0(x0);
#ifdef FB
if(FB){if(T1!=H5(o5)&&I1!=.0){if(ra==.0){e1.xyz=p5(e1.xyz,B1,z6(T1));
#ifndef DB
if(I1<j3){V0(B2,e1);}
#endif
}else{e1=S0(B2);f2(B2);}}e1.xyz*=e1.w;}
#endif
#endif
e1*=(I1-ra)/max(1.-ra*e1.w,ve);
#ifndef DB
#ifdef FB
#define ed (!FB||T1==H5(o5))&&e1.w>=1.
#else
#define ed e1.w>=1.
#endif
tc(ed,v1,packHalf2x16(X2(dd,m0)));
#else
L1(v1);
#endif
#ifndef EB
sc(e1.w==.0,x0,B1*(1.-e1.w)+e1);
#endif
}L1(K0);h2;
#ifdef EB
l1=e1;
#endif
p2;}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive