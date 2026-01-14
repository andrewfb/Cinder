#pragma once

#include "hlsl.glsl.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char hlsl[] = R"===(#pragma warning(disable:3550)
#pragma warning(disable:4000)
#ifndef _ARE_TOKEN_NAMES_PRESERVED
#define d half
#define G half2
#define r half3
#define i half4
#define c float2
#define X float3
#define g float4
#define n4 bool2
#define X5 bool3
#define a7 bool4
#define a1 uint2
#define P uint4
#define Y int2
#define p4 int4
#define d0 float2x2
#define D6 half3x3
#define E6 half2x3
#define o4 half4x4
#endif
typedef X R3;
#ifdef DE
#define V min16uint
#else
#define V uint
#endif
#define e inline
#define A1(Z1) out Z1
#define O4(Z1) inout Z1
#define g1(a) struct a{
#define i0(f,a0,a) a0 a:a
#define h1 };
#define j0(o8,D,a,a0) a0 a=D.a
#define Cc(f) register(b##f)
#define M5(f,a) cbuffer a:Cc(f){struct{
#define G6(a) }a;}
#define D1 struct k0{
#define q0 noperspective
#define RB nointerpolation
#define e3 nointerpolation
#define N(f,a0,a) a0 a:TEXCOORD##f
#define z1 g U0:SV_Position;};
#define L(a,a0) a0 a
#define W(a) U.a=a
#define I(a,a0) a0 a=U.a
#ifdef BB
#define j3
#define k3
#endif
#ifdef GB
#define W2
#define X2
#endif
#define i4
#define j4
#define l4(M,f,a) uniform Texture2D<P>a:register(t##f)
#define T4(M,f,a) uniform Texture2D<g>a:register(t##f)
#define z2(M,f,a) uniform Texture2D<unorm g>a:register(t##f)
#define W4(M,f,a) uniform Texture2D<d>a:register(t##f)
#define Q5(M,f,a) uniform Texture1DArray<d>a:register(t##f)
#define g5(f,a) SamplerState a:register(s##f);
#define w4 g5
#define A3(M,f,a) g5(f,a)
#define G1(a,k) a[k]
#define d5(a,n,k) a.Sample(n,k)
#define d2(a,n,k,Q0) a.SampleLevel(n,k,Q0)
#define e5(a,n,k,K1) a.SampleBias(n,k,K1)
#define x4(a,n,k,q3) a.Gather(n,(k)*(q3))
#define B6(a,n,o,Y5,v8,Q0) a.SampleLevel(n,c(o,Y5),Q0)
#define l6(g0,n,k) d5(g0,n,k)
#define r8(g0,n,k,Q0) d2(g0,n,k,Q0)
#define c7(g0,n,k,K1) e5(g0,n,k,K1)
#define i2
#define j2
#ifdef EE
#define D2 RasterizerOrderedTexture2D
#else
#define D2 RWTexture2D
#endif
#define v2
#ifdef LC
#define F0(f,a) uniform D2<unorm i>a:register(u##f)
#else
#define F0(f,a) uniform D2<uint>a:register(u##f)
#endif
#define U3 F0
#define Z0(f,a) uniform D2<uint>a:register(u##f)
#define W3 Z0
#define x3 Y0
#define y3 d1
#define w2
#ifdef LC
#define X0(h) h[H]
#else
#define X0(h) unpackUnorm4x8(h[H])
#endif
#define Y0(h) h[H]
#ifdef LC
#define c1(h,E) h[H]=(E)
#else
#define c1(h,E) h[H]=packUnorm4x8(E)
#endif
#define d1(h,E) h[H]=(E)
e uint h5(D2<uint>a3,Y H,uint x){uint V0;InterlockedMax(a3[H],x,V0);return V0;}
#define A5(h,o) h5(h,H,o)
e uint i5(D2<uint>a3,Y H,uint x){uint V0;InterlockedAdd(a3[H],x,V0);return V0;}
#define C5(h,o) i5(h,H,o)
#define h2(h)
#define Q1(h)
#define U5
#define o2
#define p3
#define P0
#define I6
#define f5
#define p1(a,c0,D,p,O) cbuffer Yg:Cc(Kb){uint vf;uint a##Zg;uint a##ah;uint a##bh;}k0 main(c0 D,uint p:SV_VertexID,uint f7:SV_InstanceID){uint O=f7+vf;k0 U;
#define r7(a,c0,D,p,O) k0 main(c0 D,uint p:SV_VertexID){k0 U;g U0;
#define w5(a,q2,r2,J2,K2,p) k0 main(q2 r2,J2 K2,uint p:SV_VertexID){k0 U;g U0;
#define l1(j5) U.U0=j5;}return U;
#define U1(L3,a) L3 main(k0 U):SV_Target{
#define V1(E) return E;}
#define i6 ,c v0
#define U2 ,v0
#define a4 ,Y H
#define P1 ,H
#define F1(a) [earlydepthstencil]void main(k0 U){c v0=U.U0.xy;Y H=Y(floor(v0));
#define y5(a) F1(a)
#define x2 }
#define M2(a) [earlydepthstencil]i main(k0 U):SV_Target{c v0=U.U0.xy;Y H=Y(floor(v0));i r1;
#define c4(a) M2(a)
#define S4 }return r1;
#define uintBitsToFloat asfloat
#define floatBitsToInt asint
#define floatBitsToUint asuint
#define inversesqrt rsqrt
#define equal(A,F) ((A)==(F))
#define notEqual(A,F) ((A)!=(F))
#define lessThan(A,F) ((A)<(F))
#define greaterThan(A,F) ((A)>(F))
#define H0(A,F) mul(F,A)
#define g4
#define h4
#define Y3
#define Z3
#define M4(f,x1,a) StructuredBuffer<a1>a:register(t##f)
#define v4(f,x1,a) StructuredBuffer<P>a:register(t##f)
#define N4(f,x1,a) StructuredBuffer<g>a:register(t##f)
#define A0(a,w0) a[w0]
#define Q4(a,w0) a[w0]
e G unpackHalf2x16(uint u){uint y=(u>>16);uint x=u&0xffffu;return G(f16tof32(x),f16tof32(y));}e uint packHalf2x16(c W1){uint x=f32tof16(W1.x);uint y=f32tof16(W1.y);return(y<<16)|x;}e i unpackUnorm4x8(uint u){P L1=P(u&0xffu,(u>>8)&0xffu,(u>>16)&0xffu,u>>24);return i(L1)*(1./255.);}e uint packUnorm4x8(i j){P L1=(P(j*255.)&0xff)<<P(0,8,16,24);L1.xy|=L1.zw;L1.x|=L1.y;return L1.x;}e d0 inverse(d0 n1){d0 ka=d0(n1[1][1],-n1[0][1],-n1[1][0],n1[0][0]);return ka*(1./determinant(n1));}e float mix(float x,float y,bool s){return s?y:x;}e c mix(c x,c y,n4 s){return s?y:x;}e X mix(X x,X y,X5 s){return s?y:x;}e g mix(g x,g y,a7 s){return s?y:x;}e d mix(d x,d y,bool s){return s?y:x;}e G mix(G x,G y,n4 s){return s?y:x;}e r mix(r x,r y,X5 s){return s?y:x;}e i mix(i x,i y,a7 s){return s?y:x;}e float mix(float x,float y,float s){return lerp(x,y,s);}e c mix(c x,c y,c s){return lerp(x,y,s);}e X mix(X x,X y,X s){return lerp(x,y,s);}e g mix(g x,g y,g s){return lerp(x,y,s);}e d mix(d x,d y,d s){return lerp(x,y,s);}e G mix(G x,G y,G s){return lerp(x,y,s);}e r mix(r x,r y,r s){return lerp(x,y,s);}e i mix(i x,i y,i s){return lerp(x,y,s);}e float fract(float x){return frac(x);}e c fract(c x){return frac(x);}e X fract(X x){return frac(x);}e g fract(g x){return frac(x);}e d fract(d x){return frac(x);}e G fract(G x){return G(frac(x));}e r fract(r x){return r(frac(x));}e i fract(i x){return i(frac(x));}e float mod(float x,float y){return fmod(x,y);}e d E2(d x){return sign(x);}e G E2(G x){return G(sign(x));}e r E2(r x){return r(sign(x));}e i E2(i x){return i(sign(x));}e float E2(float x){return sign(x);}e c E2(c x){return sign(x);}e X E2(X x){return sign(x);}e g E2(g x){return sign(x);}
#define sign E2
e d F2(d x){return abs(x);}e G F2(G x){return G(abs(x));}e r F2(r x){return r(abs(x));}e i F2(i x){return i(abs(x));}e float F2(float x){return abs(x);}e c F2(c x){return abs(x);}e X F2(X x){return abs(x);}e g F2(g x){return abs(x);}
#define abs F2
e d G2(d x){return sqrt(x);}e G G2(G x){return G(sqrt(x));}e r G2(r x){return r(sqrt(x));}e i G2(i x){return i(sqrt(x));}e float G2(float x){return sqrt(x);}e c G2(c x){return sqrt(x);}e X G2(X x){return sqrt(x);}e g G2(g x){return sqrt(x);}
#define sqrt G2
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive