#pragma once

#include "glsl.glsl.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char glsl[] = R"===(/*
 * Copyright 2023 Rive
 */

// This header provides GLSL-specific #defines and declarations that enable our
// shaders to be compiled on MSL and GLSL both.

#define GLSL

#ifndef _EXPORTED_GLSL_VERSION
// In "#version 320 es", Qualcomm incorrectly substitutes __VERSION__ to 300.
// @GLSL_VERSION is a workaround for this.
#define _EXPORTED_GLSL_VERSION  __VERSION__
#endif

// Explicit binding qualifier support (layout(binding=IDX)):
// - GLES 3.1+ (GLSL 310-320 es) supports it
// - Desktop GL 4.2+ (GLSL 420+) supports it
// - Desktop GL 4.0/4.1 (GLSL 400-419) does NOT support it
// The C++ code calls glUniformBlockBinding() as fallback when not supported.
#if (_EXPORTED_GLSL_VERSION >= 310 && _EXPORTED_GLSL_VERSION < 400) || _EXPORTED_GLSL_VERSION >= 420
#define _EXPORTED_GLSL_HAS_EXPLICIT_BINDINGS  1
#endif

#define float2  vec2
#define float3  vec3
#define packed_float3  vec3
#define float4  vec4

#define half  mediump float
#define half2  mediump vec2
#define half3  mediump vec3
#define half4  mediump vec4
#define half3x3  mediump mat3x3
#define half2x3  mediump mat2x3
#define half4x4  mediump mat4x4

#define int2  ivec2
#define int3  ivec3
#define int4  ivec4

#define short  mediump int
#define short2  mediump ivec2
#define short3  mediump ivec3
#define short4  mediump ivec4

#define uint2  uvec2
#define uint3  uvec3
#define uint4  uvec4

#define ushort  mediump uint
#define ushort2  mediump uvec2
#define ushort3  mediump uvec3
#define ushort4  mediump uvec4

#define bool2  bvec2
#define bool3  bvec3
#define bool4  bvec4

#define float2x2  mat2

#define INLINE
#define OUT(ARG_TYPE)  out ARG_TYPE
#define INOUT(ARG_TYPE)  inout ARG_TYPE

// unpackHalf2x16 and packHalf2x16 are built-in in GLSL 4.2+ but not in 4.1
// Provide software fallback for GL 4.1 (macOS core profile)
#if _EXPORTED_GLSL_VERSION >= 400 && _EXPORTED_GLSL_VERSION < 420
INLINE float halfBitsToFloat(uint h)
{
    uint sign = (h >> 15u) & 1u;
    uint exp = (h >> 10u) & 0x1Fu;
    uint mant = h & 0x3FFu;
    if (exp == 0u) {
        // Denormal or zero
        if (mant == 0u) return sign != 0u ? -0.0 : 0.0;
        // Denormal - convert to normal float
        float f = float(mant) / 1024.0;
        return sign != 0u ? -f * pow(2.0, -14.0) : f * pow(2.0, -14.0);
    } else if (exp == 31u) {
        // Inf or NaN
        return mant == 0u ? (sign != 0u ? -1.0/0.0 : 1.0/0.0) : 0.0/0.0;
    }
    // Normal number
    float f = float(mant) / 1024.0 + 1.0;
    f *= pow(2.0, float(exp) - 15.0);
    return sign != 0u ? -f : f;
}
INLINE half2 unpackHalf2x16(uint u)
{
    return vec2(halfBitsToFloat(u & 0xFFFFu), halfBitsToFloat(u >> 16u));
}
INLINE uint floatToHalfBits(float f)
{
    if (f == 0.0) return 0u;
    uint sign = f < 0.0 ? 0x8000u : 0u;
    f = abs(f);
    if (isinf(f)) return sign | 0x7C00u;
    if (isnan(f)) return sign | 0x7E00u;
    float e = floor(log2(f));
    float m = f / pow(2.0, e) - 1.0;
    int exp = int(e) + 15;
    if (exp <= 0) {
        // Denormal
        m = f / pow(2.0, -14.0);
        return sign | uint(m * 1024.0);
    }
    if (exp >= 31) return sign | 0x7C00u; // Overflow to inf
    return sign | (uint(exp) << 10u) | uint(m * 1024.0);
}
INLINE uint packHalf2x16(vec2 v)
{
    return floatToHalfBits(v.x) | (floatToHalfBits(v.y) << 16u);
}
#endif // GL 4.1 fallback for half-float packing

#ifdef GL_ANGLE_base_vertex_base_instance_shader_builtin
#extension GL_ANGLE_base_vertex_base_instance_shader_builtin : require
#endif

#ifdef _EXPORTED_ENABLE_KHR_BLEND
#extension GL_KHR_blend_equation_advanced : require
#endif

// Enable the necessary extensions for rendering the feather atlas.
// NOTE: We do this here instead of render_atlas.glsl because extensions have to
// be declared before any code.
#ifdef _EXPORTED_ATLAS_RENDER_TARGET_R32UI_FRAMEBUFFER_FETCH
#extension GL_EXT_shader_framebuffer_fetch : require
#elif defined(_EXPORTED_ATLAS_RENDER_TARGET_R32UI_PLS_EXT)
#extension GL_EXT_shader_pixel_local_storage : require
#elif defined(_EXPORTED_ATLAS_RENDER_TARGET_R32UI_PLS_ANGLE)
#extension GL_ANGLE_shader_pixel_local_storage : require
#elif defined(_EXPORTED_ATLAS_RENDER_TARGET_R32I_ATOMIC_TEXTURE)
#ifdef GL_ARB_shader_image_load_store
#extension GL_ARB_shader_image_load_store : require
#endif
#ifdef GL_OES_shader_image_atomic
#extension GL_OES_shader_image_atomic : require
#endif
#endif

// clang-format off
#if defined(_EXPORTED_RENDER_MODE_MSAA) && defined(_EXPORTED_ENABLE_CLIP_RECT) && defined(GL_ES) && !defined(_EXPORTED_DISABLE_CLIP_DISTANCE_FOR_UBERSHADERS)
#ifdef GL_EXT_clip_cull_distance
#extension GL_EXT_clip_cull_distance : require
#elif defined(GL_ANGLE_clip_cull_distance)
#extension GL_ANGLE_clip_cull_distance : require
#endif
#endif // RENDER_MODE_MSAA && ENABLE_CLIP_RECT && GL_ES && !DISABLE_CLIP_DISTANCE_FOR_UBERSHADERS
// clang-format on

#ifdef _EXPORTED_GLSL_HAS_EXPLICIT_BINDINGS
#define UNIFORM_BLOCK_BEGIN(IDX, NAME)                                          \
    layout(binding = IDX, std140) uniform NAME                                 \
    {
#else
#define UNIFORM_BLOCK_BEGIN(IDX, NAME)                                          \
    layout(std140) uniform NAME                                                \
    {
#endif
// clang-format barrier... Otherwise it tries to merge this #define into the
// above macro...
#define UNIFORM_BLOCK_END(NAME)                                                 \
    }                                                                          \
    NAME;

#define ATTR_BLOCK_BEGIN(NAME)
#define ATTR(IDX, TYPE, NAME)  layout(location = IDX) in TYPE NAME
#define ATTR_BLOCK_END
#define ATTR_LOAD(A, B, C, D)
#define ATTR_UNPACK(ID, attrs, NAME, TYPE)

// GLSL 4.2+: layout(location) with varying supported, any qualifier order works
// GLSL 4.0/4.1: layout(location) has ordering issues with flat, so skip layout
// GLSL 3.10-3.20: layout(location) with varying supported
// GLSL < 3.10: no layout(location) for varyings
#ifdef _EXPORTED_VERTEX
#if _EXPORTED_GLSL_VERSION >= 420
#define VARYING(IDX, TYPE, NAME)  layout(location = IDX) out TYPE NAME
#elif _EXPORTED_GLSL_VERSION >= 400
// Skip layout qualifier for 4.0/4.1 due to ordering issues with FLAT
#define VARYING(IDX, TYPE, NAME)  out TYPE NAME
#elif _EXPORTED_GLSL_VERSION >= 310
#define VARYING(IDX, TYPE, NAME)  layout(location = IDX) out TYPE NAME
#else
#define VARYING(IDX, TYPE, NAME)  out TYPE NAME
#endif
#else
#if _EXPORTED_GLSL_VERSION >= 420
#define VARYING(IDX, TYPE, NAME)  layout(location = IDX) in TYPE NAME
#elif _EXPORTED_GLSL_VERSION >= 400
#define VARYING(IDX, TYPE, NAME)  in TYPE NAME
#elif _EXPORTED_GLSL_VERSION >= 310
#define VARYING(IDX, TYPE, NAME)  layout(location = IDX) in TYPE NAME
#else
#define VARYING(IDX, TYPE, NAME)  in TYPE NAME
#endif
#endif
#define FLAT  flat
#define VARYING_BLOCK_BEGIN
#define VARYING_BLOCK_END

// clang-format off
#ifdef _EXPORTED_TARGET_VULKAN
   // Since Vulkan is compiled offline and not all platforms support noperspective, don't use it.
#define NO_PERSPECTIVE
#else
#ifdef GL_NV_shader_noperspective_interpolation
#extension GL_NV_shader_noperspective_interpolation : require
#define NO_PERSPECTIVE  noperspective
#else
#define NO_PERSPECTIVE
#endif
#endif
// clang-format on

#ifdef _EXPORTED_VERTEX
#define VERTEX_TEXTURE_BLOCK_BEGIN
#define VERTEX_TEXTURE_BLOCK_END
#endif

#ifdef _EXPORTED_FRAGMENT
#define FRAG_TEXTURE_BLOCK_BEGIN
#define FRAG_TEXTURE_BLOCK_END
#endif

#define DYNAMIC_SAMPLER_BLOCK_BEGIN
#define DYNAMIC_SAMPLER_BLOCK_END

#ifdef _EXPORTED_TARGET_VULKAN
#define TEXTURE_RGBA32UI(SET, IDX, NAME)                                        \
    layout(set = SET, binding = IDX) uniform highp utexture2D NAME
#define TEXTURE_RGBA32F(SET, IDX, NAME)                                         \
    layout(set = SET, binding = IDX) uniform highp texture2D NAME
#define TEXTURE_RGBA8(SET, IDX, NAME)                                           \
    layout(set = SET, binding = IDX) uniform mediump texture2D NAME
#define TEXTURE_R16F(SET, IDX, NAME)                                            \
    layout(binding = IDX) uniform mediump texture2D NAME
#define TEXTURE_R32I(SET, IDX, NAME)                                            \
    layout(binding = IDX) uniform highp itexture2D NAME
#define TEXTURE_R32UI(SET, IDX, NAME)                                           \
    layout(binding = IDX) uniform highp utexture2D NAME
#if defined(_EXPORTED_FRAGMENT) && defined(_EXPORTED_RENDER_MODE_MSAA)
#endif // @FRAGMENT && @RENDER_MODE_MSAA
#elif defined(_EXPORTED_GLSL_HAS_EXPLICIT_BINDINGS)
#define TEXTURE_RGBA32UI(SET, IDX, NAME)                                        \
    layout(binding = IDX) uniform highp usampler2D NAME
#define TEXTURE_RGBA32F(SET, IDX, NAME)                                         \
    layout(binding = IDX) uniform highp sampler2D NAME
#define TEXTURE_RGBA8(SET, IDX, NAME)                                           \
    layout(binding = IDX) uniform mediump sampler2D NAME
#define TEXTURE_R16F(SET, IDX, NAME)                                            \
    layout(binding = IDX) uniform mediump sampler2D NAME
#define TEXTURE_R32I(SET, IDX, NAME)                                            \
    layout(binding = IDX) uniform highp isampler2D NAME
#define TEXTURE_R32UI(SET, IDX, NAME)                                           \
    layout(binding = IDX) uniform highp usampler2D NAME
#else
#define TEXTURE_RGBA32UI(SET, IDX, NAME)  uniform highp usampler2D NAME
#define TEXTURE_RGBA32F(SET, IDX, NAME)  uniform highp sampler2D NAME
#define TEXTURE_RGBA8(SET, IDX, NAME)  uniform mediump sampler2D NAME
#define TEXTURE_R16F(SET, IDX, NAME)  uniform mediump sampler2D NAME
#define TEXTURE_R32I(SET, IDX, NAME)  uniform highp isampler2D NAME
#define TEXTURE_R32UI(SET, IDX, NAME)  uniform highp usampler2D NAME
#endif

#ifdef _EXPORTED_TARGET_VULKAN
#define SAMPLER_LINEAR(TEXTURE_IDX, NAME)                                       \
    layout(set = IMMUTABLE_SAMPLER_BINDINGS_SET, binding = TEXTURE_IDX)        \
        uniform mediump sampler NAME;
#define SAMPLER_MIPMAP(TEXTURE_IDX, NAME)                                       \
    layout(set = IMMUTABLE_SAMPLER_BINDINGS_SET, binding = TEXTURE_IDX)        \
        uniform mediump sampler NAME;
#define SAMPLER_DYNAMIC(SET, IDX, NAME)                                         \
    layout(set = SET, binding = IDX) uniform mediump sampler NAME;
#define TEXTURE_SAMPLE(NAME, SAMPLER_NAME, COORD)                               \
    texture(sampler2D(NAME, SAMPLER_NAME), COORD)
#define TEXTURE_SAMPLE_LOD(NAME, SAMPLER_NAME, COORD, LOD)                      \
    textureLod(sampler2D(NAME, SAMPLER_NAME), COORD, LOD)
#define TEXTURE_SAMPLE_LODBIAS(NAME, SAMPLER_NAME, COORD, LODBIAS)              \
    texture(sampler2D(NAME, SAMPLER_NAME), COORD, LODBIAS)
#define TEXTURE_SAMPLE_GRAD(NAME, SAMPLER_NAME, COORD, DDX, DDY)                \
    textureGrad(sampler2D(NAME, SAMPLER_NAME), COORD, DDX, DDY)
#if defined(_EXPORTED_FRAGMENT) && defined(_EXPORTED_RENDER_MODE_MSAA)
#extension GL_OES_sample_variables : require
#endif // @FRAGMENT && @RENDER_MODE_MSAA
#else  // @TARGET_VULKAN -> !@TARGET_VULKAN
// SAMPLER_LINEAR and SAMPLER_MIPMAP are no-ops because in GL, sampling
// parameters are API-level state tied to the texture.
#define SAMPLER_LINEAR(TEXTURE_IDX, NAME)
#define SAMPLER_MIPMAP(TEXTURE_IDX, NAME)
#define SAMPLER_DYNAMIC(SET, IDX, NAME)
#define TEXTURE_SAMPLE(NAME, SAMPLER_NAME, COORD)  texture(NAME, COORD)
#define TEXTURE_SAMPLE_LOD(NAME, SAMPLER_NAME, COORD, LOD)                      \
    textureLod(NAME, COORD, LOD)
#define TEXTURE_SAMPLE_LODBIAS(NAME, SAMPLER_NAME, COORD, LODBIAS)              \
    texture(NAME, COORD, LODBIAS)
#define TEXTURE_SAMPLE_GRAD(NAME, SAMPLER_NAME, COORD, DDX, DDY)                \
    textureGrad(NAME, COORD, DDX, DDY)
#endif // !@TARGET_VULKAN

#define TEXTURE_SAMPLE_DYNAMIC(TEXTURE, SAMPLER_NAME, COORD)                    \
    TEXTURE_SAMPLE(TEXTURE, SAMPLER_NAME, COORD)
#define TEXTURE_SAMPLE_DYNAMIC_LOD(TEXTURE, SAMPLER_NAME, COORD, LOD)           \
    TEXTURE_SAMPLE_LOD(TEXTURE, SAMPLER_NAME, COORD, LOD)
#define TEXTURE_SAMPLE_DYNAMIC_LODBIAS(TEXTURE, SAMPLER_NAME, COORD, LODBIAS)   \
    TEXTURE_SAMPLE_LODBIAS(TEXTURE, SAMPLER_NAME, COORD, LODBIAS)

// Polyfill the feather texture as a sampler2D since ES doesn't support
// sampler1DArray. This is why the macro needs "ARRAY_INDEX_NORMALIZED": when
// polyfilled as a 2D texture, the "array index" needs to be a 0..1 normalized
// y coordinate instead of the literal array index.
#define TEXTURE_R16F_1D_ARRAY(SET, IDX, NAME)  TEXTURE_R16F(SET, IDX, NAME)
// clang-format off
// Clang formatting on this line trips up the Qualcomm compiler.
#define TEXTURE_SAMPLE_LOD_1D_ARRAY(NAME, SAMPLER_NAME, X, ARRAY_INDEX, ARRAY_INDEX_NORMALIZED, LOD)                                        \
    TEXTURE_SAMPLE_LOD(NAME, SAMPLER_NAME, float2(X, ARRAY_INDEX_NORMALIZED), LOD)
// clang-format on

#define TEXTURE_RG32UI(SET, IDX, NAME)  TEXTURE_RGBA32UI(SET, IDX, NAME)

#define TEXTURE_CONTEXT_DECL

#define TEXTURE_CONTEXT_FORWARD
#define TEXEL_FETCH(NAME, COORD)  texelFetch(NAME, COORD, 0)

#ifdef _EXPORTED_TARGET_VULKAN
#define TEXTURE_GATHER(NAME, SAMPLER_NAME, COORD, TEXTURE_INVERSE_SIZE)         \
    textureGather(sampler2D(NAME, SAMPLER_NAME),                               \
                  (COORD) * (TEXTURE_INVERSE_SIZE))
#elif _EXPORTED_GLSL_VERSION >= 400
// textureGather is available in GLSL 400+ (desktop GL 4.0+) and GLES 3.1+
#define TEXTURE_GATHER(NAME, SAMPLER_NAME, COORD, TEXTURE_INVERSE_SIZE)         \
    textureGather(NAME, (COORD) * (TEXTURE_INVERSE_SIZE))
#elif _EXPORTED_GLSL_VERSION >= 310
// GLES 3.1 (GLSL 310 es)
#define TEXTURE_GATHER(NAME, SAMPLER_NAME, COORD, TEXTURE_INVERSE_SIZE)         \
    textureGather(NAME, (COORD) * (TEXTURE_INVERSE_SIZE))
#else
#define TEXTURE_GATHER(NAME, SAMPLER_NAME, COORD, TEXTURE_INVERSE_SIZE)         \
    TEXTURE_GATHER_MATRIX(NAME, COORD, .x)
#endif

#define VERTEX_STORAGE_BUFFER_BLOCK_BEGIN
#define VERTEX_STORAGE_BUFFER_BLOCK_END

#define FRAG_STORAGE_BUFFER_BLOCK_BEGIN
#define FRAG_STORAGE_BUFFER_BLOCK_END

#ifdef _EXPORTED_DISABLE_SHADER_STORAGE_BUFFERS

#define STORAGE_BUFFER_U32x2(IDX, GLSL_STRUCT_NAME, NAME)                       \
    TEXTURE_RGBA32UI(PER_FLUSH_BINDINGS_SET, IDX, NAME)
#define STORAGE_BUFFER_U32x4(IDX, GLSL_STRUCT_NAME, NAME)                       \
    TEXTURE_RG32UI(PER_FLUSH_BINDINGS_SET, IDX, NAME)
#define STORAGE_BUFFER_F32x4(IDX, GLSL_STRUCT_NAME, NAME)                       \
    TEXTURE_RGBA32F(PER_FLUSH_BINDINGS_SET, IDX, NAME)
#define STORAGE_BUFFER_LOAD4(NAME, I)                                           \
    TEXEL_FETCH(                                                               \
        NAME,                                                                  \
        int2((I) & STORAGE_TEXTURE_MASK_X, (I) >> STORAGE_TEXTURE_SHIFT_Y))
#define STORAGE_BUFFER_LOAD2(NAME, I)                                           \
    TEXEL_FETCH(                                                               \
        NAME,                                                                  \
        int2((I) & STORAGE_TEXTURE_MASK_X, (I) >> STORAGE_TEXTURE_SHIFT_Y))    \
        .xy

#else

#ifdef GL_ARB_shader_storage_buffer_object
#extension GL_ARB_shader_storage_buffer_object : require
#endif
)===" R"===(
#define STORAGE_BUFFER_U32x2(IDX, GLSL_STRUCT_NAME, NAME)                       \
    layout(std430, binding = IDX) readonly buffer GLSL_STRUCT_NAME             \
    {                                                                          \
        uint2 _values[];                                                       \
    }                                                                          \
    NAME
#define STORAGE_BUFFER_U32x4(IDX, GLSL_STRUCT_NAME, NAME)                       \
    layout(std430, binding = IDX) readonly buffer GLSL_STRUCT_NAME             \
    {                                                                          \
        uint4 _values[];                                                       \
    }                                                                          \
    NAME
#define STORAGE_BUFFER_F32x4(IDX, GLSL_STRUCT_NAME, NAME)                       \
    layout(std430, binding = IDX) readonly buffer GLSL_STRUCT_NAME             \
    {                                                                          \
        float4 _values[];                                                      \
    }                                                                          \
    NAME
#define STORAGE_BUFFER_U32_ATOMIC(IDX, GLSL_STRUCT_NAME, NAME)                  \
    layout(std430, binding = IDX) buffer GLSL_STRUCT_NAME { uint _values[]; }  \
    NAME
#define STORAGE_BUFFER_LOAD4(NAME, I)  NAME._values[I]
#define STORAGE_BUFFER_LOAD2(NAME, I)  NAME._values[I]
#define STORAGE_BUFFER_LOAD(NAME, I)  NAME._values[I]
#define STORAGE_BUFFER_ATOMIC_MAX(NAME, I, X)  atomicMax(NAME._values[I], X)
#define STORAGE_BUFFER_ATOMIC_ADD(NAME, I, X)  atomicAdd(NAME._values[I], X)

#endif // DISABLE_SHADER_STORAGE_BUFFERS

// Define macros for implementing pixel local storage based on available
// extensions.
#ifdef _EXPORTED_PLS_IMPL_ANGLE

#extension GL_ANGLE_shader_pixel_local_storage : require

#define PLS_BLOCK_BEGIN
#define PLS_DECL4F(IDX, NAME)                                                   \
    layout(binding = IDX, rgba8) uniform lowp pixelLocalANGLE NAME
#define PLS_DECLUI(IDX, NAME)                                                   \
    layout(binding = IDX, r32ui) uniform highp upixelLocalANGLE NAME
#define PLS_BLOCK_END

#define PLS_LOAD4F(PLANE)  pixelLocalLoadANGLE(PLANE)
#define PLS_LOADUI(PLANE)  pixelLocalLoadANGLE(PLANE).x
#define PLS_STORE4F(PLANE, VALUE)  pixelLocalStoreANGLE(PLANE, VALUE)
#define PLS_STOREUI(PLANE, VALUE)  pixelLocalStoreANGLE(PLANE, uvec4(VALUE))

#define PLS_PRESERVE_4F(PLANE)
#define PLS_PRESERVE_UI(PLANE)

#define PLS_INTERLOCK_BEGIN
#define PLS_INTERLOCK_END

#endif // PLS_IMPL_ANGLE

#ifdef _EXPORTED_PLS_IMPL_EXT_NATIVE

#ifdef _EXPORTED_FIXED_FUNCTION_COLOR_OUTPUT
// fixedFunctionColorOutput renders directly to the framebuffer, which requires
// EXT_shader_pixel_local_storage2.
#extension GL_EXT_shader_pixel_local_storage2 : require
#else
#extension GL_EXT_shader_pixel_local_storage : require
#endif

#define PLS_BLOCK_BEGIN                                                         \
    __pixel_localEXT PLS                                                       \
    {
#define PLS_DECL4F(IDX, NAME)  layout(rgba8) lowp vec4 NAME
#define PLS_DECLUI(IDX, NAME)  layout(r32ui) highp uint NAME
#define PLS_BLOCK_END                                                           \
    }                                                                          \
    ;

#define PLS_LOAD4F(PLANE)  PLANE
#define PLS_LOADUI(PLANE)  PLANE
#define PLS_STORE4F(PLANE, VALUE)  PLANE = (VALUE)
#define PLS_STOREUI(PLANE, VALUE)  PLANE = (VALUE)

#define PLS_PRESERVE_4F(PLANE)  PLANE = PLANE
#define PLS_PRESERVE_UI(PLANE)  PLANE = PLANE

#define PLS_INTERLOCK_BEGIN
#define PLS_INTERLOCK_END

#ifdef _EXPORTED_FIXED_FUNCTION_COLOR_OUTPUT
// EXT_shader_pixel_local_storage2 requires explicit output format qualifiers
// on fragment shader outputs.
#define PLS_FRAG_COLOR_MAIN(NAME)                                               \
    layout(location = 0, rgba8) out half4 _fragColor;                          \
    PLS_MAIN(NAME)

#define PLS_FRAG_COLOR_MAIN_WITH_IMAGE_UNIFORMS(NAME)                           \
    layout(location = 0, rgba8) out half4 _fragColor;                          \
    PLS_MAIN(NAME)
#endif
#endif

#ifdef _EXPORTED_PLS_IMPL_STORAGE_TEXTURE

#ifdef GL_ARB_shader_image_load_store
#extension GL_ARB_shader_image_load_store : require
#endif
#if defined(GL_ARB_fragment_shader_interlock)
#extension GL_ARB_fragment_shader_interlock : require
#define PLS_INTERLOCK_BEGIN  beginInvocationInterlockARB()
#define PLS_INTERLOCK_END  endInvocationInterlockARB()
#elif defined(GL_INTEL_fragment_shader_ordering)
#extension GL_INTEL_fragment_shader_ordering : require
#define PLS_INTERLOCK_BEGIN  beginFragmentShaderOrderingINTEL()
#define PLS_INTERLOCK_END
#else
#define PLS_INTERLOCK_BEGIN
#define PLS_INTERLOCK_END
#endif

#define PLS_BLOCK_BEGIN
#ifdef _EXPORTED_TARGET_VULKAN
#define PLS_DECL4F(IDX, NAME)                                                   \
    layout(set = PLS_TEXTURE_BINDINGS_SET, binding = IDX, rgba8)               \
        uniform lowp coherent image2D NAME
#define PLS_DECLUI(IDX, NAME)                                                   \
    layout(set = PLS_TEXTURE_BINDINGS_SET, binding = IDX, r32ui)               \
        uniform highp coherent uimage2D NAME
#else
#define PLS_DECL4F(IDX, NAME)                                                   \
    layout(binding = IDX, rgba8) uniform lowp coherent image2D NAME
#define PLS_DECLUI(IDX, NAME)                                                   \
    layout(binding = IDX, r32ui) uniform highp coherent uimage2D NAME
#endif
#define PLS_BLOCK_END

#define PLS_LOAD4F(PLANE)  imageLoad(PLANE, _plsCoord)
#define PLS_LOADUI(PLANE)  imageLoad(PLANE, _plsCoord).x
#define PLS_STORE4F(PLANE, VALUE)  imageStore(PLANE, _plsCoord, VALUE)
#define PLS_STOREUI(PLANE, VALUE)  imageStore(PLANE, _plsCoord, uvec4(VALUE))

#define PLS_PRESERVE_4F(PLANE)
#define PLS_PRESERVE_UI(PLANE)

#ifndef _EXPORTED_USING_PLS_STORAGE_TEXTURES
#define _EXPORTED_USING_PLS_STORAGE_TEXTURES

#endif // PLS_IMPL_STORAGE_TEXTURE

#endif // PLS_IMPL_STORAGE_TEXTURE

#ifdef _EXPORTED_PLS_IMPL_SUBPASS_LOAD

#define PLS_BLOCK_BEGIN
#define PLS_DECL4F_READONLY(IDX, NAME)                                          \
    layout(input_attachment_index = IDX,                                       \
           binding = IDX,                                                      \
           set = PLS_TEXTURE_BINDINGS_SET)                                     \
        uniform lowp subpassInput _in_##NAME;
#define PLS_DECL4F(IDX, NAME)                                                   \
    PLS_DECL4F_READONLY(IDX, NAME);                                            \
    layout(location = IDX) out lowp vec4 NAME
#define PLS_DECLUI(IDX, NAME)                                                   \
    layout(input_attachment_index = IDX,                                       \
           binding = IDX,                                                      \
           set = PLS_TEXTURE_BINDINGS_SET)                                     \
        uniform highp usubpassInput _in_##NAME;                                \
    layout(location = IDX) out highp uvec4 NAME
#define PLS_BLOCK_END

#define PLS_LOAD4F(PLANE)  subpassLoad(_in_##PLANE)
#define PLS_LOADUI(PLANE)  subpassLoad(_in_##PLANE).x
#define PLS_STORE4F(PLANE, VALUE)  PLANE = (VALUE)
#define PLS_STOREUI(PLANE, VALUE)  PLANE.x = (VALUE)

#define PLS_PRESERVE_4F(PLANE)  PLS_STORE4F(PLANE, subpassLoad(_in_##PLANE))
#define PLS_PRESERVE_UI(PLANE)  PLS_STOREUI(PLANE, subpassLoad(_in_##PLANE).x)

#define PLS_INTERLOCK_BEGIN
#define PLS_INTERLOCK_END

#endif

#ifdef _EXPORTED_PLS_IMPL_NONE

#define PLS_BLOCK_BEGIN
#define PLS_DECL4F(IDX, NAME)  layout(location = IDX) out lowp vec4 NAME
#define PLS_DECLUI(IDX, NAME)  layout(location = IDX) out highp uvec4 NAME
#define PLS_BLOCK_END

#define PLS_LOAD4F(PLANE)  vec4(0)
#define PLS_LOADUI(PLANE)  0u
#define PLS_STORE4F(PLANE, VALUE)  PLANE = (VALUE)
#define PLS_STOREUI(PLANE, VALUE)  PLANE.x = (VALUE)

#define PLS_PRESERVE_4F(PLANE)  PLANE = vec4(0)
#define PLS_PRESERVE_UI(PLANE)  PLANE.x = 0u

#define PLS_INTERLOCK_BEGIN
#define PLS_INTERLOCK_END

#endif

#ifndef PLS_DECL4F_READONLY
#define PLS_DECL4F_READONLY  PLS_DECL4F
#endif

#ifdef _EXPORTED_TARGET_VULKAN
#define gl_VertexID  gl_VertexIndex
#endif

// clang-format off
#ifdef _EXPORTED_ENABLE_INSTANCE_INDEX
#ifdef _EXPORTED_TARGET_VULKAN
#define INSTANCE_INDEX  gl_InstanceIndex
#else
#ifdef _EXPORTED_BASE_INSTANCE_UNIFORM_NAME
       // gl_BaseInstance isn't supported on this platform. The rendering
       // backend will set this uniform for us instead.
       uniform highp int _EXPORTED_BASE_INSTANCE_UNIFORM_NAME;
#define INSTANCE_INDEX  (gl_InstanceID + _EXPORTED_BASE_INSTANCE_UNIFORM_NAME)
#else
#define INSTANCE_INDEX  (gl_InstanceID + gl_BaseInstance)
#endif
#endif
#else
#define INSTANCE_INDEX  0
#endif
// clang-format on

#define VERTEX_CONTEXT_DECL
#define VERTEX_CONTEXT_UNPACK

#define CLIP_CONTEXT_FORWARD
#define CLIP_CONTEXT_UNPACK

#define VERTEX_MAIN(NAME, Attrs, attrs, _vertexID, _instanceID)                 \
    void main()                                                                \
    {                                                                          \
        int _vertexID = gl_VertexID;                                           \
        int _instanceID = INSTANCE_INDEX;

#define IMAGE_RECT_VERTEX_MAIN  VERTEX_MAIN

// clang-format off
#define IMAGE_MESH_VERTEX_MAIN(NAME, PositionAttr, position, UVAttr, uv, _vertexID)  \
    VERTEX_MAIN(NAME, PositionAttr, position, _vertexID, _instanceID)
// clang-format on

#define VARYING_INIT(NAME, TYPE)
#define VARYING_PACK(NAME)
#define VARYING_UNPACK(NAME, TYPE)

#define EMIT_VERTEX(_pos)                                                       \
    gl_Position = _pos;                                                        \
    }

#define FRAG_DATA_MAIN(DATA_TYPE, NAME)                                         \
    layout(location = 0) out DATA_TYPE _fd;                                    \
    void main()

#define EMIT_FRAG_DATA(VALUE)  _fd = VALUE

#define _fragCoord  gl_FragCoord.xy

#define FRAGMENT_CONTEXT_DECL
#define FRAGMENT_CONTEXT_UNPACK

#ifdef _EXPORTED_USING_PLS_STORAGE_TEXTURES

#ifdef _EXPORTED_TARGET_VULKAN
#define PLS_DECLUI_ATOMIC(IDX, NAME)                                            \
    layout(set = PLS_TEXTURE_BINDINGS_SET, binding = IDX, r32ui)               \
        uniform highp coherent uimage2D NAME
#else
#define PLS_DECLUI_ATOMIC(IDX, NAME)                                            \
    layout(binding = IDX, r32ui) uniform highp coherent uimage2D NAME
#endif
#define PLS_LOADUI_ATOMIC(PLANE)  imageLoad(PLANE, _plsCoord).x
#define PLS_STOREUI_ATOMIC(PLANE, VALUE)                                        \
    imageStore(PLANE, _plsCoord, uvec4(VALUE))
#define PLS_ATOMIC_MAX(PLANE, X)  imageAtomicMax(PLANE, _plsCoord, X)
#define PLS_ATOMIC_ADD(PLANE, X)  imageAtomicAdd(PLANE, _plsCoord, X)

#define PLS_CONTEXT_DECL  , int2 _plsCoord
#define PLS_CONTEXT_UNPACK  , _plsCoord

#define PLS_MAIN(NAME)                                                          \
    void main()                                                                \
    {                                                                          \
        int2 _plsCoord = ivec2(floor(_fragCoord));

#define EMIT_PLS  }

// Storage textures are expensive to update. It's faster to conditionally update
// them when possible.
#define PLS_STORE4F_OPTIONAL_IF(CONDITION, PLANE, VALUE)                        \
    if (!(CONDITION))                                                          \
    {                                                                          \
        PLS_STORE4F(PLANE, VALUE);                                             \
    }
#define PLS_STOREUI_OPTIONAL_IF(CONDITION, PLANE, VALUE)                        \
    if (!(CONDITION))                                                          \
    {                                                                          \
        PLS_STOREUI(PLANE, VALUE);                                             \
    }

#else // !USING_PLS_STORAGE_TEXTURES

#define PLS_CONTEXT_DECL
#define PLS_CONTEXT_UNPACK

#define PLS_MAIN(NAME)  void main()
#define EMIT_PLS

// Cheap forms of PLS do better to update unconditionally, even if it might be a
// no-op. (Especially since we otherwise would have had to preserve anyway.)
#define PLS_STORE4F_OPTIONAL_IF(CONDITION, PLANE, VALUE)                        \
    PLS_STORE4F(PLANE, VALUE);
#define PLS_STOREUI_OPTIONAL_IF(CONDITION, PLANE, VALUE)                        \
    PLS_STOREUI(PLANE, VALUE);

#endif // !USING_PLS_STORAGE_TEXTURES

#define PLS_MAIN_WITH_IMAGE_UNIFORMS(NAME)  PLS_MAIN(NAME)

#ifndef PLS_FRAG_COLOR_MAIN
#define PLS_FRAG_COLOR_MAIN(NAME)                                               \
    layout(location = 0) out half4 _fragColor;                                 \
    PLS_MAIN(NAME)
#endif

#ifndef PLS_FRAG_COLOR_MAIN_WITH_IMAGE_UNIFORMS
#define PLS_FRAG_COLOR_MAIN_WITH_IMAGE_UNIFORMS(NAME)                           \
    layout(location = 0) out half4 _fragColor;                                 \
    PLS_MAIN(NAME)
#endif

#define EMIT_PLS_AND_FRAG_COLOR  EMIT_PLS

#if defined(_EXPORTED_TARGET_VULKAN) && !defined(_EXPORTED_INPUT_ATTACHMENT_NONE)
#define DST_COLOR_TEXTURE(NAME)                                                 \
    layout(input_attachment_index = 0,                                         \
           binding = COLOR_PLANE_IDX,                                          \
           set = PLS_TEXTURE_BINDINGS_SET) uniform lowp subpassInputMS NAME
#define DST_COLOR_FETCH(NAME)                                                   \
    dst_color_fetch(mat4(subpassLoad(NAME, 0),                                 \
                         subpassLoad(NAME, 1),                                 \
                         subpassLoad(NAME, 2),                                 \
                         subpassLoad(NAME, 3)),                                \
                    gl_SampleMaskIn[0])
#else
#define DST_COLOR_TEXTURE(NAME)                                                 \
    TEXTURE_RGBA8(PER_FLUSH_BINDINGS_SET, DST_COLOR_TEXTURE_IDX, NAME)
#define DST_COLOR_FETCH(NAME)  texelFetch(NAME, ivec2(floor(_fragCoord.xy)), 0)
#endif

#define MUL(A, B)  ((A) * (B))

precision highp float;
precision highp int;

#if _EXPORTED_GLSL_VERSION < 310
// Polyfill ES 3.1+ methods.
INLINE half4 unpackUnorm4x8(uint u)
{
    uint4 vals = uint4(u & 0xffu, (u >> 8) & 0xffu, (u >> 16) & 0xffu, u >> 24);
    return float4(vals) * (1. / 255.);
}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive