#pragma once

#include "advanced_blend.glsl.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char advanced_blend[] = R"===(#ifdef GB
#ifdef RD
layout(
#ifdef RB
blend_support_all_equations
#else
blend_support_multiply,blend_support_screen,blend_support_overlay,blend_support_darken,blend_support_lighten,blend_support_colordodge,blend_support_colorburn,blend_support_hardlight,blend_support_softlight,blend_support_difference,blend_support_exclusion
#endif
)out;
#endif
#ifdef FB
#ifdef RB
e F8(p v0){return min(min(v0.x,v0.y),v0.z);}e ua(p v0){return max(max(v0.x,v0.y),v0.z);}e G8(p v0){return dot(v0,A0(.30,.59,.11));}e va(p v0){return ua(v0)-F8(v0);}p fd(p j){e G3=G8(j);e wa=F8(j);e xa=ua(j);if(wa<.0)j=G3+((j-G3)*G3)/(G3-wa);if(xa>1.)j=G3+((j-G3)*(1.-G3))/(xa-G3);return j;}p H8(p n5,p I8){e gd=G8(n5);e hd=G8(I8);e id=hd-gd;p j=n5+A0(id);return fd(j);}p ya(p n5,p jd,p I8){e kd=F8(n5);e za=va(n5);e ld=va(jd);p j;if(za>.0){j=(n5-kd)*ld/za;}else{j=A0(.0);}return H8(j,I8);}
#endif
p md(p e0,i p1,Y J8){p i0=d6(p1);p N0;switch(J8){
#if defined(AB)&&defined(SD)
case o5:N0=e0;break;
#endif
case nd:N0=e0.xyz*i0.xyz;break;case od:N0=e0.xyz+i0.xyz-e0.xyz*i0.xyz;break;case pd:{p e6=e0*i0;N0=2.0*mix(e6,e0+i0-e6-0.5,greaterThan(i0,A0(0.5)));break;}case qd:N0=min(e0.xyz,i0.xyz);break;case rd:N0=max(e0.xyz,i0.xyz);break;case sd:{p1.xyz=clamp(p1.xyz,A0(.0),p1.www);p Aa=clamp(1.-e0,A0(.0),A0(1.))*p1.w;N0=mix(min(A0(1.),p1.xyz/Aa),sign(p1.xyz),equal(Aa,A0(.0)));break;}case ud:{e0=clamp(e0,A0(.0),A0(1.));p1.xyz=clamp(p1.xyz,A0(.0),p1.www);if(p1.w==.0)p1.w=1.;p Ba=p1.w-p1.xyz;N0=1.-mix(min(A0(1.),Ba/(e0*p1.w)),sign(Ba),equal(e0,A0(.0)));break;}case vd:{p e6=e0*i0;N0=2.0*mix(e6,e0+i0-e6-0.5,greaterThan(e0,A0(0.5)));break;}case wd:{for(int w0=0;w0<3;++w0){if(e0[w0]<=0.5)N0[w0]=(1.0-i0[w0]);else if(i0[w0]<=0.25)N0[w0]=((16.0*i0[w0]-12.0)*i0[w0]+3.0);else N0[w0]=(inversesqrt(i0[w0])-1.0);}N0=i0+i0*(2.0*e0-1.0)*N0;break;}case xd:N0=abs(i0.xyz-e0.xyz);break;case yd:N0=e0.xyz+i0.xyz-2.*e0.xyz*i0.xyz;break;
#ifdef RB
case zd:if(RB){e0.xyz=clamp(e0.xyz,A0(.0),A0(1.));N0=ya(e0.xyz,i0.xyz,i0.xyz);}break;case Ad:if(RB){e0.xyz=clamp(e0.xyz,A0(.0),A0(1.));N0=ya(i0.xyz,e0.xyz,i0.xyz);}break;case Bd:if(RB){e0.xyz=clamp(e0.xyz,A0(.0),A0(1.));N0=H8(e0.xyz,i0.xyz);}break;case Cd:if(RB){e0.xyz=clamp(e0.xyz,A0(.0),A0(1.));N0=H8(i0.xyz,e0.xyz);}break;
#endif
}return N0;}d p p5(p e0,i p1,Y J8){p N0=md(e0,p1,J8);return mix(e0,N0,A0(p1.w));}
#endif
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive