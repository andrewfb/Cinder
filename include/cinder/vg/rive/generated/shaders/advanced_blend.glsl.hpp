#pragma once

#include "advanced_blend.glsl.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char advanced_blend[] = R"===(#ifdef GB
#ifdef RD
layout(
#ifdef WB
blend_support_all_equations
#else
blend_support_multiply,blend_support_screen,blend_support_overlay,blend_support_darken,blend_support_lighten,blend_support_colordodge,blend_support_colorburn,blend_support_hardlight,blend_support_softlight,blend_support_difference,blend_support_exclusion
#endif
)out;
#endif
#ifdef HB
#ifdef WB
d M8(r x0){return min(min(x0.x,x0.y),x0.z);}d Ba(r x0){return max(max(x0.x,x0.y),x0.z);}d N8(r x0){return dot(x0,E0(.30,.59,.11));}d Ca(r x0){return Ba(x0)-M8(x0);}r fd(r j){d Q3=N8(j);d Da=M8(j);d Ea=Ba(j);if(Da<.0)j=Q3+((j-Q3)*Q3)/(Q3-Da);if(Ea>1.)j=Q3+((j-Q3)*(1.-Q3))/(Ea-Q3);return j;}r O8(r q5,r P8){d gd=N8(q5);d hd=N8(P8);d id=hd-gd;r j=q5+E0(id);return fd(j);}r Fa(r q5,r jd,r P8){d kd=M8(q5);d Ga=Ca(q5);d ld=Ca(jd);r j;if(Ga>.0){j=(q5-kd)*ld/Ga;}else{j=E0(.0);}return O8(j,P8);}
#endif
r md(r h0,i y1,V Q8){r n0=H4(y1);r R0;switch(Q8){
#if defined(CB)&&defined(SD)
case r5:R0=h0;break;
#endif
case nd:R0=h0.xyz*n0.xyz;break;case od:R0=h0.xyz+n0.xyz-h0.xyz*n0.xyz;break;case pd:{r f6=h0*n0;R0=2.0*mix(f6,h0+n0-f6-0.5,greaterThan(n0,E0(0.5)));break;}case qd:R0=min(h0.xyz,n0.xyz);break;case rd:R0=max(h0.xyz,n0.xyz);break;case sd:{y1.xyz=clamp(y1.xyz,E0(.0),y1.www);r Ha=clamp(1.-h0,E0(.0),E0(1.))*y1.w;R0=mix(min(E0(1.),y1.xyz/Ha),sign(y1.xyz),equal(Ha,E0(.0)));break;}case ud:{h0=clamp(h0,E0(.0),E0(1.));y1.xyz=clamp(y1.xyz,E0(.0),y1.www);if(y1.w==.0)y1.w=1.;r Ia=y1.w-y1.xyz;R0=1.-mix(min(E0(1.),Ia/(h0*y1.w)),sign(Ia),equal(h0,E0(.0)));break;}case vd:{r f6=h0*n0;R0=2.0*mix(f6,h0+n0-f6-0.5,greaterThan(h0,E0(0.5)));break;}case wd:{for(int y0=0;y0<3;++y0){if(h0[y0]<=0.5)R0[y0]=(1.0-n0[y0]);else if(n0[y0]<=0.25)R0[y0]=((16.0*n0[y0]-12.0)*n0[y0]+3.0);else R0[y0]=(inversesqrt(n0[y0])-1.0);}R0=n0+n0*(2.0*h0-1.0)*R0;break;}case xd:R0=abs(n0.xyz-h0.xyz);break;case yd:R0=h0.xyz+n0.xyz-2.*h0.xyz*n0.xyz;break;
#ifdef WB
case zd:if(WB){h0.xyz=clamp(h0.xyz,E0(.0),E0(1.));R0=Fa(h0.xyz,n0.xyz,n0.xyz);}break;case Ad:if(WB){h0.xyz=clamp(h0.xyz,E0(.0),E0(1.));R0=Fa(n0.xyz,h0.xyz,n0.xyz);}break;case Bd:if(WB){h0.xyz=clamp(h0.xyz,E0(.0),E0(1.));R0=O8(h0.xyz,n0.xyz);}break;case Cd:if(WB){h0.xyz=clamp(h0.xyz,E0(.0),E0(1.));R0=O8(n0.xyz,h0.xyz);}break;
#endif
}return R0;}e r v5(r h0,i y1,V Q8){r R0=md(h0,y1,Q8);return mix(h0,R0,E0(y1.w));}
#endif
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive