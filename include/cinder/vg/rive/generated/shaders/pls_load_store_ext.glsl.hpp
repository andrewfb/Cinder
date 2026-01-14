#pragma once

#include "pls_load_store_ext.glsl.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char pls_load_store_ext[] = R"===(#ifdef BB
void main(){gl_Position=vec4(mix(vec2(-1,1),vec2(1,-1),equal(gl_VertexID&ivec2(1,2),ivec2(0))),0,1);
#ifdef JC
gl_Position.y=-gl_Position.y;
#endif
}
#endif
#ifdef GB
#extension GL_EXT_shader_pixel_local_storage:require
#ifdef GL_ARM_shader_framebuffer_fetch
#extension GL_ARM_shader_framebuffer_fetch:require
#else
#extension GL_EXT_shader_framebuffer_fetch:require
#endif
#ifdef FE
#if __VERSION__>=310
layout(binding=0,std140)uniform dh{uniform highp vec4 Af;}Bf;
#else
uniform mediump vec4 GE;
#endif
#endif
#ifdef GL_EXT_shader_pixel_local_storage
#ifdef LD
__pixel_local_inEXT m1
#else
__pixel_local_outEXT m1
#endif
{layout(rgba8)mediump vec4 z0;layout(r32ui)highp uint M0;layout(rgba8)mediump vec4 H2;layout(r32ui)highp uint j7;};
#ifndef GL_ARM_shader_framebuffer_fetch
#ifdef HE
layout(location=0)inout mediump vec4 ma;
#endif
#endif
#ifdef LD
layout(location=0)out mediump vec4 ma;
#endif
void main(){
#ifdef FE
#if __VERSION__>=310
z0=Bf.Af;
#else
z0=GE;
#endif
#endif
#ifdef HE
#ifdef GL_ARM_shader_framebuffer_fetch
z0=gl_LastFragColorARM;
#else
z0=ma;
#endif
#endif
#ifdef MD
j7=0u;
#endif
#ifdef DF
M0=0u;
#endif
#ifdef LD
ma=z0;
#endif
}
#else
layout(location=0)out mediump vec4 Cf;void main(){Cf=vec4(0,1,0,1);}
#endif
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive