#pragma once

#include "copy_attachment_to_attachment.frag.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char copy_attachment_to_attachment_frag[] = R"===(#ifdef GB
layout(input_attachment_index=0,binding=we,set=q4)uniform lowp subpassInput ng;layout(location=0)out i ya;void main(){ya=subpassLoad(ng);}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive