#pragma once

#include "draw_raster_order_path.frag.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char draw_raster_order_path_frag[] = R"===(#ifdef GB
v2 F0(T3,z0);Z0(L4,M0);F0(M6,H2);Z0(h6,j7);w2 F1(JB){I(o1,g);
#ifdef EB
I(W0,d);
#else
I(C,c3);
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
#if!defined(EB)
i2;
#endif
G Z2=unpackHalf2x16(Y0(j7));d K8=Z2.y;d L2=K8==e0?Z2.x:k1(.0);
#ifdef EB
L2+=W0;Q1(j7);
#else
L2=mg(L2,C P0);d1(j7,packHalf2x16(f3(L2,e0)));
#endif
d m;
#ifdef YC
if(YC){m=C9(L2,k1(.0),k1(1.));}else
#endif
{m=abs(L2);
#ifdef IC
if(IC&&e0<.0){m=1.-k1(abs(fract(m*.5)*2.+-1.));}
#endif
m=min(m,k1(1.));}
#ifdef Q
if(Q&&a2.x<.0){d N0=-a2.x;
#ifdef OC
if(OC){d n5=a2.y;if(n5!=.0){G I0=unpackHalf2x16(Y0(M0));d e6=I0.y;d P3;if(e6!=N0){P3=e6==n5?I0.x:.0;
#ifndef EB
c1(H2,T0(P3,.0,.0,.0));
#endif
}else{P3=X0(H2).x;
#ifndef EB
h2(H2);
#endif
}m=min(m,P3);}}
#endif
d1(M0,packHalf2x16(f3(m,N0)));h2(z0);}else
#endif
{
#ifdef Q
if(Q){d N0=a2.x;if(N0!=.0){G I0=unpackHalf2x16(Y0(M0));d e6=I0.y;m=(e6==N0)?min(I0.x,m):k1(.0);}}
#endif
#ifdef DB
if(DB){d R4=J7(F5(S0));m=clamp(R4,k1(.0),m);}
#endif
i j=J8(o1,m U2);i E1;if(K8!=e0){E1=X0(z0);
#ifndef EB
c1(H2,E1);
#endif
}else{E1=X0(H2);
#ifndef EB
h2(H2);
#endif
}
#ifdef HB
if(HB){if(m2!=J5(r5)){j.xyz=v5(j.xyz,E1,C6(m2));}j.xyz*=j.w;}
#endif
#ifdef CC
if(CC){j=h3(j);}
#endif
j+=E1*(1.-j.w);c1(z0,j);Q1(M0);}
#if!defined(EB)
j2;
#endif
x2;}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive