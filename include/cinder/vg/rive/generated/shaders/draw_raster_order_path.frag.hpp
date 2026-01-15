#pragma once

#include "draw_raster_order_path.frag.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char draw_raster_order_path_frag[] = R"===(#ifdef GB
n2 C0(J3,x0);U0(C4,K0);C0(J6,B2);U0(h6,f7);o2 C1(IB){H(Z0,g);
#ifdef DB
H(A1,e);
#else
H(J,N2);
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
#if!defined(DB)
g2;
#endif
E r4=unpackHalf2x16(T0(f7));e D8=r4.y;e E2=D8==m0?r4.x:d1(.0);
#ifdef DB
E2+=A1;L1(f7);
#else
E2=cg(E2,J f1);W0(f7,packHalf2x16(X2(E2,m0)));
#endif
e l;
#ifdef WC
if(WC){l=r9(E2,d1(.0),d1(1.));}else
#endif
{l=abs(E2);
#ifdef HC
if(HC&&m0<.0){l=1.-d1(abs(fract(l*.5)*2.+-1.));}
#endif
l=min(l,d1(1.));}
#ifdef L
if(L&&M1.x<.0){e L0=-M1.x;
#ifdef NC
if(NC){e k5=M1.y;if(k5!=.0){E D0=unpackHalf2x16(T0(K0));e c6=D0.y;e F3;if(c6!=L0){F3=c6==k5?D0.x:.0;
#ifndef DB
V0(B2,c1(F3,.0,.0,.0));
#endif
}else{F3=S0(B2).x;
#ifndef DB
f2(B2);
#endif
}l=min(l,F3);}}
#endif
W0(K0,packHalf2x16(X2(l,L0)));f2(x0);}else
#endif
{
#ifdef L
if(L){e L0=M1.x;if(L0!=.0){E D0=unpackHalf2x16(T0(K0));e c6=D0.y;l=(c6==L0)?min(D0.x,l):d1(.0);}}
#endif
#ifdef Z
if(Z){e J4=D7(D5(J0));l=clamp(J4,d1(.0),l);}
#endif
i j=m7(Z0,l H2);i B1;if(D8!=m0){B1=S0(x0);
#ifndef DB
V0(B2,B1);
#endif
}else{B1=S0(B2);
#ifndef DB
f2(B2);
#endif
}
#ifdef FB
if(FB){if(T1!=H5(o5)){j.xyz=p5(j.xyz,B1,z6(T1));}j.xyz*=j.w;}
#endif
#ifdef YB
if(YB){j=Z2(j);}
#endif
j+=B1*(1.-j.w);V0(x0,j);L1(K0);}
#if!defined(DB)
h2;
#endif
p2;}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive