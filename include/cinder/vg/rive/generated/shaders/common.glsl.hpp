#pragma once

#include "common.glsl.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char common[] = R"===(#define i3 3.14159265359
#define Q7 6.28318530718
#define A6 1.57079632679
#ifndef CB
#define S3 float(.5)
#else
#define S3 float(.0)
#endif
#define R2(k) P7(k,q.Vd,q.Wd)
#ifdef TE
#define kb(M,f,a) T4(M,f,a)
#define k4 g
#define x9(o) o
#define H5(o) o
#define y9(o) uintBitsToFloat(o)
#define U4(o) floatBitsToUint(o)
#else
#define kb(M,f,a) l4(M,f,a)
#define k4 P
#define x9(o) floatBitsToUint(o)
#define H5(o) uintBitsToFloat(o)
#define y9(o) o
#define U4(o) o
#endif
#define lb(a,k,R7) G1(a,Y(k)+Y(-1,0))R7,G1(a,Y(k)+Y(0,0))R7,G1(a,Y(k)+Y(0,-1))R7,G1(a,Y(k)+Y(-1,-1))R7
#define m4(o) B6(RC,z9,o,mb,float(mb),.0).x
#define I5(o) B6(RC,z9,o,nb,float(nb),.0).x
#ifdef ob
e d J4(float x){return x;}e d J5(uint x){return float(x);}e d Xd(V x){return float(x);}e d A9(int x){return float(x);}e i F5(g xyzw){return xyzw;}e G q7(c xy){return xy;}e i fb(P xyzw){return vec4(xyzw);}e V C6(d x){return uint(x);}e V R1(uint x){return x;}
#else
e d J4(float x){return(d)x;}e d J5(uint x){return(d)x;}e d Xd(V x){return(d)x;}e d A9(int x){return(d)x;}e i F5(g xyzw){return(i)xyzw;}e G q7(c xy){return(G)xy;}e i fb(P xyzw){return(i)xyzw;}e V C6(d x){return(V)x;}e V R1(uint x){return(V)x;}
#endif
e d k1(d x){return x;}e G f3(G xy){return xy;}e G f3(d x,d y){G K;K.x=x,K.y=y;return K;}e G f3(d x){G K;K.x=x,K.y=x;return K;}e c m6(float x){return c(x,x);}e r E0(d x,d y,d z){r K;K.x=x,K.y=y,K.z=z;return K;}e r E0(d x){r K;K.x=x,K.y=x,K.z=x;return K;}e i T0(d x,d y,d z,d w){i K;K.x=x,K.y=y,K.z=z,K.w=w;return K;}e i T0(r xyz,d w){i K;K.xyz=xyz;K.w=w;return K;}e i T0(d x){i K;K.x=x,K.y=x,K.z=x,K.w=x;return K;}e i T0(i x){return x;}e n4 Yd(bool b){return n4(b,b);}e D6 tg(r l,r b,r x0){D6 K;K[0]=l;K[1]=b;K[2]=x0;return K;}e E6 ug(r l,r b){E6 K;K[0]=l;K[1]=b;return K;}e o4 Zd(i l,i b,i x0,i ae){o4 K;K[0]=l;K[1]=b;K[2]=x0;K[3]=ae;return K;}e d0 N1(g x){return d0(x.xy,x.zw);}e uint Sa(V x){return x;}e c K5(c l,c b,float t){return(b-l)*t+l;}e d S7(uint pb,uint L5){return pb==0u?.0:unpackHalf2x16((pb+be)*L5).x;}e float qb(c W1){W1=normalize(W1);float e1=acos(clamp(W1.x,-1.,1.));return W1.y>=.0?e1:-e1;}e i vg(i j){return T0(j.xyz*j.w,j.w);}e r H4(i B9){return B9.xyz*(B9.w!=.0?1./B9.w:.0);}e d J7(i rb){G sb=min(rb.xy,rb.zw);d ce=min(sb.x,sb.y);return ce;}e float V8(c x){return abs(x.x)+abs(x.y);}e d C9(d x,d D9,d E9){
#if defined(UE)||defined(SC)
#ifdef SC
if(SC==de)
#endif
{if(x<E9)if(x>D9)return x;else return D9;else return E9;}
#endif
return clamp(x,D9,E9);}
#ifndef UNIFORM_DEFINITIONS_AUTO_GENERATED
M5(Y2,SB)float gb;float tb;float Vd;float Wd;uint F6;uint ee;uint Id;uint Jd;p4 x7;c d4;c ub;uint B3;uint L5;float M1;float vb;uint fe;G6(q)
#endif
#ifdef BB
e g P7(c wb,float ge,float xb){return g(wb.x*ge-1.,wb.y*xb-sign(xb),0.,1.);}
#ifndef CB
e g w7(d0 c2,c p2,c F9){c G9=abs(c2[0])+abs(c2[1]);if(G9.x!=.0&&G9.y!=.0){c S=1./G9;c V4=H0(c2,F9)+p2;const float he=.5;return g(V4,-V4)*S.xyxy+S.xyxy+he;}else{return p2.xyxy;}}
#else
e float H9(uint H6){return 1.-float(H6)*(2./32768.);}
#ifdef DB
e void yb(d0 c2,c p2,c F9 I6){
#ifndef VD
if(any(notEqual(g(c2),g(.0,.0,.0,.0)))){c V4=H0(c2,F9)+p2.xy;gl_ClipDistance[0]=V4.x+1.;gl_ClipDistance[1]=V4.y+1.;gl_ClipDistance[2]=1.-V4.x;gl_ClipDistance[3]=1.-V4.y;}else{gl_ClipDistance[0]=gl_ClipDistance[1]=gl_ClipDistance[2]=gl_ClipDistance[3]=p2.x-.5;}
#endif
}
#endif
#endif
#endif
#ifdef GB
#ifdef CC
e d h3(d j){return(j<=0.04045)?j/12.92:pow(abs((j+0.055)/1.055),2.4);}e r h3(r j){return E0(h3(j.x),h3(j.y),h3(j.z));}e i h3(i j){return T0(h3(j.xyz),j.w);}
#endif
#endif
#ifdef CD
#ifndef UNIFORM_DEFINITIONS_AUTO_GENERATED
M5(N5,KC)g v7;c j1;float V2;float wg;g c2;c p2;uint N0;uint e2;uint H6;G6(l0)
#endif
#endif
#if defined(GB)&&defined(CB)&&!defined(FB)
e i zb(o4 J6,int T7){if(T7==0xf){return(J6[0]+J6[1]+J6[2]+J6[3])*.25;}else{i ie=g(notEqual(T7&p4(1,2,4,8),p4(0)));i K=H0(J6,ie);int U7=(T7&5)+((T7>>1)&5);U7=(U7&3)+(U7>>2);K*=1./float(U7);return K;}}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive