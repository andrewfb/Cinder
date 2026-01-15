#pragma once

#include "common.glsl.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char common[] = R"===(#define a3 3.14159265359
#define M7 6.28318530718
#define x6 1.57079632679
#ifndef AB
#define I3 float(.5)
#else
#define I3 float(.0)
#endif
#define H3(k) L7(k,A.Vd,A.Wd)
#ifdef UE
#define jb(K,f,a) P4(K,f,a)
#define Y3 g
#define m9(o) o
#define F5(o) o
#define n9(o) uintBitsToFloat(o)
#define Q4(o) floatBitsToUint(o)
#else
#define jb(K,f,a) Z3(K,f,a)
#define Y3 M
#define m9(o) floatBitsToUint(o)
#define F5(o) uintBitsToFloat(o)
#define n9(o) o
#define Q4(o) o
#endif
#define kb(a,k,N7) D1(a,S(k)+S(-1,0))N7,D1(a,S(k)+S(0,0))N7,D1(a,S(k)+S(0,-1))N7,D1(a,S(k)+S(-1,-1))N7
#define a4(o) y6(RC,o9,o,lb,float(lb),.0).x
#define G5(o) y6(RC,o9,o,mb,float(mb),.0).x
#ifdef nb
d e A4(float x){return x;}d e H5(uint x){return float(x);}d e Xd(Y x){return float(x);}d e p9(int x){return float(x);}d i D5(g xyzw){return xyzw;}d E n7(c xy){return xy;}d i eb(M xyzw){return vec4(xyzw);}d Y z6(e x){return uint(x);}d Y W1(uint x){return x;}
#else
d e A4(float x){return(e)x;}d e H5(uint x){return(e)x;}d e Xd(Y x){return(e)x;}d e p9(int x){return(e)x;}d i D5(g xyzw){return(i)xyzw;}d E n7(c xy){return(E)xy;}d i eb(M xyzw){return(i)xyzw;}d Y z6(e x){return(Y)x;}d Y W1(uint x){return(Y)x;}
#endif
d e d1(e x){return x;}d E X2(E xy){return xy;}d E X2(e x,e y){E I;I.x=x,I.y=y;return I;}d E X2(e x){E I;I.x=x,I.y=x;return I;}d c l6(float x){return c(x,x);}d p A0(e x,e y,e z){p I;I.x=x,I.y=y,I.z=z;return I;}d p A0(e x){p I;I.x=x,I.y=x,I.z=x;return I;}d i c1(e x,e y,e z,e w){i I;I.x=x,I.y=y,I.z=z,I.w=w;return I;}d i c1(p xyz,e w){i I;I.xyz=xyz;I.w=w;return I;}d i c1(e x){i I;I.x=x,I.y=x,I.z=x,I.w=x;return I;}d i c1(i x){return x;}d c4 Yd(bool b){return c4(b,b);}d A6 rg(p m,p b,p v0){A6 I;I[0]=m;I[1]=b;I[2]=v0;return I;}d B6 sg(p m,p b){B6 I;I[0]=m;I[1]=b;return I;}d d4 Zd(i m,i b,i v0,i ae){d4 I;I[0]=m;I[1]=b;I[2]=v0;I[3]=ae;return I;}d W X1(g x){return W(x.xy,x.zw);}d uint Ra(Y x){return x;}d c I5(c m,c b,float t){return(b-m)*t+m;}d e O7(uint ob,uint J5){return ob==0u?.0:unpackHalf2x16((ob+be)*J5).x;}d float pb(c Q1){Q1=normalize(Q1);float X0=acos(clamp(Q1.x,-1.,1.));return Q1.y>=.0?X0:-X0;}d i tg(i j){return c1(j.xyz*j.w,j.w);}d p d6(i q9){return q9.xyz*(q9.w!=.0?1./q9.w:.0);}d e D7(i qb){E rb=min(qb.xy,qb.zw);e ce=min(rb.x,rb.y);return ce;}d float O8(c x){return abs(x.x)+abs(x.y);}d e r9(e x,e v9,e w9){
#if defined(VE)||defined(SC)
#ifdef SC
if(SC==de)
#endif
{if(x<w9)if(x>v9)return x;else return v9;else return w9;}
#endif
return clamp(x,v9,w9);}
#ifndef UNIFORM_DEFINITIONS_AUTO_GENERATED
K5(R2,NB)float fb;float sb;float Vd;float Wd;uint C6;uint ee;uint Id;uint Jd;e4 q7;c M4;c tb;uint p3;uint J5;float D2;float ub;uint fe;D6(A)
#endif
#ifdef BB
d g L7(c vb,float ge,float wb){return g(vb.x*ge-1.,vb.y*wb-sign(wb),0.,1.);}
#ifndef AB
d g p7(W Y1,c m2,c x9){c y9=abs(Y1[0])+abs(Y1[1]);if(y9.x!=.0&&y9.y!=.0){c N=1./y9;c R4=P0(Y1,x9)+m2;const float he=.5;return g(R4,-R4)*N.xyxy+N.xyxy+he;}else{return m2.xyxy;}}
#else
d float z9(uint E6){return 1.-float(E6)*(2./32768.);}
#ifdef Z
d void xb(W Y1,c m2,c x9 F6){
#ifndef VD
if(any(notEqual(g(Y1),g(.0,.0,.0,.0)))){c R4=P0(Y1,x9)+m2.xy;gl_ClipDistance[0]=R4.x+1.;gl_ClipDistance[1]=R4.y+1.;gl_ClipDistance[2]=1.-R4.x;gl_ClipDistance[3]=1.-R4.y;}else{gl_ClipDistance[0]=gl_ClipDistance[1]=gl_ClipDistance[2]=gl_ClipDistance[3]=m2.x-.5;}
#endif
}
#endif
#endif
#endif
#ifdef GB
#ifdef YB
d e Z2(e j){return(j<=0.04045)?j/12.92:pow(abs((j+0.055)/1.055),2.4);}d p Z2(p j){return A0(Z2(j.x),Z2(j.y),Z2(j.z));}d i Z2(i j){return c1(Z2(j.xyz),j.w);}
#endif
#endif
#ifdef BD
#ifndef UNIFORM_DEFINITIONS_AUTO_GENERATED
K5(L5,JC)g M8;c O1;float R3;float ug;g Y1;c m2;uint L0;uint Z1;uint E6;D6(p0)
#endif
#endif
#if defined(GB)&&defined(AB)&&!defined(EB)
d i yb(d4 G6,int P7){if(P7==0xf){return(G6[0]+G6[1]+G6[2]+G6[3])*.25;}else{i ie=g(notEqual(P7&e4(1,2,4,8),e4(0)));i I=P0(G6,ie);int Q7=(P7&5)+((P7>>1)&5);Q7=(Q7&3)+(Q7>>2);I*=1./float(Q7);return I;}}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive