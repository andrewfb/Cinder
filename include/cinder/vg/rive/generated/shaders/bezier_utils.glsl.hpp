#pragma once

#include "bezier_utils.glsl.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char bezier_utils[] = R"===(#ifndef Sa
#define Sa g
#endif
#ifndef l6
#define l6 c
#endif
d float e9(c m,c b){float Kd=dot(m,b);float Ta=dot(m,m)*dot(b,b);return(Ta==.0)?1.:clamp(Kd*inversesqrt(Ta),-1.,1.);}d void Ld(c j0,c k0,c y0,c z0,q1(c) r,q1(c) D,q1(c) P1){P1=k0-j0;c m6=y0-k0;c E7=z0-j0;D=m6-P1;r=-3.*m6+E7;}d W f9(c j0,c k0,c y0,c z0){W t;t[0]=(any(notEqual(j0,k0))?k0:any(notEqual(k0,y0))?y0:z0)-j0;t[1]=z0-(any(notEqual(z0,y0))?y0:any(notEqual(y0,k0))?k0:j0);return t;}d float Md(c j0,c k0,c y0,c z0,float m1,float Nd){c r,D,P1;Ld(j0,k0,y0,z0,r,D,P1);c n6=3.*(((r*m1)+2.*D)*m1+P1);float Ua=length(n6);if(Ua==.0){return.0;}n6*=1./Ua;float F7=2.*dot(r,n6);float o6=3.*(F7*m1+4.*dot(D,n6))*m1+6.*dot(P1,n6);float g9=min(m1,1.-m1);float Od=(F7*g9*g9+o6)*g9;float Va=min(Nd,Od*.9999);float I2;if(F7==.0){I2=Va/o6;}else{float N=1./F7;float b=o6*N,v0=-Va*N;float p6=(-1./3.)*b,q6=.5*v0;float Wa=q6*q6-p6*p6*p6;if(Wa<.0){float G7=sqrt(p6);float X0=acos(q6/(G7*G7*G7));I2=-2.*G7*cos(X0*(1./3.)+(-a3*2./3.));}else{float r=pow(abs(q6)+sqrt(Wa),1./3.);if(q6<.0)r=-r;I2=r!=.0?r+p6/r:.0;}}I2=abs(I2);g t0011=m1+Sa(-I2,-I2,I2,I2);g Xa=(r.xyxy*t0011+2.*D.xyxy)*t0011+P1.xyxy;W q2=f9(j0,k0,y0,z0);c Pd=t0011.x<1e-3?q2[0]:Xa.xy;c Qd=t0011.z>1.-1e-3?q2[1]:Xa.zw;return acos(e9(Pd,Qd));}d float H7(float m,float b){m=b<.0?-m:m;b=abs(b);return m>.0?(m<b?m/b:1.):.0;}float Rd(c j0,c k0,c y0,c z0,q1(float) h9){c Ya=z0-j0;float Za=length(z0-j0);if(Za==.0){h9=.5;return.0;}c J2=l6(-Ya.y,Ya.x)/Za;float ab=dot(J2,y0-j0);float S3=dot(J2,k0-j0);float T3=S3-ab;
#if 0
float m=3.*T3;float bb=T3+S3;float v0=S3;float c2=sqrt(max(T3*T3+ab*S3,.0));if(bb<.0)c2=-c2;c2+=bb;c r6=l6(H7(c2,m),H7(v0,c2));c E5=3.*(r6*(r6*(r6*T3-(S3+T3))+S3));E5=abs(E5);h9=E5.x>E5.y?r6.x:r6.y;return max(E5.x,E5.y);
#else
float cb=3.*T3;float D=-S3-T3;float P1=S3;float t=.5;for(int w0=0;w0<3;++w0){float db=cb*t;t=H7(db*t-P1,2.*(db+D));}h9=t;return abs(t*(t*(t*cb+3.*D)+3.*P1));
#endif
}
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive