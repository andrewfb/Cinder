#pragma once

#include "draw_fullscreen_quad.vert.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char draw_fullscreen_quad_vert[] = R"===(/*
 * Copyright 2025 Rive
 */

#ifdef _EXPORTED_VERTEX
void main()
{
    // Fill the entire screen. The caller will use a scissor test to control the
    // bounds being drawn.
    gl_Position.x = (gl_VertexID & 1) == 0 ? -1. : 1.;
    gl_Position.y = (gl_VertexID & 2) == 0 ? -1. : 1.;
    gl_Position.z = 0.;
    gl_Position.w = 1.;
}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive