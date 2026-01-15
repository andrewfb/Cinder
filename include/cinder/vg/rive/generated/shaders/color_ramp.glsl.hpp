#pragma once

#include "color_ramp.glsl.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char color_ramp[] = R"===(#ifdef BB
w1(c0)
#ifdef j9
l0(0,uint,ED);l0(1,uint,FD);l0(2,uint,GD);l0(3,uint,HD);
#else
l0(0,M,ZB);
#endif
x1
#endif
V1 B0 X(0,i,v6);N1
#ifdef BB
n3 o3 U3 V3 i Td(uint j){return eb((M(j,j,j,j)>>M(16,8,0,24))&0xffu)/255.;}y1(SE,c0,F,B,O){
#ifdef j9
n0(O,F,ED,uint);n0(O,F,FD,uint);n0(O,F,GD,uint);n0(O,F,HD,uint);M ZB=M(ED,FD,GD,HD);
#else
n0(O,F,ZB,M);
#endif
T(v6,i);int K7=B>>1;float x=float(K7<=1?ZB.x&0xffffu:ZB.x>>16)/65536.;float k9=(B&1)==0?.0:1.;if(A.fb<.0){k9=1.-k9;}uint w6=ZB.y;float y=float(w6&~Ud)+k9;if((w6&gb)!=0u&&K7==0){if((w6&l9)!=0u)x=.0;else x-=hb;}if((w6&ib)!=0u&&K7==3){if((w6&l9)!=0u)x=1.;else x+=hb;}v6=Td(K7<=1?ZB.z:ZB.w);g P=L7(c(x,y),2.,A.fb);
#ifdef IC
P.y=-P.y;
#endif
g0(v6);z1(P);}
#endif
#ifdef GB
c3 d3 d2(i,TE){H(v6,i);e2(v6);}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive