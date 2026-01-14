#pragma once

#include "specialization.glsl.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char specialization[] = R"===(#ifndef SD
layout(constant_id=ye)const bool Gf=true;layout(constant_id=ze)const bool Hf=true;layout(constant_id=Ae)const bool If=true;layout(constant_id=Be)const bool Jf=true;layout(constant_id=Ce)const bool Kf=true;layout(constant_id=De)const bool Lf=true;layout(constant_id=Ee)const bool Mf=true;layout(constant_id=Fe)const bool Nf=true;layout(constant_id=Ge)const bool Of=true;layout(constant_id=He)const uint Pf=0;
#define Q Gf
#define DB Hf
#define HB If
#define IB Jf
#define IC Kf
#define OC Lf
#define WB Mf
#define YC Nf
#define OB Of
#define SC Pf
#else
#define Q true
#define DB true
#define HB true
#define IB true
#define IC true
#define OC true
#define WB true
#define YC true
#define OB true
#define SC 0
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive