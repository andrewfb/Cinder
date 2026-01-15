#pragma once

#include "metal.glsl.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char metal[] = R"===(#ifndef _ARE_TOKEN_NAMES_PRESERVED
#define e half
#define E half2
#define p half3
#define i half4
#define Y ushort
#define c float2
#define U float3
#define y4 packed_float3
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
#define d inline
#define q1(S1) thread S1&
#define H4(S1) thread S1&
#define equal(r,D) ((r)==(D))
#define notEqual(r,D) ((r)!=(D))
#define lessThan(r,D) ((r)<(D))
#define greaterThan(r,D) ((r)>(D))
#define P0(r,D) ((r)*(D))
#define inversesqrt rsqrt
#define K5(f,a) struct a{
#define D6(a) };
#define w1(a) struct a{
#define l0(f,V,a) V a
#define x1 };
#define n0(i8,F,a,V) V a=F[i8].a
#define V1 struct h0{
#define X(f,V,a) V a
#define x4 [[flat]]
#define B0 [[center_no_perspective]]
#ifndef OB
#define OB
#endif
#define N1 g Q0[[position]][[invariant]];};
#define T(a,V) thread V&a=R.a
#define g0(a)
#define H(a,V) V a=R.a
#define U3 struct p8{
#define V3 };
#define D4 struct g5{
#define G4 };
#define E4(f,o1,a) constant j1*a[[buffer(F0(f))]]
#define g4(f,o1,a) constant M*a[[buffer(F0(f))]]
#define F4(f,o1,a) constant g*a[[buffer(F0(f))]]
#define E0(a,r0) i2.a[r0]
#define q5(a,r0) i2.a[r0]
#define n3 struct q8{
#define o3 };
#define c3 struct A3{
#define d3 };
#define N4 struct c7{
#define O4 };
#define Z3(K,f,a) [[texture(f)]]texture2d<uint>a
#define P4(K,f,a) [[texture(f)]]texture2d<float>a
#define K2(K,f,a) [[texture(f)]]texture2d<e>a
#define S4(K,f,a) [[texture(f)]]texture2d<e>a
#define N5(K,f,a) [[texture(f)]]texture1d_array<e>a
#define h4(X4,a) constexpr sampler a(filter::linear,mip_filter::none);
#define X3(K,f,a) [[sampler(f)]]sampler a;
#define D1(d0,k) G0.d0.read(j1(k))
#define Y4(d0,n,k) G0.d0.sample(n,k)
#define F2(d0,n,k,M0) G0.d0.sample(n,k,level(M0))
#define Z4(d0,n,k,G1) G0.d0.sample(n,k,bias(G1))
#define i4(d0,n,k,g3) G0.d0.gather(n,(k)*(g3))
#define C7(d0,n,k) G0.d0.sample(o4.n,k)
#define l8(d0,n,k,M0) G0.d0.sample(o4.n,k,level(M0))
#define X6(d0,n,k,G1) G0.d0.sample(o4.n,k,bias(G1))
#define y6(d0,n,o,V5,m8,M0) G0.d0.sample(n,o,V5)
#define R5 ,constant NB&A,q8 G0,p8 i2
#define U2 ,A,G0,i2
#ifdef CE
#define y1(a,c0,F,B,O) __attribute__((visibility("default")))h0 vertex a(uint B[[vertex_id]],uint O[[instance_id]],constant uint&nf[[buffer(F0(Jb))]],constant NB&A[[buffer(F0(R2))]],constant c0*F[[buffer(0)]],q8 G0,p8 i2){O+=nf;h0 R;
#else
#define y1(a,c0,F,B,O) __attribute__((visibility("default")))h0 vertex a(uint B[[vertex_id]],uint O[[instance_id]],constant NB&A[[buffer(F0(R2))]],constant c0*F[[buffer(0)]],q8 G0,p8 i2){h0 R;
#endif
#define o7(a,c0,F,B,O) __attribute__((visibility("default")))h0 vertex a(uint B[[vertex_id]],constant NB&A[[buffer(F0(R2))]],constant JC&p0[[buffer(F0(L5))]],constant c0*F[[buffer(0)]],q8 G0,p8 i2){h0 R;
#define g6(a,O2,P2,V2,W2,B) __attribute__((visibility("default")))h0 vertex a(uint B[[vertex_id]],constant NB&A[[buffer(F0(R2))]],constant JC&p0[[buffer(F0(L5))]],constant O2*P2[[buffer(0)]],constant V2*W2[[buffer(1)]]){h0 R;
#define z1(f5) R.Q0=f5;}return R;
#define d2(z3,a) z3 __attribute__((visibility("default")))fragment a(h0 R[[stage_in]],A3 G0){
#define e2(C) return C;}
#define i6 ,c q0,A3 G0,g5 i2,c7 o4
#define H2 ,q0,G0,i2,o4
#define f3 ,A3 G0
#define f1 ,G0
#define F6
#define a5
#ifdef CF
#define n2 struct g1{
#ifdef DF
#define C0(f,a) device uint*a[[buffer(F0(f+M5)),raster_order_group(0)]]
#define U0(f,a) device uint*a[[buffer(F0(f+M5)),raster_order_group(0)]]
#define M3(f,a) device atomic_uint*a[[buffer(F0(f+M5)),raster_order_group(0)]]
#else
#define C0(f,a) device uint*a[[buffer(F0(f+M5))]]
#define U0(f,a) device uint*a[[buffer(F0(f+M5))]]
#define M3(f,a) device atomic_uint*a[[buffer(F0(f+M5))]]
#endif
#define o2 };
#define O3 ,g1 H0,uint Y0
#define K1 ,H0,Y0
#define S0(h) unpackUnorm4x8(H0.h[Y0])
#define T0(h) H0.h[Y0]
#define l3(h) atomic_load_explicit(&H0.h[Y0],memory_order::memory_order_relaxed)
#define V0(h,C) H0.h[Y0]=packUnorm4x8(C)
#define W0(h,C) H0.h[Y0]=(C)
#define m3(h,C) atomic_store_explicit(&H0.h[Y0],C,memory_order::memory_order_relaxed)
#define f2(h)
#define L1(h)
#define x5(h,o) atomic_fetch_max_explicit(&H0.h[Y0],o,memory_order::memory_order_relaxed)
#define z5(h,o) atomic_fetch_add_explicit(&H0.h[Y0],o,memory_order::memory_order_relaxed)
#define g2
#define h2
#define d7(a) __attribute__((visibility("default")))fragment a(g1 H0,constant NB&A[[buffer(F0(R2))]],h0 R[[stage_in]],A3 G0,c7 o4,g5 i2){c q0=R.Q0.xy;j1 G=j1(metal::floor(q0));uint Y0=G.y*A.C6+G.x;
#define xc(a) __attribute__((visibility("default")))fragment a(g1 H0,constant NB&A[[buffer(F0(R2))]],constant JC&p0[[buffer(F0(L5))]],h0 R[[stage_in]],c7 o4,A3 G0,g5 i2){c q0=R.Q0.xy;j1 G=j1(metal::floor(q0));uint Y0=G.y*A.C6+G.x;
#define C1(a) void d7(a)
#define v5(a) void xc(a)
#define p2 }
#define G2(a) i d7(a){i l1;
#define P3(a) i xc(a){i l1;
#define L4 }return l1;p2
#else
#define n2 struct g1{
#define C0(f,a) [[color(f)]]i a
#define U0(f,a) [[color(f)]]uint a
#define M3 U0
#define o2 };
#define O3 ,thread g1&B3,thread g1&H0
#define K1 ,B3,H0
#define S0(h) B3.h
#define T0(h) B3.h
#define l3(h) T0
#define V0(h,C) H0.h=(C)
#define W0(h,C) H0.h=(C)
#define m3(h) W0
#define f2(h) H0.h=B3.h
#define L1(h) H0.h=B3.h
d uint d5(thread uint&i0,uint x){uint R0=i0;i0=metal::max(R0,x);return R0;}
#define x5(h,o) d5(H0.h,o)
d uint e5(thread uint&i0,uint x){uint R0=i0;i0=R0+x;return R0;}
#define z5(h,o) e5(H0.h,o)
#define g2
#define h2
#define d7(a,...) g1 __attribute__((visibility("default")))fragment a(__VA_ARGS__){c q0[[maybe_unused]]=R.Q0.xy;g1 H0;
#define C1(a,...) d7(a,g1 B3,constant NB&A[[buffer(F0(R2))]],h0 R[[stage_in]],c7 o4,A3 G0,g5 i2)
#define v5(a) d7(a,g1 B3,constant NB&A[[buffer(F0(R2))]],h0 R[[stage_in]],A3 G0,g5 i2,c7 o4,constant JC&p0[[buffer(F0(L5))]])
#define p2 }return H0;
#define yc(a,...) struct of{i pf[[j(0)]];g1 H0;};of __attribute__((visibility("default")))fragment a(__VA_ARGS__){c q0[[maybe_unused]]=R.Q0.xy;i l1;g1 H0;
#define G2(a) yc(a,g1 B3,constant NB&A[[buffer(F0(R2))]],h0 R[[stage_in]],A3 G0,g5 i2)
#define P3(a) yc(a,g1 B3,constant NB&A[[buffer(F0(R2))]],h0 R[[stage_in]],A3 G0,g5 i2,__VA_ARGS__ constant JC&p0[[buffer(F0(L5))]])
#define L4 }return{.pf=l1,.H0=H0};
#endif
#define K3 C0
#define discard discard_fragment()
using namespace metal;template<int F1>d vec<uint,F1>floatBitsToUint(vec<float,F1>x){return as_type<vec<uint,F1>>(x);}template<int F1>d vec<int,F1>floatBitsToInt(vec<float,F1>x){return as_type<vec<int,F1>>(x);}d uint floatBitsToUint(float x){return as_type<uint>(x);}d int floatBitsToInt(float x){return as_type<int>(x);}template<int F1>d vec<float,F1>uintBitsToFloat(vec<uint,F1>x){return as_type<vec<float,F1>>(x);}d float uintBitsToFloat(uint x){return as_type<float>(x);}d E unpackHalf2x16(uint x){return as_type<E>(x);}d uint packHalf2x16(E x){return as_type<uint>(x);}d i unpackUnorm4x8(uint x){return unpack_unorm4x8_to_half(x);}d uint packUnorm4x8(i x){return pack_half_to_unorm4x8(x);}d W inverse(W h1){W ba=W(h1[1][1],-h1[0][1],-h1[1][0],h1[0][0]);float qf=(ba[0][0]*h1[0][0])+(ba[0][1]*h1[1][0]);return ba*(1/qf);}d p mix(p m,p b,U5 v0){p e7;for(int w0=0;w0<3;++w0)e7[w0]=v0[w0]?b[w0]:m[w0];return e7;}d c mix(c m,c b,c4 v0){c e7;for(int w0=0;w0<2;++w0)e7[w0]=v0[w0]?b[w0]:m[w0];return e7;}d c mix(c m,c b,float t){return mix(m,b,c(t));}d float mod(float x,float y){return fmod(x,y);}
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive