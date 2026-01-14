#pragma once

#include "draw_msaa_object.frag.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char draw_msaa_object_frag[] = R"===(#ifdef GB
#ifdef MB
W2 z2(l3,w6,UB);
#ifdef HB
Q6(ID);
#endif
X2 i4 A3(l3,x6,z3)j4
#endif
U1(i,JB){
#ifdef MB
I(r0,c);
#else
I(o1,g);
#ifdef AB
I(i1,c);
#endif
#ifdef HB
I(m2,d);
#endif
#endif
#ifdef MB
i j=c7(UB,z3,r0,q.vb)*l0.V2;
#else
d m=
#ifdef AB
I7(i1,q.d4 P0);
#else
1.;
#endif
i j=J8(o1,m U2);
#endif
#ifdef HB
if(HB){
#ifndef FB
#ifdef MB
j.xyz=H4(j);V e2=R1(l0.e2);
#else
V e2=C6(m2);
#endif
i E1=x8(ID);j.xyz=v5(j.xyz,E1,e2);
#endif
j.xyz*=j.w;}
#endif
#ifdef CC
if(CC){j=h3(j);}
#endif
V1(j);}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive