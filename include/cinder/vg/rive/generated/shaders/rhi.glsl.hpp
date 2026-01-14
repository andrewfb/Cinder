#pragma once

#include "rhi.glsl.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char rhi[] = R"===(#pragma warning(disable:3550)
#pragma warning(disable:4000)
#ifndef _ARE_TOKEN_NAMES_PRESERVED
#define d half
#define G half2
#define r half3
#define i half4
#define V ushort
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
#define V ushort
#define d0 float2x2
#define D6 half3x3
#define E6 half2x3
#define o4 half4x4
#endif
typedef X R3;
#ifdef DE
#if Df
typedef min16uint V;
#endif
#else
#if Df
typedef uint V;
#endif
#endif
#define Ic(A,F) A##F
#define e inline
#define A1(Z1) out Z1
#define O4(Z1) inout Z1
#define g1(a) struct a{
#define i0(f,a0,a) a0 a:Ic(eh,f)
#define h1 };
#define j0(o8,D,a,a0) a0 a=D.a
#define M5(f,a) cbuffer a{struct{
#define G6(a) }a;}
#define D1 struct k0{
#define q0 noperspective
#define RB nointerpolation
#define e3 nointerpolation
#define N(f,a0,a) a0 a:Ic(TEXCOORD,f)
#ifdef KE
#define z1 g U0:SV_Position;g Ef:SV_ClipDistance;};
#else
#define z1 g U0:SV_Position;};
#endif
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
#define l4(M,f,a) uniform Texture2D<P>a
#define T4(M,f,a) uniform Texture2D<g>a
#define z2(M,f,a) uniform Texture2D<i>a
#define W4(M,f,a) uniform Texture2D<d>a
#define Q5(M,f,a) uniform Texture2DArray<d>a
#define g5(f,a) SamplerState a;
#define w4 g5
#define A3(M,f,a) g5(f,a)
#define G1(a,k) a[k]
#define d5(a,n,k) a.Sample(n,k)
#define d2(a,n,k,Q0) a.SampleLevel(n,k,Q0)
#define e5(a,n,k,K1) a.SampleBias(n,k,K1)
#define x4(a,n,k,q3) a.Gather(n,(k)*(q3))
#define B6(a,n,o,Y5,v8,Q0) a.SampleLevel(n,X(o,0.5,Y5),Q0)
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
#if defined(GB)&&defined(CB)
#ifdef HF
#define Q6(a) [[ih::input_attachment_index(T3)]]SubpassInputMS<i>a
#define x8(a) zb(o4(a.na(0),a.na(1),a.na(2),a.na(3)),Ff)
#else
#define Q6(a) Texture2D a
#define x8(a) a[H]
#endif
#endif
#define v2
#define w2
#ifdef LC
#define F0(f,a) uniform D2<jh i>a
#else
#define F0(f,a) uniform D2<uint>a
#endif
#define U3 F0
#define Z0(f,a) uniform D2<uint>a
#define x3 Y0
#define y3 d1
#if COMPILER_METAL
#define W3(f,a) uniform RWBuffer<uint>a
#define x3(h) h[f1]
#define y3(h,E) h[f1]=E
#else
#define W3 Z0
#define x3 Y0
#define y3 d1
#endif
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
#if COMPILER_METAL
e uint h5(RWBuffer<uint>a3,uint f1,uint x){uint V0;InterlockedMax(a3[f1],x,V0);return V0;}
#define A5(h,o) h5(h,f1,o)
e uint i5(RWBuffer<uint>a3,uint f1,uint x){uint V0;InterlockedAdd(a3[f1],x,V0);return V0;}
#define C5(h,o) i5(h,f1,o)
#else
e uint h5(D2<uint>a3,Y H,uint x){uint V0;InterlockedMax(a3[H],x,V0);return V0;}
#define A5(h,o) h5(h,H,o)
e uint i5(D2<uint>a3,Y H,uint x){uint V0;InterlockedAdd(a3[H],x,V0);return V0;}
#define C5(h,o) i5(h,H,o)
#endif
#define h2(h)
#define Q1(h)
#define U5
#define o2
#define p3
#define P0
#ifdef LE
#define p1(a,c0,D,p,O) uint baseInstance;g a(c0 D,uint p:SV_VertexID,uint f7:SV_InstanceID):SV_Position{uint O=f7+baseInstance;
#define l1(j5) return j5;}
#else
#define p1(a,c0,D,p,O) uint baseInstance;k0 a(c0 D,uint p:SV_VertexID,uint f7:SV_InstanceID){uint O=f7+baseInstance;k0 U;
#define r7(a,c0,D,p,O) k0 a(c0 D,uint p:SV_VertexID){k0 U;g U0;
#define w5(a,q2,r2,J2,K2,p) k0 a(q2 r2,J2 K2,uint p:SV_VertexID){k0 U;g U0;
#define l1(j5) U.U0=j5;}return U;
#endif
#ifdef LE
#define U1(L3,a) EARLYDEPTHSTENCIL L3 a(g U0:SV_Position):SV_Target{c v0=U0.xy;
#else
#define U1(L3,a) EARLYDEPTHSTENCIL L3 a(k0 U,uint Ff:SV_Coverage):SV_Target{c v0=U.U0.xy;Y H=Y(floor(v0));uint f1=H.y*q.F6+H.x;
#endif
#define V1(E) return E;}
#ifdef KE
#define I6 ,out g gl_ClipDistance
#define f5 ,U.Ef
#else
#define I6
#define f5
#endif
#define i6 ,c v0
#define U2 ,v0
#define a4 ,Y H
#define P1 ,H
#define F1(a) EARLYDEPTHSTENCIL void a(k0 U){c v0=U.U0.xy;Y H=Y(floor(v0));uint f1=H.y*q.F6+H.x;
#define y5(a) F1(a)
#if defined(FB)&&defined(MB)
#define x2 S4
#else
#define x2 }
#endif
#define M2(a) EARLYDEPTHSTENCIL i a(k0 U):SV_Target{c v0=U.U0.xy;Y H=Y(floor(v0));uint f1=H.y*q.F6+H.x;i r1;
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
#define M4(f,x1,a) StructuredBuffer<a1>a
#define v4(f,x1,a) StructuredBuffer<P>a
#define N4(f,x1,a) StructuredBuffer<g>a
#define A0(a,w0) a[w0]
#define Q4(a,w0) a[w0]
e G unpackHalf2x16(uint u){uint y=(u>>16);uint x=u&0xffffu;return G(f16tof32(x),f16tof32(y));}e uint packHalf2x16(c W1){uint x=f32tof16(W1.x);uint y=f32tof16(W1.y);return(y<<16)|x;}e i unpackUnorm4x8(uint u){P L1=P(u&0xffu,(u>>8)&0xffu,(u>>16)&0xffu,u>>24);return i(L1)*(1./255.);}e uint packUnorm4x8(i j){P L1=(P(j*255.)&0xff)<<P(0,8,16,24);L1.xy|=L1.zw;L1.x|=L1.y;return L1.x;}e d0 inverse(d0 n1){d0 ka=d0(n1[1][1],-n1[0][1],-n1[1][0],n1[0][0]);return ka*(1./determinant(n1));}e float mix(float x,float y,float s){return lerp(x,y,s);}e c mix(c x,c y,c s){return lerp(x,y,s);}e X mix(X x,X y,X s){return lerp(x,y,s);}e g mix(g x,g y,g s){return lerp(x,y,s);}e float fract(float x){return frac(x);}e c fract(c x){return frac(x);}e X fract(X x){return frac(x);}e g fract(g x){return frac(x);}e float mod(float x,float y){return fmod(x,y);}e float E2(float x){return sign(x);}e c E2(c x){return sign(x);}e X E2(X x){return sign(x);}e g E2(g x){return sign(x);}
#define sign E2
e float F2(float x){return abs(x);}e c F2(c x){return abs(x);}e X F2(X x){return abs(x);}e g F2(g x){return abs(x);}
#define abs F2
e float G2(float x){return sqrt(x);}e c G2(c x){return sqrt(x);}e X G2(X x){return sqrt(x);}e g G2(g x){return sqrt(x);}
#define sqrt G2
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive