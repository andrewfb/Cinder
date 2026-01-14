#pragma once

#include "color_ramp.glsl.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char color_ramp[] = R"===(#ifdef BB
g1(c0)
#ifdef r9
i0(0,uint,ED);i0(1,uint,FD);i0(2,uint,GD);i0(3,uint,HD);
#else
i0(0,P,DC);
#endif
h1
#endif
D1 q0 N(0,i,y6);z1
#ifdef BB
j3 k3 g4 h4 i Td(uint j){return fb((P(j,j,j,j)>>P(16,8,0,24))&0xffu)/255.;}p1(RE,c0,D,p,O){
#ifdef r9
j0(O,D,ED,uint);j0(O,D,FD,uint);j0(O,D,GD,uint);j0(O,D,HD,uint);P DC=P(ED,FD,GD,HD);
#else
j0(O,D,DC,P);
#endif
L(y6,i);int O7=p>>1;float x=float(O7<=1?DC.x&0xffffu:DC.x>>16)/65536.;float v9=(p&1)==0?.0:1.;if(q.gb<.0){v9=1.-v9;}uint z6=DC.y;float y=float(z6&~Ud)+v9;if((z6&hb)!=0u&&O7==0){if((z6&w9)!=0u)x=.0;else x-=ib;}if((z6&jb)!=0u&&O7==3){if((z6&w9)!=0u)x=1.;else x+=ib;}y6=Td(O7<=1?DC.z:DC.w);g J=P7(c(x,y),2.,q.gb);
#ifdef JC
J.y=-J.y;
#endif
W(y6);l1(J);}
#endif
#ifdef GB
W2 X2 U1(i,SE){I(y6,i);V1(y6);}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive