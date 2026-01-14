#pragma once

#include "bezier_utils.glsl.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char bezier_utils[] = R"===(#ifndef Ta
#define Ta g
#endif
#ifndef m6
#define m6 c
#endif
e float m9(c l,c b){float Kd=dot(l,b);float Ua=dot(l,l)*dot(b,b);return(Ua==.0)?1.:clamp(Kd*inversesqrt(Ua),-1.,1.);}e void Ld(c o0,c p0,c B0,c C0,A1(c)A,A1(c)F,A1(c)T1){T1=p0-o0;c n6=B0-p0;c K7=C0-o0;F=n6-T1;A=-3.*n6+K7;}e d0 n9(c o0,c p0,c B0,c C0){d0 t;t[0]=(any(notEqual(o0,p0))?p0:any(notEqual(p0,B0))?B0:C0)-o0;t[1]=C0-(any(notEqual(C0,B0))?B0:any(notEqual(B0,p0))?p0:o0);return t;}e float Md(c o0,c p0,c B0,c C0,float v1,float Nd){c A,F,T1;Ld(o0,p0,B0,C0,A,F,T1);c o6=3.*(((A*v1)+2.*F)*v1+T1);float Va=length(o6);if(Va==.0){return.0;}o6*=1./Va;float L7=2.*dot(A,o6);float p6=3.*(L7*v1+4.*dot(F,o6))*v1+6.*dot(T1,o6);float o9=min(v1,1.-v1);float Od=(L7*o9*o9+p6)*o9;float Wa=min(Nd,Od*.9999);float N2;if(L7==.0){N2=Wa/p6;}else{float S=1./L7;float b=p6*S,x0=-Wa*S;float q6=(-1./3.)*b,r6=.5*x0;float Xa=r6*r6-q6*q6*q6;if(Xa<.0){float M7=sqrt(q6);float e1=acos(r6/(M7*M7*M7));N2=-2.*M7*cos(e1*(1./3.)+(-i3*2./3.));}else{float A=pow(abs(r6)+sqrt(Xa),1./3.);if(r6<.0)A=-A;N2=A!=.0?A+q6/A:.0;}}N2=abs(N2);g t0011=v1+Ta(-N2,-N2,N2,N2);g Ya=(A.xyxy*t0011+2.*F.xyxy)*t0011+T1.xyxy;d0 y2=n9(o0,p0,B0,C0);c Pd=t0011.x<1e-3?y2[0]:Ya.xy;c Qd=t0011.z>1.-1e-3?y2[1]:Ya.zw;return acos(m9(Pd,Qd));}e float N7(float l,float b){l=b<.0?-l:l;b=abs(b);return l>.0?(l<b?l/b:1.):.0;}float Rd(c o0,c p0,c B0,c C0,A1(float)p9){c Za=C0-o0;float ab=length(C0-o0);if(ab==.0){p9=.5;return.0;}c O2=m6(-Za.y,Za.x)/ab;float bb=dot(O2,B0-o0);float e4=dot(O2,p0-o0);float f4=e4-bb;
#if 0
float l=3.*f4;float cb=f4+e4;float x0=e4;float g2=sqrt(max(f4*f4+bb*e4,.0));if(cb<.0)g2=-g2;g2+=cb;c v6=m6(N7(g2,l),N7(x0,g2));c G5=3.*(v6*(v6*(v6*f4-(e4+f4))+e4));G5=abs(G5);p9=G5.x>G5.y?v6.x:v6.y;return max(G5.x,G5.y);
#else
float db=3.*f4;float F=-e4-f4;float T1=e4;float t=.5;for(int y0=0;y0<3;++y0){float eb=db*t;t=N7(eb*t-T1,2.*(eb+F));}p9=t;return abs(t*(t*(t*db+3.*F)+3.*T1));
#endif
}
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive