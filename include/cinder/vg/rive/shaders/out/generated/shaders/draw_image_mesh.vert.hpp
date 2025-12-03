#pragma once

#include "draw_image_mesh.vert.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char draw_image_mesh_vert[] = R"===(/*
 * Copyright 2023 Rive
 */

#ifdef _EXPORTED_VERTEX
ATTR_BLOCK_BEGIN(PositionAttr)
ATTR(0, float2, _EXPORTED_a_position);
ATTR_BLOCK_END

ATTR_BLOCK_BEGIN(UVAttr)
ATTR(1, float2, _EXPORTED_a_texCoord);
ATTR_BLOCK_END
#endif

VARYING_BLOCK_BEGIN
NO_PERSPECTIVE VARYING(0, float2, v_texCoord);
#ifdef _EXPORTED_ENABLE_CLIPPING
_EXPORTED_OPTIONALLY_FLAT VARYING(1, half, v_clipID);
#endif
#if defined(_EXPORTED_ENABLE_CLIP_RECT) && !defined(_EXPORTED_RENDER_MODE_MSAA)
NO_PERSPECTIVE VARYING(2, float4, v_clipRect);
#endif
VARYING_BLOCK_END

#ifdef _EXPORTED_VERTEX
VERTEX_TEXTURE_BLOCK_BEGIN
VERTEX_TEXTURE_BLOCK_END

IMAGE_MESH_VERTEX_MAIN(_EXPORTED_drawVertexMain,
                       PositionAttr,
                       position,
                       UVAttr,
                       uv,
                       _vertexID)
{
    ATTR_UNPACK(_vertexID, position, _EXPORTED_a_position, float2);
    ATTR_UNPACK(_vertexID, uv, _EXPORTED_a_texCoord, float2);

    VARYING_INIT(v_texCoord, float2);
#ifdef _EXPORTED_ENABLE_CLIPPING
    VARYING_INIT(v_clipID, half);
#endif
#if defined(_EXPORTED_ENABLE_CLIP_RECT) && !defined(_EXPORTED_RENDER_MODE_MSAA)
    VARYING_INIT(v_clipRect, float4);
#endif

    float2 vertexPosition =
        MUL(make_float2x2(imageDrawUniforms.viewMatrix), _EXPORTED_a_position) +
        imageDrawUniforms.translate;
    v_texCoord = _EXPORTED_a_texCoord;
#ifdef _EXPORTED_ENABLE_CLIPPING
    if (_EXPORTED_ENABLE_CLIPPING)
    {
        v_clipID = id_bits_to_f16(imageDrawUniforms.clipID,
                                  uniforms.pathIDGranularity);
    }
#endif
#ifdef _EXPORTED_ENABLE_CLIP_RECT
    if (_EXPORTED_ENABLE_CLIP_RECT)
    {
#ifndef _EXPORTED_RENDER_MODE_MSAA
        v_clipRect = find_clip_rect_coverage_distances(
            make_float2x2(imageDrawUniforms.clipRectInverseMatrix),
            imageDrawUniforms.clipRectInverseTranslate,
            vertexPosition CLIP_CONTEXT_UNPACK);
#else
        set_clip_rect_plane_distances(
            make_float2x2(imageDrawUniforms.clipRectInverseMatrix),
            imageDrawUniforms.clipRectInverseTranslate,
            vertexPosition CLIP_CONTEXT_UNPACK);
#endif
    }
#endif // ENABLE_CLIP_RECT
    float4 pos = RENDER_TARGET_COORD_TO_CLIP_COORD(vertexPosition);
#ifdef _EXPORTED_POST_INVERT_Y
    pos.y = -pos.y;
#endif
#ifdef _EXPORTED_RENDER_MODE_MSAA
    pos.z = normalize_z_index(imageDrawUniforms.zIndex);
#endif

    VARYING_PACK(v_texCoord);
#ifdef _EXPORTED_ENABLE_CLIPPING
    VARYING_PACK(v_clipID);
#endif
#if defined(_EXPORTED_ENABLE_CLIP_RECT) && !defined(_EXPORTED_RENDER_MODE_MSAA)
    VARYING_PACK(v_clipRect);
#endif
    EMIT_VERTEX(pos);
}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive