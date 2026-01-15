#pragma once

#include "pls_load_store_ext.glsl.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char pls_load_store_ext[] = R"===(#ifdef BB
void main(){gl_Position=vec4(mix(vec2(-1,1),vec2(1,-1),equal(gl_VertexID&ivec2(1,2),ivec2(0))),0,1);
#ifdef IC
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
layout(binding=0,std140)uniform bh{uniform highp vec4 rf;}sf;
#else
uniform mediump vec4 GE;
#endif
#endif
#ifdef GL_EXT_shader_pixel_local_storage
#ifdef LD
__pixel_local_inEXT g1
#else
__pixel_local_outEXT g1
#endif
{layout(rgba8)mediump vec4 x0;layout(r32ui)highp uint K0;layout(rgba8)mediump vec4 B2;layout(r32ui)highp uint f7;};
#ifndef GL_ARM_shader_framebuffer_fetch
#ifdef HE
layout(location=0)inout mediump vec4 ca;
#endif
#endif
#ifdef LD
layout(location=0)out mediump vec4 ca;
#endif
void main(){
#ifdef FE
#if __VERSION__>=310
x0=sf.rf;
#else
x0=GE;
#endif
#endif
#ifdef HE
#ifdef GL_ARM_shader_framebuffer_fetch
x0=gl_LastFragColorARM;
#else
x0=ca;
#endif
#endif
#ifdef MD
f7=0u;
#endif
#ifdef EF
K0=0u;
#endif
#ifdef LD
ca=x0;
#endif
}
#else
layout(location=0)out mediump vec4 tf;void main(){tf=vec4(0,1,0,1);}
#endif
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive