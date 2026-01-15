#pragma once

#include "draw_msaa_resolve.frag.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char draw_msaa_resolve_frag[] = R"===(#ifdef GB
layout(input_attachment_index=0,binding=J3,set=f4)uniform lowp subpassInputMS E8;layout(location=0)out i sa;void main(){sa=(subpassLoad(E8,0)+subpassLoad(E8,1)+subpassLoad(E8,2)+subpassLoad(E8,3))*.25;}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive