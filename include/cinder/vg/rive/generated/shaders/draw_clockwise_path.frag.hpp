#pragma once

#include "draw_clockwise_path.frag.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char draw_clockwise_path_frag[] = R"===(#ifdef GB
v2
#ifndef FB
F0(T3,z0);
#endif
Z0(L4,M0);
#ifndef FB
F0(M6,H2);
#endif
Z0(h6,B1);w2
#ifdef FB
M2(JB)
#else
F1(JB)
#endif
{I(o1,g);
#ifdef EB
L(W0,d);
#else
L(C,c3);
#endif
I(e0,d);
#ifdef Q
I(a2,G);
#endif
#ifdef DB
I(S0,g);
#endif
#ifdef HB
I(m2,d);
#endif
d m0=
#ifdef EB
W0;
#else
lg(C);
#endif
i D0;d v3;
#if defined(EB)&&defined(OB)
if(!OB)
#endif
{D0=J8(o1,1. U2);v3=1.;
#ifdef DB
if(DB){d og=J7(F5(S0));v3=min(og,v3);}
#endif
}i2;
#if defined(EB)&&defined(OB)
if(OB){d1(B1,packHalf2x16(f3(m0,e0)));
#ifndef FB
h2(z0);
#endif
}else
#endif
{G Z2=unpackHalf2x16(Y0(B1));d K8=Z2.y;d G4=K8==e0?Z2.x:k1(.0);d dd=
#ifndef EB
B5(C)?max(G4,m0):
#endif
G4+m0;
#ifdef Q
if(Q&&a2.x!=.0){G I0=unpackHalf2x16(Y0(M0));d o5=I0.y;d pg=o5==a2.x?I0.x:k1(.0);v3=min(pg,v3);}
#endif
v3=max(v3,.0);d za=C9(G4,.0,v3);d H1=C9(dd,.0,v3);
#ifndef FB
i E1=X0(z0);
#ifdef HB
if(HB){if(m2!=J5(r5)&&H1!=.0){if(za==.0){D0.xyz=v5(D0.xyz,E1,C6(m2));
#ifndef EB
if(H1<v3){c1(H2,D0);}
#endif
}else{D0=X0(H2);h2(H2);}}D0.xyz*=D0.w;}
#endif
#endif
D0*=(H1-za)/max(1.-za*D0.w,xe);
#ifndef EB
#ifdef HB
#define ed (!HB||m2==J5(r5))&&D0.w>=1.
#else
#define ed D0.w>=1.
#endif
zc(ed,B1,packHalf2x16(f3(dd,e0)));
#else
Q1(B1);
#endif
#ifndef FB
yc(D0.w==.0,z0,E1*(1.-D0.w)+D0);
#endif
}Q1(M0);j2;
#ifdef FB
r1=D0;
#endif
x2;}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive