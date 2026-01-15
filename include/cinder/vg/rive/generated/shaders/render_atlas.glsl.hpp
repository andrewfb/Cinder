#pragma once

#include "render_atlas.glsl.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char render_atlas[] = R"===(#ifdef BB
w1(c0)l0(0,g,SB);l0(1,g,TB);x1
#endif
V1 B0 X(0,g,J);N1
#ifdef BB
y1(FF,c0,F,B,O){n0(B,F,SB,g);n0(B,F,TB,g);T(J,g);g P;uint o0;c f0;if(K8(SB,TB,O,o0,f0,J U2)){M m4=E0(LB,o0*4u+2u);U V6=uintBitsToFloat(m4.yzw);f0=f0*V6.x+V6.yz;P=L7(f0,A.tb.x,A.tb.y);
#ifdef IC
P.y=-P.y;
#endif
}else{P=g(A.D2,A.D2,A.D2,A.D2);}g0(J);z1(P);}
#endif
#ifdef GB
#ifdef ZD
layout(location=0)inout highp uvec4 W5;
#ifdef LC
void main(){float l=uintBitsToFloat(W5.x);l+=k3(J);W5.x=floatBitsToUint(l);}
#endif
#ifdef MC
void main(){float l=uintBitsToFloat(W5.x);l=max(l,Q3(J));W5.x=floatBitsToUint(l);}
#endif
#elif defined(AE)
__pixel_localEXT g1{layout(r32f)highp float M2;};
#ifdef LC
void main(){M2+=k3(J);}
#endif
#ifdef MC
void main(){M2=max(M2,Q3(J));}
#endif
#elif defined(_EXPORTED_ATLAS_RENDER_TARGET_R32UI_PLS_ANGLE)
layout(binding=0,r32ui)uniform highp upixelLocalANGLE M2;
#ifdef LC
void main(){float l=uintBitsToFloat(pixelLocalLoadANGLE(M2).x);l+=k3(J);pixelLocalStoreANGLE(M2,M(floatBitsToUint(l)));}
#endif
#ifdef MC
void main(){float l=uintBitsToFloat(pixelLocalLoadANGLE(M2).x);l=max(l,Q3(J));pixelLocalStoreANGLE(M2,M(floatBitsToUint(l)));}
#endif
#elif defined(BE)
layout(binding=0,r32i)uniform highp coherent iimage2D zc;ivec2 Ac(){return ivec2(floor(q0));}int Bc(float l){return int(l*Ob);}
#ifdef LC
void main(){int l=Bc(k3(J));imageAtomicAdd(zc,Ac(),l);}
#endif
#ifdef MC
void main(){int l=Bc(Q3(J));imageAtomicMax(zc,Ac(),l);}
#endif
#elif defined(GF)
#ifdef LC
d2(i,IE){H(J,g);e l=k3(J f1);if(abs(l)>le-1e-3){e2(l>.0?c1(.0,.0,1./255.,.0):c1(.0,.0,.0,1./255.));}else{l*=1./W7;e2(c1(max(l,.0),max(-l,.0),.0,.0));}}
#endif
#ifdef MC
d2(i,JE){H(J,g);e l=Q3(J f1);l*=1./W7;e2(c1(l,.0,.0,.0));}
#endif
#else
#ifdef LC
d2(float,IE){H(J,g);e2(k3(J f1));}
#endif
#ifdef MC
d2(float,JE){H(J,g);e2(Q3(J f1));}
#endif
#endif
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive