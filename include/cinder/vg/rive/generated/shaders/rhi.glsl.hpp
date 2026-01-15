#pragma once

#include "rhi.glsl.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char rhi[] = R"===(#pragma warning(disable:3550)
#pragma warning(disable:4000)
#ifndef _ARE_TOKEN_NAMES_PRESERVED
#define e half
#define E half2
#define p half3
#define i half4
#define Y ushort
#define c float2
#define U float3
#define g float4
#define c4 bool2
#define U5 bool3
#define W6 bool4
#define j1 uint2
#define M uint4
#define S int2
#define e4 int4
#define Y ushort
#define W float2x2
#define A6 half3x3
#define B6 half2x3
#define d4 half4x4
#endif
typedef U y4;
#ifdef DE
#if uf
typedef min16uint Y;
#endif
#else
#if uf
typedef uint Y;
#endif
#endif
#define Cc(r,D) r##D
#define d inline
#define q1(S1) out S1
#define H4(S1) inout S1
#define w1(a) struct a{
#define l0(f,V,a) V a:Cc(ch,f)
#define x1 };
#define n0(i8,F,a,V) V a=F.a
#define K5(f,a) cbuffer a{struct{
#define D6(a) }a;}
#define V1 struct h0{
#define B0 noperspective
#define OB nointerpolation
#define x4 nointerpolation
#define X(f,V,a) V a:Cc(TEXCOORD,f)
#ifdef KE
#define N1 g Q0:SV_Position;g vf:SV_ClipDistance;};
#else
#define N1 g Q0:SV_Position;};
#endif
#define T(a,V) V a
#define g0(a) R.a=a
#define H(a,V) V a=R.a
#ifdef BB
#define n3
#define o3
#endif
#ifdef GB
#define c3
#define d3
#endif
#define N4
#define O4
#define Z3(K,f,a) uniform Texture2D<M>a
#define P4(K,f,a) uniform Texture2D<g>a
#define K2(K,f,a) uniform Texture2D<i>a
#define S4(K,f,a) uniform Texture2D<e>a
#define N5(K,f,a) uniform Texture2DArray<e>a
#define c5(f,a) SamplerState a;
#define h4 c5
#define X3(K,f,a) c5(f,a)
#define D1(a,k) a[k]
#define Y4(a,n,k) a.Sample(n,k)
#define F2(a,n,k,M0) a.SampleLevel(n,k,M0)
#define Z4(a,n,k,G1) a.SampleBias(n,k,G1)
#define i4(a,n,k,g3) a.Gather(n,(k)*(g3))
#define y6(a,n,o,V5,m8,M0) a.SampleLevel(n,U(o,0.5,V5),M0)
#define C7(d0,n,k) Y4(d0,n,k)
#define l8(d0,n,k,M0) F2(d0,n,k,M0)
#define X6(d0,n,k,G1) Z4(d0,n,k,G1)
#define g2
#define h2
#ifdef EE
#define x2 RasterizerOrderedTexture2D
#else
#define x2 RWTexture2D
#endif
#if defined(GB)&&defined(AB)
#ifdef IF
#define M6(a) [[gh::input_attachment_index(J3)]]SubpassInputMS<i>a
#define o8(a) yb(d4(a.da(0),a.da(1),a.da(2),a.da(3)),wf)
#else
#define M6(a) Texture2D a
#define o8(a) a[G]
#endif
#endif
#define n2
#define o2
#ifdef KC
#define C0(f,a) uniform x2<hh i>a
#else
#define C0(f,a) uniform x2<uint>a
#endif
#define K3 C0
#define U0(f,a) uniform x2<uint>a
#define l3 T0
#define m3 W0
#if COMPILER_METAL
#define M3(f,a) uniform RWBuffer<uint>a
#define l3(h) h[Y0]
#define m3(h,C) h[Y0]=C
#else
#define M3 U0
#define l3 T0
#define m3 W0
#endif
#ifdef KC
#define S0(h) h[G]
#else
#define S0(h) unpackUnorm4x8(h[G])
#endif
#define T0(h) h[G]
#ifdef KC
#define V0(h,C) h[G]=(C)
#else
#define V0(h,C) h[G]=packUnorm4x8(C)
#endif
#define W0(h,C) h[G]=(C)
#if COMPILER_METAL
d uint d5(RWBuffer<uint>S2,uint Y0,uint x){uint R0;InterlockedMax(S2[Y0],x,R0);return R0;}
#define x5(h,o) d5(h,Y0,o)
d uint e5(RWBuffer<uint>S2,uint Y0,uint x){uint R0;InterlockedAdd(S2[Y0],x,R0);return R0;}
#define z5(h,o) e5(h,Y0,o)
#else
d uint d5(x2<uint>S2,S G,uint x){uint R0;InterlockedMax(S2[G],x,R0);return R0;}
#define x5(h,o) d5(h,G,o)
d uint e5(x2<uint>S2,S G,uint x){uint R0;InterlockedAdd(S2[G],x,R0);return R0;}
#define z5(h,o) e5(h,G,o)
#endif
#define f2(h)
#define L1(h)
#define R5
#define U2
#define f3
#define f1
#ifdef LE
#define y1(a,c0,F,B,O) uint baseInstance;g a(c0 F,uint B:SV_VertexID,uint a7:SV_InstanceID):SV_Position{uint O=a7+baseInstance;
#define z1(f5) return f5;}
#else
#define y1(a,c0,F,B,O) uint baseInstance;h0 a(c0 F,uint B:SV_VertexID,uint a7:SV_InstanceID){uint O=a7+baseInstance;h0 R;
#define o7(a,c0,F,B,O) h0 a(c0 F,uint B:SV_VertexID){h0 R;g Q0;
#define g6(a,O2,P2,V2,W2,B) h0 a(O2 P2,V2 W2,uint B:SV_VertexID){h0 R;g Q0;
#define z1(f5) R.Q0=f5;}return R;
#endif
#ifdef LE
#define d2(z3,a) EARLYDEPTHSTENCIL z3 a(g Q0:SV_Position):SV_Target{c q0=Q0.xy;
#else
#define d2(z3,a) EARLYDEPTHSTENCIL z3 a(h0 R,uint wf:SV_Coverage):SV_Target{c q0=R.Q0.xy;S G=S(floor(q0));uint Y0=G.y*A.C6+G.x;
#endif
#define e2(C) return C;}
#ifdef KE
#define F6 ,out g gl_ClipDistance
#define a5 ,R.vf
#else
#define F6
#define a5
#endif
#define i6 ,c q0
#define H2 ,q0
#define O3 ,S G
#define K1 ,G
#define C1(a) EARLYDEPTHSTENCIL void a(h0 R){c q0=R.Q0.xy;S G=S(floor(q0));uint Y0=G.y*A.C6+G.x;
#define v5(a) C1(a)
#if defined(EB)&&defined(KB)
#define p2 L4
#else
#define p2 }
#endif
#define G2(a) EARLYDEPTHSTENCIL i a(h0 R):SV_Target{c q0=R.Q0.xy;S G=S(floor(q0));uint Y0=G.y*A.C6+G.x;i l1;
#define P3(a) G2(a)
#define L4 }return l1;
#define uintBitsToFloat asfloat
#define floatBitsToInt asint
#define floatBitsToUint asuint
#define inversesqrt rsqrt
#define equal(r,D) ((r)==(D))
#define notEqual(r,D) ((r)!=(D))
#define lessThan(r,D) ((r)<(D))
#define greaterThan(r,D) ((r)>(D))
#define P0(r,D) mul(D,r)
#define U3
#define V3
#define D4
#define G4
#define E4(f,o1,a) StructuredBuffer<j1>a
#define g4(f,o1,a) StructuredBuffer<M>a
#define F4(f,o1,a) StructuredBuffer<g>a
#define E0(a,r0) a[r0]
#define q5(a,r0) a[r0]
d E unpackHalf2x16(uint u){uint y=(u>>16);uint x=u&0xffffu;return E(f16tof32(x),f16tof32(y));}d uint packHalf2x16(c Q1){uint x=f32tof16(Q1.x);uint y=f32tof16(Q1.y);return(y<<16)|x;}d i unpackUnorm4x8(uint u){M H1=M(u&0xffu,(u>>8)&0xffu,(u>>16)&0xffu,u>>24);return i(H1)*(1./255.);}d uint packUnorm4x8(i j){M H1=(M(j*255.)&0xff)<<M(0,8,16,24);H1.xy|=H1.zw;H1.x|=H1.y;return H1.x;}d W inverse(W h1){W aa=W(h1[1][1],-h1[0][1],-h1[1][0],h1[0][0]);return aa*(1./determinant(h1));}d float mix(float x,float y,float s){return lerp(x,y,s);}d c mix(c x,c y,c s){return lerp(x,y,s);}d U mix(U x,U y,U s){return lerp(x,y,s);}d g mix(g x,g y,g s){return lerp(x,y,s);}d float fract(float x){return frac(x);}d c fract(c x){return frac(x);}d U fract(U x){return frac(x);}d g fract(g x){return frac(x);}d float mod(float x,float y){return fmod(x,y);}d float y2(float x){return sign(x);}d c y2(c x){return sign(x);}d U y2(U x){return sign(x);}d g y2(g x){return sign(x);}
#define sign y2
d float z2(float x){return abs(x);}d c z2(c x){return abs(x);}d U z2(U x){return abs(x);}d g z2(g x){return abs(x);}
#define abs z2
d float A2(float x){return sqrt(x);}d c A2(c x){return sqrt(x);}d U A2(U x){return sqrt(x);}d g A2(g x){return sqrt(x);}
#define sqrt A2
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive