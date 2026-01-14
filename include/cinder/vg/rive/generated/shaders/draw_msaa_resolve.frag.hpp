#pragma once

#include "draw_msaa_resolve.frag.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char draw_msaa_resolve_frag[] = R"===(#ifdef GB
layout(input_attachment_index=0,binding=T3,set=q4)uniform lowp subpassInputMS L8;layout(location=0)out i ya;void main(){ya=(subpassLoad(L8,0)+subpassLoad(L8,1)+subpassLoad(L8,2)+subpassLoad(L8,3))*.25;}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive