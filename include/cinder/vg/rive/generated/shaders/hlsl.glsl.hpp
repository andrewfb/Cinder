#pragma once

#include "hlsl.glsl.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char hlsl[] = R"===(#pragma warning(disable:3550)
#pragma warning(disable:4000)
#ifndef _ARE_TOKEN_NAMES_PRESERVED
#define e half
#define E half2
#define p half3
#define i half4
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
#define W float2x2
#define A6 half3x3
#define B6 half2x3
#define d4 half4x4
#endif
typedef U y4;
#ifdef DE
#define Y min16uint
#else
#define Y uint
#endif
#define d inline
#define q1(S1) out S1
#define H4(S1) inout S1
#define w1(a) struct a{
#define l0(f,V,a) V a:a
#define x1 };
#define n0(i8,F,a,V) V a=F.a
#define wc(f) register(b##f)
#define K5(f,a) cbuffer a:wc(f){struct{
#define D6(a) }a;}
#define V1 struct h0{
#define B0 noperspective
#define OB nointerpolation
#define x4 nointerpolation
#define X(f,V,a) V a:TEXCOORD##f
#define N1 g Q0:SV_Position;};
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
#define Z3(K,f,a) uniform Texture2D<M>a:register(t##f)
#define P4(K,f,a) uniform Texture2D<g>a:register(t##f)
#define K2(K,f,a) uniform Texture2D<unorm g>a:register(t##f)
#define S4(K,f,a) uniform Texture2D<e>a:register(t##f)
#define N5(K,f,a) uniform Texture1DArray<e>a:register(t##f)
#define c5(f,a) SamplerState a:register(s##f);
#define h4 c5
#define X3(K,f,a) c5(f,a)
#define D1(a,k) a[k]
#define Y4(a,n,k) a.Sample(n,k)
#define F2(a,n,k,M0) a.SampleLevel(n,k,M0)
#define Z4(a,n,k,G1) a.SampleBias(n,k,G1)
#define i4(a,n,k,g3) a.Gather(n,(k)*(g3))
#define y6(a,n,o,V5,m8,M0) a.SampleLevel(n,c(o,V5),M0)
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
#define n2
#ifdef KC
#define C0(f,a) uniform x2<unorm i>a:register(u##f)
#else
#define C0(f,a) uniform x2<uint>a:register(u##f)
#endif
#define K3 C0
#define U0(f,a) uniform x2<uint>a:register(u##f)
#define M3 U0
#define l3 T0
#define m3 W0
#define o2
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
d uint d5(x2<uint>S2,S G,uint x){uint R0;InterlockedMax(S2[G],x,R0);return R0;}
#define x5(h,o) d5(h,G,o)
d uint e5(x2<uint>S2,S G,uint x){uint R0;InterlockedAdd(S2[G],x,R0);return R0;}
#define z5(h,o) e5(h,G,o)
#define f2(h)
#define L1(h)
#define R5
#define U2
#define f3
#define f1
#define F6
#define a5
#define y1(a,c0,F,B,O) cbuffer Wg:wc(Jb){uint mf;uint a##Xg;uint a##Yg;uint a##Zg;}h0 main(c0 F,uint B:SV_VertexID,uint a7:SV_InstanceID){uint O=a7+mf;h0 R;
#define o7(a,c0,F,B,O) h0 main(c0 F,uint B:SV_VertexID){h0 R;g Q0;
#define g6(a,O2,P2,V2,W2,B) h0 main(O2 P2,V2 W2,uint B:SV_VertexID){h0 R;g Q0;
#define z1(f5) R.Q0=f5;}return R;
#define d2(z3,a) z3 main(h0 R):SV_Target{
#define e2(C) return C;}
#define i6 ,c q0
#define H2 ,q0
#define O3 ,S G
#define K1 ,G
#define C1(a) [earlydepthstencil]void main(h0 R){c q0=R.Q0.xy;S G=S(floor(q0));
#define v5(a) C1(a)
#define p2 }
#define G2(a) [earlydepthstencil]i main(h0 R):SV_Target{c q0=R.Q0.xy;S G=S(floor(q0));i l1;
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
#define E4(f,o1,a) StructuredBuffer<j1>a:register(t##f)
#define g4(f,o1,a) StructuredBuffer<M>a:register(t##f)
#define F4(f,o1,a) StructuredBuffer<g>a:register(t##f)
#define E0(a,r0) a[r0]
#define q5(a,r0) a[r0]
d E unpackHalf2x16(uint u){uint y=(u>>16);uint x=u&0xffffu;return E(f16tof32(x),f16tof32(y));}d uint packHalf2x16(c Q1){uint x=f32tof16(Q1.x);uint y=f32tof16(Q1.y);return(y<<16)|x;}d i unpackUnorm4x8(uint u){M H1=M(u&0xffu,(u>>8)&0xffu,(u>>16)&0xffu,u>>24);return i(H1)*(1./255.);}d uint packUnorm4x8(i j){M H1=(M(j*255.)&0xff)<<M(0,8,16,24);H1.xy|=H1.zw;H1.x|=H1.y;return H1.x;}d W inverse(W h1){W aa=W(h1[1][1],-h1[0][1],-h1[1][0],h1[0][0]);return aa*(1./determinant(h1));}d float mix(float x,float y,bool s){return s?y:x;}d c mix(c x,c y,c4 s){return s?y:x;}d U mix(U x,U y,U5 s){return s?y:x;}d g mix(g x,g y,W6 s){return s?y:x;}d e mix(e x,e y,bool s){return s?y:x;}d E mix(E x,E y,c4 s){return s?y:x;}d p mix(p x,p y,U5 s){return s?y:x;}d i mix(i x,i y,W6 s){return s?y:x;}d float mix(float x,float y,float s){return lerp(x,y,s);}d c mix(c x,c y,c s){return lerp(x,y,s);}d U mix(U x,U y,U s){return lerp(x,y,s);}d g mix(g x,g y,g s){return lerp(x,y,s);}d e mix(e x,e y,e s){return lerp(x,y,s);}d E mix(E x,E y,E s){return lerp(x,y,s);}d p mix(p x,p y,p s){return lerp(x,y,s);}d i mix(i x,i y,i s){return lerp(x,y,s);}d float fract(float x){return frac(x);}d c fract(c x){return frac(x);}d U fract(U x){return frac(x);}d g fract(g x){return frac(x);}d e fract(e x){return frac(x);}d E fract(E x){return E(frac(x));}d p fract(p x){return p(frac(x));}d i fract(i x){return i(frac(x));}d float mod(float x,float y){return fmod(x,y);}d e y2(e x){return sign(x);}d E y2(E x){return E(sign(x));}d p y2(p x){return p(sign(x));}d i y2(i x){return i(sign(x));}d float y2(float x){return sign(x);}d c y2(c x){return sign(x);}d U y2(U x){return sign(x);}d g y2(g x){return sign(x);}
#define sign y2
d e z2(e x){return abs(x);}d E z2(E x){return E(abs(x));}d p z2(p x){return p(abs(x));}d i z2(i x){return i(abs(x));}d float z2(float x){return abs(x);}d c z2(c x){return abs(x);}d U z2(U x){return abs(x);}d g z2(g x){return abs(x);}
#define abs z2
d e A2(e x){return sqrt(x);}d E A2(E x){return E(sqrt(x));}d p A2(p x){return p(sqrt(x));}d i A2(i x){return i(sqrt(x));}d float A2(float x){return sqrt(x);}d c A2(c x){return sqrt(x);}d U A2(U x){return sqrt(x);}d g A2(g x){return sqrt(x);}
#define sqrt A2
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive