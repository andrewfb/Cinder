#pragma once

#include "glsl.glsl.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char glsl[] = R"===(#define nb
#ifndef CC
#define CC __VERSION__
#endif
#define c vec2
#define U vec3
#define y4 vec3
#define g vec4
#define e mediump float
#define E mediump vec2
#define p mediump vec3
#define i mediump vec4
#define A6 mediump mat3x3
#define B6 mediump mat2x3
#define d4 mediump mat4x4
#define S ivec2
#define e4 ivec4
#define j1 uvec2
#define M uvec4
#define Y mediump uint
#define c4 bvec2
#define U5 bvec3
#define W6 bvec4
#define W mat2
// unpackHalf2x16 and packHalf2x16 are built-in in GLSL 4.2+ but not in 4.1
// Provide software fallback for GL 4.1 (macOS core profile)
#if CC>=400 && CC<420
float j7(uint h){uint S9=(h>>15u)&1u;uint T9=(h>>10u)&0x1Fu;uint U9=h&0x3FFu;if(T9==0u){if(U9==0u)return S9!=0u?-0.0:0.0;float f=float(U9)/1024.0;return S9!=0u?-f*pow(2.0,-14.0):f*pow(2.0,-14.0);}else if(T9==31u){return U9==0u?(S9!=0u?-1.0/0.0:1.0/0.0):0.0/0.0;}float f=float(U9)/1024.0+1.0;f*=pow(2.0,float(T9)-15.0);return S9!=0u?-f:f;}
E unpackHalf2x16(uint u){return c(j7(u&0xFFFFu),j7(u>>16u));}
uint k7(float f){if(f==0.0)return 0u;uint S9=f<0.0?0x8000u:0u;f=abs(f);if(isinf(f))return S9|0x7C00u;if(isnan(f))return S9|0x7E00u;float V9=floor(log2(f));float W9=f/pow(2.0,V9)-1.0;int T9=int(V9)+15;if(T9<=0){W9=f/pow(2.0,-14.0);return S9|uint(W9*1024.0);}if(T9>=31)return S9|0x7C00u;return S9|(uint(T9)<<10u)|uint(W9*1024.0);}
uint packHalf2x16(c v){return k7(v.x)|(k7(v.y)<<16u);}
#endif
#define d
#define q1(S1) out S1
#define H4(S1) inout S1
#ifdef GL_ANGLE_base_vertex_base_instance_shader_builtin
#extension GL_ANGLE_base_vertex_base_instance_shader_builtin:require
#endif
#ifdef RD
#extension GL_KHR_blend_equation_advanced:require
#endif
#ifdef ZD
#extension GL_EXT_shader_framebuffer_fetch:require
#elif defined(AE)
#extension GL_EXT_shader_pixel_local_storage:require
#elif defined(_EXPORTED_ATLAS_RENDER_TARGET_R32UI_PLS_ANGLE)
#extension GL_ANGLE_shader_pixel_local_storage:require
#elif defined(BE)
#ifdef GL_ARB_shader_image_load_store
#extension GL_ARB_shader_image_load_store:require
#endif
#ifdef GL_OES_shader_image_atomic
#extension GL_OES_shader_image_atomic:require
#endif
#endif
#if defined(AB)&&defined(Z)&&defined(GL_ES)&&!defined(VD)
#ifdef GL_EXT_clip_cull_distance
#extension GL_EXT_clip_cull_distance:require
#elif defined(GL_ANGLE_clip_cull_distance)
#extension GL_ANGLE_clip_cull_distance:require
#endif
#endif
#if CC>=420||(CC>=310&&defined(GL_ES))
#define K5(f,a) layout(binding=f,std140)uniform a{
#else
#define K5(f,a) layout(std140)uniform a{
#endif
#define D6(a) }a;
#define w1(a)
#define l0(f,V,a) layout(location=f)in V a
#define x1
#define n0(i8,F,a,V)
// Skip layout(location) for varyings on GL 4.0/4.1 due to ordering issues with flat/noperspective
#ifdef BB
#if CC>=420||(CC>=310&&CC<400)
#define X(f,V,a) layout(location=f)out V a
#else
#define X(f,V,a) out V a
#endif
#else
#if CC>=420||(CC>=310&&CC<400)
#define X(f,V,a) layout(location=f)in V a
#else
#define X(f,V,a) in V a
#endif
#endif
#define x4 flat
#define V1
#define N1
#ifdef UB
#define B0
#else
#ifdef GL_NV_shader_noperspective_interpolation
#extension GL_NV_shader_noperspective_interpolation:require
#define B0 noperspective
#else
#define B0
#endif
#endif
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
#ifdef UB
#define Z3(K,f,a) layout(set=K,binding=f)uniform highp utexture2D a
#define P4(K,f,a) layout(set=K,binding=f)uniform highp texture2D a
#define K2(K,f,a) layout(set=K,binding=f)uniform mediump texture2D a
#define S4(K,f,a) layout(binding=f)uniform mediump texture2D a
#define H9(K,f,a) layout(binding=f)uniform highp Vg a
#define G9(K,f,a) layout(binding=f)uniform highp utexture2D a
#if defined(GB)&&defined(AB)
#endif
// binding qualifier requires GL 4.2+ on desktop GL, or GLES 3.1+
#elif CC>=420||(CC>=310&&defined(GL_ES))
#define Z3(K,f,a) layout(binding=f)uniform highp usampler2D a
#define P4(K,f,a) layout(binding=f)uniform highp sampler2D a
#define K2(K,f,a) layout(binding=f)uniform mediump sampler2D a
#define S4(K,f,a) layout(binding=f)uniform mediump sampler2D a
#define H9(K,f,a) layout(binding=f)uniform highp isampler2D a
#define G9(K,f,a) layout(binding=f)uniform highp usampler2D a
#else
#define Z3(K,f,a) uniform highp usampler2D a
#define P4(K,f,a) uniform highp sampler2D a
#define K2(K,f,a) uniform mediump sampler2D a
#define S4(K,f,a) uniform mediump sampler2D a
#define H9(K,f,a) uniform highp isampler2D a
#define G9(K,f,a) uniform highp usampler2D a
#endif
#ifdef UB
#define h4(X4,a) layout(set=Nb,binding=X4)uniform mediump sampler a;
#define X3(K,f,a) layout(set=K,binding=f)uniform mediump sampler a;
#define Y4(a,n,k) texture(sampler2D(a,n),k)
#define F2(a,n,k,M0) textureLod(sampler2D(a,n),k,M0)
#define Z4(a,n,k,G1) texture(sampler2D(a,n),k,G1)
#if defined(GB)&&defined(AB)
#extension GL_OES_sample_variables:require
#endif
#else
#define h4(X4,a)
#define X3(K,f,a)
#define Y4(a,n,k) texture(a,k)
#define F2(a,n,k,M0) textureLod(a,k,M0)
#define Z4(a,n,k,G1) texture(a,k,G1)
#endif
#define C7(d0,n,k) Y4(d0,n,k)
#define l8(d0,n,k,M0) F2(d0,n,k,M0)
#define X6(d0,n,k,G1) Z4(d0,n,k,G1)
#define N5(K,f,a) S4(K,f,a)
#define y6(a,n,o,V5,m8,M0) F2(a,n,c(o,m8),M0)
#define hf(K,f,a) Z3(K,f,a)
#define f3
#define f1
#define D1(a,k) texelFetch(a,k,0)
#ifdef UB
#define i4(a,n,k,g3) textureGather(sampler2D(a,n),(k)*(g3))
#elif CC>=310
#define i4(a,n,k,g3) textureGather(a,(k)*(g3))
#else
#define i4(a,n,k,g3) kb(a,k,.x)
#endif
#define U3
#define V3
#define D4
#define G4
#ifdef WE
#define E4(f,o1,a) Z3(r2,f,a)
#define g4(f,o1,a) hf(r2,f,a)
#define F4(f,o1,a) P4(r2,f,a)
#define E0(a,r0) D1(a,S((r0)&Cb,(r0)>>Bb))
#define q5(a,r0) D1(a,S((r0)&Cb,(r0)>>Bb)).xy
#else
#ifdef GL_ARB_shader_storage_buffer_object
#extension GL_ARB_shader_storage_buffer_object:require
#endif
#define E4(f,o1,a) layout(std430,binding=f)readonly buffer o1{j1 n4[];}a
#define g4(f,o1,a) layout(std430,binding=f)readonly buffer o1{M n4[];}a
#define F4(f,o1,a) layout(std430,binding=f)readonly buffer o1{g n4[];}a
#define jf(f,o1,a) layout(std430,binding=f)buffer o1{uint n4[];}a
#define E0(a,r0) a.n4[r0]
#define q5(a,r0) a.n4[r0]
#define kf(a,r0) a.n4[r0]
#define Z9(a,r0,o) atomicMax(a.n4[r0],o)
#define rc(a,r0,o) atomicAdd(a.n4[r0],o)
#endif
#ifdef _EXPORTED_PLS_IMPL_ANGLE
#extension GL_ANGLE_shader_pixel_local_storage:require
#define n2
#define C0(f,a) layout(binding=f,rgba8)uniform lowp pixelLocalANGLE a
#define U0(f,a) layout(binding=f,r32ui)uniform highp upixelLocalANGLE a
#define o2
#define S0(h) pixelLocalLoadANGLE(h)
#define T0(h) pixelLocalLoadANGLE(h).x
#define V0(h,C) pixelLocalStoreANGLE(h,C)
#define W0(h,C) pixelLocalStoreANGLE(h,uvec4(C))
#define f2(h)
#define L1(h)
#define g2
#define h2
#endif
#ifdef XE
#ifdef EB
#extension GL_EXT_shader_pixel_local_storage2:require
#else
#extension GL_EXT_shader_pixel_local_storage:require
#endif
#define n2 __pixel_localEXT g1{
#define C0(f,a) layout(rgba8)lowp vec4 a
#define U0(f,a) layout(r32ui)highp uint a
#define o2 };
#define S0(h) h
#define T0(h) h
#define V0(h,C) h=(C)
#define W0(h,C) h=(C)
#define f2(h) h=h
#define L1(h) h=h
#define g2
#define h2
#ifdef EB
#define G2(a) layout(location=0,rgba8)out i l1;C1(a)
#define P3(a) layout(location=0,rgba8)out i l1;C1(a)
#endif
#endif
#ifdef YE
#ifdef GL_ARB_shader_image_load_store
#extension GL_ARB_shader_image_load_store:require
#endif
#if defined(GL_ARB_fragment_shader_interlock)
#extension GL_ARB_fragment_shader_interlock:require
#define g2 beginInvocationInterlockARB()
#define h2 endInvocationInterlockARB()
#elif defined(GL_INTEL_fragment_shader_ordering)
#extension GL_INTEL_fragment_shader_ordering:require
#define g2 beginFragmentShaderOrderingINTEL()
#define h2
#else
#define g2
#define h2
#endif
#define n2
#ifdef UB
#define C0(f,a) layout(set=f4,binding=f,rgba8)uniform lowp coherent image2D a
#define U0(f,a) layout(set=f4,binding=f,r32ui)uniform highp coherent uimage2D a
#else
#define C0(f,a) layout(binding=f,rgba8)uniform lowp coherent image2D a
#define U0(f,a) layout(binding=f,r32ui)uniform highp coherent uimage2D a
#endif
#define o2
#define S0(h) imageLoad(h,G)
#define T0(h) imageLoad(h,G).x
#define V0(h,C) imageStore(h,G,C)
#define W0(h,C) imageStore(h,G,uvec4(C))
#define f2(h)
#define L1(h)
#ifndef JD
#define JD
#endif
#endif
#ifdef ZE
#define n2
#define K3(f,a) layout(input_attachment_index=f,binding=f,set=f4)uniform lowp subpassInput Y6##a;
#define C0(f,a) K3(f,a);layout(location=f)out lowp vec4 a
#define U0(f,a) layout(input_attachment_index=f,binding=f,set=f4)uniform highp usubpassInput Y6##a;layout(location=f)out highp uvec4 a
#define o2
#define S0(h) subpassLoad(Y6##h)
#define T0(h) subpassLoad(Y6##h).x
#define V0(h,C) h=(C)
#define W0(h,C) h.x=(C)
#define f2(h) V0(h,subpassLoad(Y6##h))
#define L1(h) W0(h,subpassLoad(Y6##h).x)
#define g2
#define h2
#endif
#ifdef AF
#define n2
#define C0(f,a) layout(location=f)out lowp vec4 a
#define U0(f,a) layout(location=f)out highp uvec4 a
#define o2
#define S0(h) vec4(0)
#define T0(h) 0u
#define V0(h,C) h=(C)
#define W0(h,C) h.x=(C)
#define f2(h) h=vec4(0)
#define L1(h) h.x=0u
#define g2
#define h2
#endif
#ifndef K3
#define K3 C0
#endif
#ifdef UB
#define gl_VertexID gl_VertexIndex
#endif
#ifdef CE
#ifdef UB
#define n8 gl_InstanceIndex
#else
#ifdef KD
uniform highp int KD;
#define n8 (gl_InstanceID+KD)
#else
#define n8 (gl_InstanceID+gl_BaseInstance)
#endif
#endif
#else
#define n8 0
#endif
#define R5
#define U2
#define F6
#define a5
#define y1(a,c0,F,B,O) void main(){int B=gl_VertexID;int O=n8;
#define o7 y1
#define g6(a,O2,P2,V2,W2,B) y1(a,O2,P2,B,O)
#define T(a,V)
#define g0(a)
#define H(a,V)
#define z1(Q0) gl_Position=Q0;}
#define d2(z3,a) layout(location=0)out z3 lf;void main()
#define e2(C) lf=C
#define q0 gl_FragCoord.xy
#define i6
#define H2
#ifdef JD
#ifdef UB
#define M3(f,a) layout(set=f4,binding=f,r32ui)uniform highp coherent uimage2D a
#else
#define M3(f,a) layout(binding=f,r32ui)uniform highp coherent uimage2D a
#endif
#define l3(h) imageLoad(h,G).x
#define m3(h,C) imageStore(h,G,uvec4(C))
#define x5(h,o) imageAtomicMax(h,G,o)
#define z5(h,o) imageAtomicAdd(h,G,o)
#define O3 ,S G
#define K1 ,G
#define C1(a) void main(){S G=ivec2(floor(q0));
#define p2 }
#define sc(Z6,h,C) if(!(Z6)){V0(h,C);}
#define tc(Z6,h,C) if(!(Z6)){W0(h,C);}
#else
#define O3
#define K1
#define C1(a) void main()
#define p2
#define sc(Z6,h,C) V0(h,C);
#define tc(Z6,h,C) W0(h,C);
#endif
#define v5(a) C1(a)
#ifndef G2
#define G2(a) layout(location=0)out i l1;C1(a)
#endif
#ifndef P3
#define P3(a) layout(location=0)out i l1;C1(a)
#endif
#define L4 p2
#if defined(UB)&&!defined(BF)
#define M6(a) layout(input_attachment_index=0,binding=J3,set=f4)uniform lowp subpassInputMS a
#define o8(a) yb(mat4(subpassLoad(a,0),subpassLoad(a,1),subpassLoad(a,2),subpassLoad(a,3)),gl_SampleMaskIn[0])
#else
#define M6(a) K2(r2,ue,a)
#define o8(a) texelFetch(a,ivec2(floor(q0.xy)),0)
#endif
#define P0(r,D) ((r)*(D))
precision highp float;precision highp int;
#if CC<310
d i unpackUnorm4x8(uint u){M H1=M(u&0xffu,(u>>8)&0xffu,(u>>16)&0xffu,u>>24);return g(H1)*(1./255.);}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive