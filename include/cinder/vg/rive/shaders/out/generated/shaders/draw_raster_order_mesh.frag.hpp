#pragma once

#include "draw_raster_order_mesh.frag.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char draw_raster_order_mesh_frag[] = R"===(/*
 * Copyright 2023 Rive
 */

#ifdef _EXPORTED_FRAGMENT

#if defined(_EXPORTED_FIXED_FUNCTION_COLOR_OUTPUT) && !defined(_EXPORTED_ENABLE_CLIPPING)
// @FIXED_FUNCTION_COLOR_OUTPUT without clipping can skip the interlock.
#undef NEEDS_INTERLOCK
#else
#define NEEDS_INTERLOCK
#endif

PLS_BLOCK_BEGIN
#ifndef _EXPORTED_FIXED_FUNCTION_COLOR_OUTPUT
PLS_DECL4F(COLOR_PLANE_IDX, colorBuffer);
#endif
PLS_DECLUI(CLIP_PLANE_IDX, clipBuffer);
#ifndef _EXPORTED_FIXED_FUNCTION_COLOR_OUTPUT
PLS_DECL4F(SCRATCH_COLOR_PLANE_IDX, scratchColorBuffer);
#endif
PLS_DECLUI(COVERAGE_PLANE_IDX, coverageBuffer);
PLS_BLOCK_END

// ATLAS_BLIT includes draw_path_common.glsl, which declares the textures &
// samplers, so we only need to declare these for image meshes.
#ifdef _EXPORTED_DRAW_IMAGE_MESH
FRAG_TEXTURE_BLOCK_BEGIN
TEXTURE_RGBA8(PER_DRAW_BINDINGS_SET, IMAGE_TEXTURE_IDX, _EXPORTED_imageTexture);
FRAG_TEXTURE_BLOCK_END

DYNAMIC_SAMPLER_BLOCK_BEGIN
SAMPLER_DYNAMIC(PER_DRAW_BINDINGS_SET, IMAGE_SAMPLER_IDX, imageSampler)
DYNAMIC_SAMPLER_BLOCK_END

FRAG_STORAGE_BUFFER_BLOCK_BEGIN
FRAG_STORAGE_BUFFER_BLOCK_END
#endif // @DRAW_IMAGE_MESH

#ifdef _EXPORTED_FIXED_FUNCTION_COLOR_OUTPUT
#ifdef _EXPORTED_DRAW_IMAGE_MESH
PLS_FRAG_COLOR_MAIN_WITH_IMAGE_UNIFORMS(_EXPORTED_drawFragmentMain)
#else
PLS_FRAG_COLOR_MAIN(_EXPORTED_drawFragmentMain)
#endif
#else
#ifdef _EXPORTED_DRAW_IMAGE_MESH
PLS_MAIN_WITH_IMAGE_UNIFORMS(_EXPORTED_drawFragmentMain)
#else
PLS_MAIN(_EXPORTED_drawFragmentMain)
#endif
#endif
{
#ifdef _EXPORTED_ATLAS_BLIT
    VARYING_UNPACK(v_paint, float4);
    VARYING_UNPACK(v_atlasCoord, float2);
#endif
#ifdef _EXPORTED_DRAW_IMAGE_MESH
    VARYING_UNPACK(v_texCoord, float2);
#endif
#ifdef _EXPORTED_ENABLE_CLIPPING
    VARYING_UNPACK(v_clipID, half);
#endif
#ifdef _EXPORTED_ENABLE_CLIP_RECT
    VARYING_UNPACK(v_clipRect, float4);
#endif
#if defined(_EXPORTED_ATLAS_BLIT) && defined(_EXPORTED_ENABLE_ADVANCED_BLEND)
    VARYING_UNPACK(v_blendMode, half);
#endif

#ifdef _EXPORTED_ATLAS_BLIT
    half4 color = find_paint_color(v_paint, 1. FRAGMENT_CONTEXT_UNPACK);
    half coverage = filter_feather_atlas(
        v_atlasCoord,
        uniforms.atlasTextureInverseSize TEXTURE_CONTEXT_FORWARD);
#endif

#ifdef _EXPORTED_DRAW_IMAGE_MESH
    half4 color = TEXTURE_SAMPLE_DYNAMIC_LODBIAS(_EXPORTED_imageTexture,
                                                 imageSampler,
                                                 v_texCoord,
                                                 uniforms.mipMapLODBias);
    half coverage = 1.;
#endif

#ifdef _EXPORTED_ENABLE_CLIP_RECT
    // Calculate the clip rect before entering the interlock.
    if (_EXPORTED_ENABLE_CLIP_RECT)
    {
        half clipRectCoverage =
            max(min_value(cast_float4_to_half4(v_clipRect)), make_half(.0));
        coverage = min(clipRectCoverage, coverage);
    }
#endif

#ifdef NEEDS_INTERLOCK
    PLS_INTERLOCK_BEGIN;
#endif

#ifdef _EXPORTED_ENABLE_CLIPPING
    if (_EXPORTED_ENABLE_CLIPPING && v_clipID != .0)
    {
        half2 clipData = unpackHalf2x16(PLS_LOADUI(clipBuffer));
        half clipContentID = clipData.y;
        half clipCoverage =
            max(clipContentID == v_clipID ? clipData.x : make_half(.0),
                make_half(.0));
        coverage = min(coverage, clipCoverage);
    }
#endif

#ifdef _EXPORTED_DRAW_IMAGE_MESH
    // Apply opacity after clipping.
    coverage *= imageDrawUniforms.opacity;
#endif

#ifndef _EXPORTED_FIXED_FUNCTION_COLOR_OUTPUT
    half4 dstColorPremul = PLS_LOAD4F(colorBuffer);
#ifdef _EXPORTED_ENABLE_ADVANCED_BLEND
    if (_EXPORTED_ENABLE_ADVANCED_BLEND)
    {
#ifdef _EXPORTED_ATLAS_BLIT
        // GENERATE_PREMULTIPLIED_PAINT_COLORS is false in this case for
        // find_paint_color() because advanced blend needs unmultiplied colors.
        ushort blendMode = cast_half_to_ushort(v_blendMode);
#endif

#ifdef _EXPORTED_DRAW_IMAGE_MESH
        // Unmultiply the image for advanced blend. Images are always
        // premultiplied so that the filtering works correctly.
        // TODO: This unmultiply technically isn't necessary with srcOver blend.
        // We may want to experiment with dynamically not premultiplying here
        // and in find_paint_color() when the blend mode is srcOver.
        color.xyz = unmultiply_rgb(color);
        ushort blendMode = cast_uint_to_ushort(imageDrawUniforms.blendMode);
#endif

        if (blendMode != BLEND_SRC_OVER)
        {
            color.xyz =
                advanced_color_blend(color.xyz, dstColorPremul, blendMode);
        }
        // Premultiply alpha now.
        color.w *= coverage;
        color.xyz *= color.w;
    }
    else
#endif // @ENABLE_ADVANCED_BLEND
    {
        color *= coverage;
    }

    // Certain platforms give us less control of the format of what we are
    // rendering too. Specifically, we are auto converted from linear -> sRGB on
    // render target writes in unreal. In those cases we made need to end up in
    // linear color space
#ifdef _EXPORTED_NEEDS_GAMMA_CORRECTION
    if (_EXPORTED_NEEDS_GAMMA_CORRECTION)
    {
        color = gamma_to_linear(color);
    }
#endif

    PLS_STORE4F(colorBuffer, dstColorPremul * (1. - color.w) + color);
#endif // !@FIXED_FUNCTION_COLOR_OUTPUT

    PLS_PRESERVE_UI(clipBuffer);
    PLS_PRESERVE_UI(coverageBuffer);
#ifdef NEEDS_INTERLOCK
    PLS_INTERLOCK_END;
#endif

#ifdef _EXPORTED_FIXED_FUNCTION_COLOR_OUTPUT
    _fragColor = color * coverage;
#endif

    EMIT_PLS;
}

#endif // @FRAGMENT
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive