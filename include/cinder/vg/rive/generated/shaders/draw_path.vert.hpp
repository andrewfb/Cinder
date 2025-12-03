#pragma once

#include "draw_path.vert.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char draw_path_vert[] = R"===(/*
 * Copyright 2022 Rive
 */

// undef GENERATE_PREMULTIPLIED_PAINT_COLORS first because this file gets
// included multiple times with different defines in the Metal library.
#undef GENERATE_PREMULTIPLIED_PAINT_COLORS
#ifdef _EXPORTED_ENABLE_ADVANCED_BLEND
// If advanced blend is enabled, we generate unmultiplied paint colors in the
// shader. Otherwise we would have to just turn around and unmultiply them in
// order to run the blend equation.
#define GENERATE_PREMULTIPLIED_PAINT_COLORS  !_EXPORTED_ENABLE_ADVANCED_BLEND
#else
// As long as advanced blend is not enabled, it's more efficient for the shader
// to generate premultiplied paint colors from the start.
#define GENERATE_PREMULTIPLIED_PAINT_COLORS  true
#endif

// undef COVERAGE_TYPE first because this file gets included multiple times with
// different defines in the Metal library.
#undef COVERAGE_TYPE
#ifdef _EXPORTED_ENABLE_FEATHER
#define COVERAGE_TYPE  float4
#else
#define COVERAGE_TYPE  half2
#endif

#ifdef _EXPORTED_VERTEX
ATTR_BLOCK_BEGIN(Attrs)
#if defined(_EXPORTED_DRAW_INTERIOR_TRIANGLES) || defined(_EXPORTED_ATLAS_BLIT)
ATTR(0, packed_float3, _EXPORTED_a_triangleVertex);
#else
ATTR(0,
     float4,
     _EXPORTED_a_patchVertexData); // [localVertexID, outset, fillCoverage, vertexType]
ATTR(1, float4, _EXPORTED_a_mirroredVertexData);
#endif
ATTR_BLOCK_END
#endif

VARYING_BLOCK_BEGIN
NO_PERSPECTIVE VARYING(0, float4, v_paint);

#ifdef _EXPORTED_ATLAS_BLIT
NO_PERSPECTIVE VARYING(1, float2, v_atlasCoord);
#elif !defined(_EXPORTED_RENDER_MODE_MSAA)
#ifdef _EXPORTED_DRAW_INTERIOR_TRIANGLES
_EXPORTED_OPTIONALLY_FLAT VARYING(1, half, v_windingWeight);
#else
NO_PERSPECTIVE VARYING(2, COVERAGE_TYPE, v_coverages);
#endif //@DRAW_INTERIOR_TRIANGLES
_EXPORTED_OPTIONALLY_FLAT VARYING(3, half, v_pathID);
#endif // !@RENDER_MODE_MSAA

#ifdef _EXPORTED_ENABLE_CLIPPING
#ifdef _EXPORTED_ATLAS_BLIT
_EXPORTED_OPTIONALLY_FLAT VARYING(4, half, v_clipID); // [clipID, outerClipID]
#else
_EXPORTED_OPTIONALLY_FLAT VARYING(4, half2, v_clipIDs); // [clipID, outerClipID]
#endif
#endif // @ENABLE_CLIPPING
#if defined(_EXPORTED_ENABLE_CLIP_RECT) && !defined(_EXPORTED_RENDER_MODE_MSAA)
NO_PERSPECTIVE VARYING(5, float4, v_clipRect);
#endif
#ifdef _EXPORTED_ENABLE_ADVANCED_BLEND
_EXPORTED_OPTIONALLY_FLAT VARYING(6, half, v_blendMode);
#endif
VARYING_BLOCK_END

#ifdef _EXPORTED_VERTEX
VERTEX_MAIN(_EXPORTED_drawVertexMain, Attrs, attrs, _vertexID, _instanceID)
{
#if defined(_EXPORTED_DRAW_INTERIOR_TRIANGLES) || defined(_EXPORTED_ATLAS_BLIT)
    ATTR_UNPACK(_vertexID, attrs, _EXPORTED_a_triangleVertex, float3);
#else
    ATTR_UNPACK(_vertexID, attrs, _EXPORTED_a_patchVertexData, float4);
    ATTR_UNPACK(_vertexID, attrs, _EXPORTED_a_mirroredVertexData, float4);
#endif

    VARYING_INIT(v_paint, float4);

#ifdef _EXPORTED_ATLAS_BLIT
    VARYING_INIT(v_atlasCoord, float2);
#elif !defined(_EXPORTED_RENDER_MODE_MSAA)
#ifdef _EXPORTED_DRAW_INTERIOR_TRIANGLES
    VARYING_INIT(v_windingWeight, half);
#else
    VARYING_INIT(v_coverages, COVERAGE_TYPE);
#endif //@DRAW_INTERIOR_TRIANGLES
    VARYING_INIT(v_pathID, half);
#endif // !@RENDER_MODE_MSAA

#ifdef _EXPORTED_ENABLE_CLIPPING
#ifdef _EXPORTED_ATLAS_BLIT
    VARYING_INIT(v_clipID, half);
#else
    VARYING_INIT(v_clipIDs, half2);
#endif
#endif // @ENABLE_CLIPPING
#if defined(_EXPORTED_ENABLE_CLIP_RECT) && !defined(_EXPORTED_RENDER_MODE_MSAA)
    VARYING_INIT(v_clipRect, float4);
#endif
#ifdef _EXPORTED_ENABLE_ADVANCED_BLEND
    VARYING_INIT(v_blendMode, half);
#endif

    bool shouldDiscardVertex = false;
    uint pathID;
    float2 vertexPosition;
#ifdef _EXPORTED_RENDER_MODE_MSAA
    ushort pathZIndex;
#endif

#ifdef _EXPORTED_ATLAS_BLIT
    vertexPosition =
        unpack_atlas_coverage_vertex(_EXPORTED_a_triangleVertex,
                                     pathID,
#ifdef _EXPORTED_RENDER_MODE_MSAA
                                     pathZIndex,
#endif
                                     v_atlasCoord VERTEX_CONTEXT_UNPACK);
#elif defined(_EXPORTED_DRAW_INTERIOR_TRIANGLES)
    vertexPosition = unpack_interior_triangle_vertex(_EXPORTED_a_triangleVertex,
                                                     pathID
#ifdef _EXPORTED_RENDER_MODE_MSAA
                                                     ,
                                                     pathZIndex
#else
                                                     ,
                                                     v_windingWeight
#endif
                                                         VERTEX_CONTEXT_UNPACK);
#else // !@DRAW_INTERIOR_TRIANGLES
    float4 coverages;
    shouldDiscardVertex =
        !unpack_tessellated_path_vertex(_EXPORTED_a_patchVertexData,
                                        _EXPORTED_a_mirroredVertexData,
                                        _instanceID,
                                        pathID,
                                        vertexPosition
#ifndef _EXPORTED_RENDER_MODE_MSAA
                                        ,
                                        coverages
#else
                                        ,
                                        pathZIndex
#endif
                                            VERTEX_CONTEXT_UNPACK);
#ifndef _EXPORTED_RENDER_MODE_MSAA
#ifdef _EXPORTED_ENABLE_FEATHER
    v_coverages = coverages;
#else
    v_coverages.xy = cast_float2_to_half2(coverages.xy);
#endif
#endif
#endif // !DRAW_INTERIOR_TRIANGLES

    uint2 paintData = STORAGE_BUFFER_LOAD2(_EXPORTED_paintBuffer, pathID);

#if !defined(_EXPORTED_ATLAS_BLIT) && !defined(_EXPORTED_RENDER_MODE_MSAA)
    // Encode the integral pathID as a "half" that we know the hardware will see
    // as a unique value in the fragment shader.
    v_pathID = id_bits_to_f16(pathID, uniforms.pathIDGranularity);

    // Indicate even-odd fill rule by making pathID negative.
    if ((paintData.x & PAINT_FLAG_EVEN_ODD_FILL) != 0u)
        v_pathID = -v_pathID;
#endif // !@ATLAS_BLIT && !@RENDER_MODE_MSAA

    uint paintType = paintData.x & 0xfu;
#ifdef _EXPORTED_ENABLE_CLIPPING
    if (_EXPORTED_ENABLE_CLIPPING)
    {
        uint clipIDBits =
            (paintType == CLIP_UPDATE_PAINT_TYPE ? paintData.y : paintData.x) >>
            16;
        half clipID = id_bits_to_f16(clipIDBits, uniforms.pathIDGranularity);
        // Negative clipID means to update the clip buffer instead of the color
        // buffer.
        if (paintType == CLIP_UPDATE_PAINT_TYPE)
            clipID = -clipID;
#ifdef _EXPORTED_ATLAS_BLIT
        v_clipID = clipID;
#else
        v_clipIDs.x = clipID;
#endif
    }
#endif
#ifdef _EXPORTED_ENABLE_ADVANCED_BLEND
    if (_EXPORTED_ENABLE_ADVANCED_BLEND)
    {
        v_blendMode = float((paintData.x >> 4) & 0xfu);
    }
#endif

    // Paint matrices operate on the fragment shader's "_fragCoord", which is
    // bottom-up in GL.
    float2 fragCoord = vertexPosition;
#ifdef _EXPORTED_FRAMEBUFFER_BOTTOM_UP
    fragCoord.y = float(uniforms.renderTargetHeight) - fragCoord.y;
#endif

#ifdef _EXPORTED_ENABLE_CLIP_RECT
    if (_EXPORTED_ENABLE_CLIP_RECT)
    {
        // clipRectInverseMatrix transforms from pixel coordinates to a space
        // where the clipRect is the normalized rectangle: [-1, -1, 1, 1].
        float2x2 clipRectInverseMatrix = make_float2x2(
            STORAGE_BUFFER_LOAD4(_EXPORTED_paintAuxBuffer, pathID * 4u + 2u));
        float4 clipRectInverseTranslate =
            STORAGE_BUFFER_LOAD4(_EXPORTED_paintAuxBuffer, pathID * 4u + 3u);
#ifndef _EXPORTED_RENDER_MODE_MSAA
        v_clipRect =
            find_clip_rect_coverage_distances(clipRectInverseMatrix,
                                              clipRectInverseTranslate.xy,
                                              fragCoord);
#else  // !@RENDER_MODE_MSAA => @RENDER_MODE_MSAA
        set_clip_rect_plane_distances(clipRectInverseMatrix,
                                      clipRectInverseTranslate.xy,
                                      fragCoord CLIP_CONTEXT_UNPACK);
#endif // @RENDER_MODE_MSAA
    }
#endif // ENABLE_CLIP_RECT
       // #endif // TARGET_VULKAN

    // Unpack the paint once we have a position.
    if (paintType == SOLID_COLOR_PAINT_TYPE)
    {
        half4 color = unpackUnorm4x8(paintData.y);
        if (GENERATE_PREMULTIPLIED_PAINT_COLORS)
            color.xyz *= color.w;
        v_paint = float4(color);
    }
#if defined(_EXPORTED_ENABLE_CLIPPING) && !defined(_EXPORTED_ATLAS_BLIT)
    else if (_EXPORTED_ENABLE_CLIPPING && paintType == CLIP_UPDATE_PAINT_TYPE)
    {
        half outerClipID =
            id_bits_to_f16(paintData.x >> 16, uniforms.pathIDGranularity);
        v_clipIDs.y = outerClipID;
    }
#endif
    else
    {
        float2x2 paintMatrix =
            make_float2x2(STORAGE_BUFFER_LOAD4(_EXPORTED_paintAuxBuffer, pathID * 4u));
        float4 paintTranslate =
            STORAGE_BUFFER_LOAD4(_EXPORTED_paintAuxBuffer, pathID * 4u + 1u);
        float2 paintCoord = MUL(paintMatrix, fragCoord) + paintTranslate.xy;
        if (paintType == LINEAR_GRADIENT_PAINT_TYPE ||
            paintType == RADIAL_GRADIENT_PAINT_TYPE)
        {
            // v_paint.a contains "-row" of the gradient ramp at texel center,
            // in normalized space.
            v_paint.w = -uintBitsToFloat(paintData.y);
            // abs(v_paint.b) contains either:
            //   - 2 if the gradient ramp spans an entire row.
            //   - x0 of the gradient ramp in normalized space, if it's a simple
            //   2-texel ramp.
            float gradientSpan = paintTranslate.z;
            // gradientSpan is either ~1 (complex gradients span the whole width
            // of the texture minus 1px), or 1/GRAD_TEXTURE_WIDTH (simple
            // gradients span 1px).
            if (gradientSpan > .9)
            {
                // Complex ramps span an entire row. Set it to 2 to convey this.
                v_paint.z = 2.;
            }
            else
            {
                // This is a simple ramp.
                v_paint.z = paintTranslate.w;
            }
            if (paintType == LINEAR_GRADIENT_PAINT_TYPE)
            {
                // The paint is a linear gradient.
                v_paint.y = .0;
                v_paint.x = paintCoord.x;
            }
            else
            {
                // The paint is a radial gradient. Mark v_paint.b negative to
                // indicate this to the fragment shader. (v_paint.b can't be
                // zero because the gradient ramp is aligned on pixel centers,
                // so negating it will always produce a negative number.)
                v_paint.z = -v_paint.z;
                v_paint.xy = paintCoord.xy;
            }
        }
        else // IMAGE_PAINT_TYPE
        {
            // v_paint.a <= -1. signals that the paint is an image.
            // -v_paint.a - 2 is the texture mipmap level-of-detail.
            // v_paint.b is the image opacity.
            // v_paint.rg is the normalized image texture coordinate (built into
            // the paintMatrix).
            float opacity = uintBitsToFloat(paintData.y);
            float lod = paintTranslate.z;
            v_paint = float4(paintCoord.x, paintCoord.y, opacity, -2. - lod);
        }
    }

    float4 pos;
    if (!shouldDiscardVertex)
    {
        pos = RENDER_TARGET_COORD_TO_CLIP_COORD(vertexPosition);
#ifdef _EXPORTED_POST_INVERT_Y
        pos.y = -pos.y;
#endif
#ifdef _EXPORTED_RENDER_MODE_MSAA
        pos.z = normalize_z_index(pathZIndex);
#endif
    }
    else
    {
        pos = float4(uniforms.vertexDiscardValue,
                     uniforms.vertexDiscardValue,
                     uniforms.vertexDiscardValue,
                     uniforms.vertexDiscardValue);
    }

    VARYING_PACK(v_paint);
#ifdef _EXPORTED_ATLAS_BLIT
    VARYING_PACK(v_atlasCoord);
#elif !defined(_EXPORTED_RENDER_MODE_MSAA)
#ifdef _EXPORTED_DRAW_INTERIOR_TRIANGLES
    VARYING_PACK(v_windingWeight);
#else
    VARYING_PACK(v_coverages);
#endif //@DRAW_INTERIOR_TRIANGLES
    VARYING_PACK(v_pathID);
#endif // !@RENDER_MODE_MSAA

#ifdef _EXPORTED_ENABLE_CLIPPING
#ifdef _EXPORTED_ATLAS_BLIT
    VARYING_PACK(v_clipID);
#else
    VARYING_PACK(v_clipIDs);
#endif
#endif // @ENABLE_CLIPPING
#if defined(_EXPORTED_ENABLE_CLIP_RECT) && !defined(_EXPORTED_RENDER_MODE_MSAA)
    VARYING_PACK(v_clipRect);
#endif
#ifdef _EXPORTED_ENABLE_ADVANCED_BLEND
    VARYING_PACK(v_blendMode);
#endif
    EMIT_VERTEX(pos);
}
#endif

#ifdef _EXPORTED_FRAGMENT

FRAG_STORAGE_BUFFER_BLOCK_BEGIN
FRAG_STORAGE_BUFFER_BLOCK_END

// Add a function here for fragments to unpack the paint since we're the ones
// who packed it in the vertex shader.
INLINE half4 find_paint_color(float4 paint,
                              float coverage FRAGMENT_CONTEXT_DECL)
{
    half4 color;
    if (paint.w >= .0) // Is the paint a solid color?
    {
        // The vertex shader will have premultiplied 'paint' (or not) based on
        // GENERATE_PREMULTIPLIED_PAINT_COLORS.
        color = cast_float4_to_half4(paint);
        if (GENERATE_PREMULTIPLIED_PAINT_COLORS)
            color *= coverage;
        else
            color.w *= coverage;
    }
    else if (paint.w > -1.) // Is paint is a gradient (linear or radial)?
    {
        float t =
            paint.z > .0 ? /*linear*/ paint.x : /*radial*/ length(paint.xy);
        t = clamp(t, .0, 1.);
        float span = abs(paint.z);
        float x = span > 1.
                      ? /*entire row*/ (1. - 1. / GRAD_TEXTURE_WIDTH) * t +
                            (.5 / GRAD_TEXTURE_WIDTH)
                      : /*two texels*/ (1. / GRAD_TEXTURE_WIDTH) * t + span;
        float row = -paint.w;
        // Our gradient texture is not mipmapped. Issue a texture-sample that
        // explicitly does not find derivatives for LOD computation.
        color =
            TEXTURE_SAMPLE_LOD(_EXPORTED_gradTexture, gradSampler, float2(x, row), .0);
        color.w *= coverage;
        // Gradients are always unmultiplied so we don't lose color data while
        // doing the hardware filter.
        if (GENERATE_PREMULTIPLIED_PAINT_COLORS)
            color.xyz *= color.w;
    }
    else // The paint is an image.
    {
        half lod = -paint.w - 2.;
        color = TEXTURE_SAMPLE_DYNAMIC_LOD(_EXPORTED_imageTexture,
                                           imageSampler,
                                           paint.xy,
                                           lod);
        half opacity = paint.z * coverage;
        // Images are always premultiplied so the (transparent) background color
        // doesn't bleed into the edges during the hardware filter.
        if (GENERATE_PREMULTIPLIED_PAINT_COLORS)
            color *= opacity;
        else
            color = make_half4(unmultiply_rgb(color), color.w * opacity);
    }
    return color;
}

#if !defined(_EXPORTED_DRAW_INTERIOR_TRIANGLES) && !defined(_EXPORTED_ATLAS_BLIT)

// Add functions here for fragments to unpack and evaluate coverage since we're
// the ones who packed the coverage components in the vertex shader.
INLINE half find_stroke_coverage(COVERAGE_TYPE coverages TEXTURE_CONTEXT_DECL)
{
#ifdef _EXPORTED_ENABLE_FEATHER
    if (_EXPORTED_ENABLE_FEATHER && is_feathered_stroke(coverages))
        return eval_feathered_stroke(coverages TEXTURE_CONTEXT_FORWARD);
    else
#endif // @ENABLE_FEATHER
        return min(coverages.x, coverages.y);
}

INLINE half find_fill_coverage(COVERAGE_TYPE coverages TEXTURE_CONTEXT_DECL)
{
#if defined(_EXPORTED_ENABLE_FEATHER)
    if (_EXPORTED_ENABLE_FEATHER && is_feathered_fill(coverages))
        return eval_feathered_fill(coverages TEXTURE_CONTEXT_FORWARD);
    else
#endif // @ENABLE_FEATHER
        return coverages.x;
}

INLINE half find_frag_coverage(COVERAGE_TYPE coverages TEXTURE_CONTEXT_DECL)
{
    if (is_stroke(coverages))
        return find_stroke_coverage(coverages TEXTURE_CONTEXT_FORWARD);
    else // Fill. (Back-face culling handles the sign of coverages.x.)
        return find_fill_coverage(coverages TEXTURE_CONTEXT_FORWARD);
}
)===" R"===(
INLINE half apply_frag_coverage(half initialCoverage,
                                COVERAGE_TYPE coverages TEXTURE_CONTEXT_DECL)
{
    if (is_stroke(coverages))
    {
        half fragCoverage =
            find_stroke_coverage(coverages TEXTURE_CONTEXT_FORWARD);
        return max(fragCoverage, initialCoverage);
    }
    else // Fill. (Back-face culling handles the sign of coverages.x.)
    {
        half fragCoverage =
            find_fill_coverage(coverages TEXTURE_CONTEXT_FORWARD);
        return initialCoverage + fragCoverage;
    }
}

#endif // !@DRAW_INTERIOR_TRIANGLES && !@ATLAS_BLIT

#endif // @FRAGMENT
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive