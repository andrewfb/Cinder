#pragma once

#include "draw_input_attachment.frag.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char draw_input_attachment_frag[] = R"===(#ifdef GB
layout(input_attachment_index=0,
#ifdef ME
binding=ME,
#else
binding=0,
#endif
set=f4)uniform lowp subpassInput ng;layout(location=0)out i sa;void main(){sa=subpassLoad(ng);}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive