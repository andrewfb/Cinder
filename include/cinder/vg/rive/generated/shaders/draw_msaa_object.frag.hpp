#pragma once

#include "draw_msaa_object.frag.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char draw_msaa_object_frag[] = R"===(#ifdef GB
#ifdef KB
c3 K2(W3,I7,EC);
#ifdef FB
M6(ID);
#endif
d3 N4 X3(W3,J7,B5)O4
#endif
d2(i,IB){
#ifdef KB
H(I0,c);
#else
H(Z0,g);
#ifdef CB
H(l2,c);
#endif
#ifdef FB
H(T1,e);
#endif
#endif
#ifdef KB
i j=X6(EC,B5,I0,A.ub)*p0.R3;
#else
e l=
#ifdef CB
c9(l2,A.M4 f1);
#else
1.;
#endif
i j=m7(Z0,l H2);
#endif
#ifdef FB
if(FB){
#ifndef EB
#ifdef KB
j.xyz=d6(j);Y Z1=W1(p0.Z1);
#else
Y Z1=z6(T1);
#endif
i B1=o8(ID);j.xyz=p5(j.xyz,B1,Z1);
#endif
j.xyz*=j.w;}
#endif
#ifdef YB
if(YB){j=Z2(j);}
#endif
e2(j);}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive