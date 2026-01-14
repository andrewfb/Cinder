#pragma once

#include "glsl.glsl.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char glsl[] = R"===(#define ob
#ifndef GC
#define GC __VERSION__
#endif
#define c vec2
#define X vec3
#define R3 vec3
#define g vec4
#define d mediump float
#define G mediump vec2
#define r mediump vec3
#define i mediump vec4
#define D6 mediump mat3x3
#define E6 mediump mat2x3
#define o4 mediump mat4x4
#define Y ivec2
#define p4 ivec4
#define a1 uvec2
#define P uvec4
#define V mediump uint
#define n4 bvec2
#define X5 bvec3
#define a7 bvec4
#define d0 mat2
#define e
#define A1(Z1) out Z1
#define O4(Z1) inout Z1
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
#if defined(CB)&&defined(DB)&&defined(GL_ES)&&!defined(VD)
#ifdef GL_EXT_clip_cull_distance
#extension GL_EXT_clip_cull_distance:require
#elif defined(GL_ANGLE_clip_cull_distance)
#extension GL_ANGLE_clip_cull_distance:require
#endif
#endif
#if GC>=310
#define M5(f,a) layout(binding=f,std140)uniform a{
#else
#define M5(f,a) layout(std140)uniform a{
#endif
#define G6(a) }a;
#define g1(a)
#define i0(f,a0,a) layout(location=f)in a0 a
#define h1
#define j0(o8,D,a,a0)
#ifdef BB
#if GC>=310
#define N(f,a0,a) layout(location=f)out a0 a
#else
#define N(f,a0,a) out a0 a
#endif
#else
#if GC>=310
#define N(f,a0,a) layout(location=f)in a0 a
#else
#define N(f,a0,a) in a0 a
#endif
#endif
#define e3 flat
#define D1
#define z1
#ifdef ZB
#define q0
#else
#ifdef GL_NV_shader_noperspective_interpolation
#extension GL_NV_shader_noperspective_interpolation:require
#define q0 noperspective
#else
#define q0
#endif
#endif
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
#ifdef ZB
#define l4(M,f,a) layout(set=M,binding=f)uniform highp utexture2D a
#define T4(M,f,a) layout(set=M,binding=f)uniform highp texture2D a
#define z2(M,f,a) layout(set=M,binding=f)uniform mediump texture2D a
#define W4(M,f,a) layout(binding=f)uniform mediump texture2D a
#define S9(M,f,a) layout(binding=f)uniform highp Xg a
#define R9(M,f,a) layout(binding=f)uniform highp utexture2D a
#if defined(GB)&&defined(CB)
#endif
#elif GC>=310
#define l4(M,f,a) layout(binding=f)uniform highp usampler2D a
#define T4(M,f,a) layout(binding=f)uniform highp sampler2D a
#define z2(M,f,a) layout(binding=f)uniform mediump sampler2D a
#define W4(M,f,a) layout(binding=f)uniform mediump sampler2D a
#define S9(M,f,a) layout(binding=f)uniform highp isampler2D a
#define R9(M,f,a) layout(binding=f)uniform highp usampler2D a
#else
#define l4(M,f,a) uniform highp usampler2D a
#define T4(M,f,a) uniform highp sampler2D a
#define z2(M,f,a) uniform mediump sampler2D a
#define W4(M,f,a) uniform mediump sampler2D a
#define S9(M,f,a) uniform highp isampler2D a
#define R9(M,f,a) uniform highp usampler2D a
#endif
#ifdef ZB
#define w4(c5,a) layout(set=Ob,binding=c5)uniform mediump sampler a;
#define A3(M,f,a) layout(set=M,binding=f)uniform mediump sampler a;
#define d5(a,n,k) texture(sampler2D(a,n),k)
#define d2(a,n,k,Q0) textureLod(sampler2D(a,n),k,Q0)
#define e5(a,n,k,K1) texture(sampler2D(a,n),k,K1)
#if defined(GB)&&defined(CB)
#extension GL_OES_sample_variables:require
#endif
#else
#define w4(c5,a)
#define A3(M,f,a)
#define d5(a,n,k) texture(a,k)
#define d2(a,n,k,Q0) textureLod(a,k,Q0)
#define e5(a,n,k,K1) texture(a,k,K1)
#endif
#define l6(g0,n,k) d5(g0,n,k)
#define r8(g0,n,k,Q0) d2(g0,n,k,Q0)
#define c7(g0,n,k,K1) e5(g0,n,k,K1)
#define Q5(M,f,a) W4(M,f,a)
#define B6(a,n,o,Y5,v8,Q0) d2(a,n,c(o,v8),Q0)
#define tf(M,f,a) l4(M,f,a)
#define p3
#define P0
#define G1(a,k) texelFetch(a,k,0)
#ifdef ZB
#define x4(a,n,k,q3) textureGather(sampler2D(a,n),(k)*(q3))
#elif GC>=310
#define x4(a,n,k,q3) textureGather(a,(k)*(q3))
#else
#define x4(a,n,k,q3) lb(a,k,.x)
#endif
#define g4
#define h4
#define Y3
#define Z3
#ifdef VE
#define M4(f,x1,a) l4(A2,f,a)
#define v4(f,x1,a) tf(A2,f,a)
#define N4(f,x1,a) T4(A2,f,a)
#define A0(a,w0) G1(a,Y((w0)&Db,(w0)>>Cb))
#define Q4(a,w0) G1(a,Y((w0)&Db,(w0)>>Cb)).xy
#else
#ifdef GL_ARB_shader_storage_buffer_object
#extension GL_ARB_shader_storage_buffer_object:require
#endif
#define M4(f,x1,a) layout(std430,binding=f)readonly buffer x1{a1 C4[];}a
#define v4(f,x1,a) layout(std430,binding=f)readonly buffer x1{P C4[];}a
#define N4(f,x1,a) layout(std430,binding=f)readonly buffer x1{g C4[];}a
#define Ie(f,x1,a) layout(std430,binding=f)buffer x1{uint C4[];}a
#define A0(a,w0) a.C4[w0]
#define Q4(a,w0) a.C4[w0]
#define Oe(a,w0) a.C4[w0]
#define O9(a,w0,o) atomicMax(a.C4[w0],o)
#define Sb(a,w0,o) atomicAdd(a.C4[w0],o)
#endif
#ifdef _EXPORTED_PLS_IMPL_ANGLE
#extension GL_ANGLE_shader_pixel_local_storage:require
#define v2
#define F0(f,a) layout(binding=f,rgba8)uniform lowp pixelLocalANGLE a
#define Z0(f,a) layout(binding=f,r32ui)uniform highp upixelLocalANGLE a
#define w2
#define X0(h) pixelLocalLoadANGLE(h)
#define Y0(h) pixelLocalLoadANGLE(h).x
#define c1(h,E) pixelLocalStoreANGLE(h,E)
#define d1(h,E) pixelLocalStoreANGLE(h,uvec4(E))
#define h2(h)
#define Q1(h)
#define i2
#define j2
#endif
#ifdef WE
#ifdef FB
#extension GL_EXT_shader_pixel_local_storage2:require
#else
#extension GL_EXT_shader_pixel_local_storage:require
#endif
#define v2 __pixel_localEXT m1{
#define F0(f,a) layout(rgba8)lowp vec4 a
#define Z0(f,a) layout(r32ui)highp uint a
#define w2 };
#define X0(h) h
#define Y0(h) h
#define c1(h,E) h=(E)
#define d1(h,E) h=(E)
#define h2(h) h=h
#define Q1(h) h=h
#define i2
#define j2
#ifdef FB
#define M2(a) layout(location=0,rgba8)out i r1;F1(a)
#define c4(a) layout(location=0,rgba8)out i r1;F1(a)
#endif
#endif
#ifdef XE
#ifdef GL_ARB_shader_image_load_store
#extension GL_ARB_shader_image_load_store:require
#endif
#if defined(GL_ARB_fragment_shader_interlock)
#extension GL_ARB_fragment_shader_interlock:require
#define i2 beginInvocationInterlockARB()
#define j2 endInvocationInterlockARB()
#elif defined(GL_INTEL_fragment_shader_ordering)
#extension GL_INTEL_fragment_shader_ordering:require
#define i2 beginFragmentShaderOrderingINTEL()
#define j2
#else
#define i2
#define j2
#endif
#define v2
#ifdef ZB
#define F0(f,a) layout(set=q4,binding=f,rgba8)uniform lowp coherent image2D a
#define Z0(f,a) layout(set=q4,binding=f,r32ui)uniform highp coherent uimage2D a
#else
#define F0(f,a) layout(binding=f,rgba8)uniform lowp coherent image2D a
#define Z0(f,a) layout(binding=f,r32ui)uniform highp coherent uimage2D a
#endif
#define w2
#define X0(h) imageLoad(h,H)
#define Y0(h) imageLoad(h,H).x
#define c1(h,E) imageStore(h,H,E)
#define d1(h,E) imageStore(h,H,uvec4(E))
#define h2(h)
#define Q1(h)
#ifndef JD
#define JD
#endif
#endif
#ifdef YE
#define v2
#define U3(f,a) layout(input_attachment_index=f,binding=f,set=q4)uniform lowp subpassInput d7##a;
#define F0(f,a) U3(f,a);layout(location=f)out lowp vec4 a
#define Z0(f,a) layout(input_attachment_index=f,binding=f,set=q4)uniform highp usubpassInput d7##a;layout(location=f)out highp uvec4 a
#define w2
#define X0(h) subpassLoad(d7##h)
#define Y0(h) subpassLoad(d7##h).x
#define c1(h,E) h=(E)
#define d1(h,E) h.x=(E)
#define h2(h) c1(h,subpassLoad(d7##h))
#define Q1(h) d1(h,subpassLoad(d7##h).x)
#define i2
#define j2
#endif
#ifdef ZE
#define v2
#define F0(f,a) layout(location=f)out lowp vec4 a
#define Z0(f,a) layout(location=f)out highp uvec4 a
#define w2
#define X0(h) vec4(0)
#define Y0(h) 0u
#define c1(h,E) h=(E)
#define d1(h,E) h.x=(E)
#define h2(h) h=vec4(0)
#define Q1(h) h.x=0u
#define i2
#define j2
#endif
#ifndef U3
#define U3 F0
#endif
#ifdef ZB
#define gl_VertexID gl_VertexIndex
#endif
#ifdef CE
#ifdef ZB
#define w8 gl_InstanceIndex
#else
#ifdef KD
uniform highp int KD;
#define w8 (gl_InstanceID+KD)
#else
#define w8 (gl_InstanceID+gl_BaseInstance)
#endif
#endif
#else
#define w8 0
#endif
#define U5
#define o2
#define I6
#define f5
#define p1(a,c0,D,p,O) void main(){int p=gl_VertexID;int O=w8;
#define r7 p1
#define w5(a,q2,r2,J2,K2,p) p1(a,q2,r2,p,O)
#define L(a,a0)
#define W(a)
#define I(a,a0)
#define l1(U0) gl_Position=U0;}
#define U1(L3,a) layout(location=0)out L3 uf;void main()
#define V1(E) uf=E
#define v0 gl_FragCoord.xy
#define i6
#define U2
#ifdef JD
#ifdef ZB
#define W3(f,a) layout(set=q4,binding=f,r32ui)uniform highp coherent uimage2D a
#else
#define W3(f,a) layout(binding=f,r32ui)uniform highp coherent uimage2D a
#endif
#define x3(h) imageLoad(h,H).x
#define y3(h,E) imageStore(h,H,uvec4(E))
#define A5(h,o) imageAtomicMax(h,H,o)
#define C5(h,o) imageAtomicAdd(h,H,o)
#define a4 ,Y H
#define P1 ,H
#define F1(a) void main(){Y H=ivec2(floor(v0));
#define x2 }
#define yc(e7,h,E) if(!(e7)){c1(h,E);}
#define zc(e7,h,E) if(!(e7)){d1(h,E);}
#else
#define a4
#define P1
#define F1(a) void main()
#define x2
#define yc(e7,h,E) c1(h,E);
#define zc(e7,h,E) d1(h,E);
#endif
#define y5(a) F1(a)
#ifndef M2
#define M2(a) layout(location=0)out i r1;F1(a)
#endif
#ifndef c4
#define c4(a) layout(location=0)out i r1;F1(a)
#endif
#define S4 x2
#if defined(ZB)&&!defined(AF)
#define Q6(a) layout(input_attachment_index=0,binding=T3,set=q4)uniform lowp subpassInputMS a
#define x8(a) zb(mat4(subpassLoad(a,0),subpassLoad(a,1),subpassLoad(a,2),subpassLoad(a,3)),gl_SampleMaskIn[0])
#else
#define Q6(a) z2(A2,ve,a)
#define x8(a) texelFetch(a,ivec2(floor(v0.xy)),0)
#endif
#define H0(A,F) ((A)*(F))
precision highp float;precision highp int;
#if GC<310
e i unpackUnorm4x8(uint u){P L1=P(u&0xffu,(u>>8)&0xffu,(u>>16)&0xffu,u>>24);return g(L1)*(1./255.);}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive