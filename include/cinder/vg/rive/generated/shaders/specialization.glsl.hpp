#pragma once

#include "specialization.glsl.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char specialization[] = R"===(#ifndef SD
layout(constant_id=we)const bool xf=true;layout(constant_id=xe)const bool yf=true;layout(constant_id=ye)const bool zf=true;layout(constant_id=ze)const bool Af=true;layout(constant_id=Ae)const bool Bf=true;layout(constant_id=Be)const bool Cf=true;layout(constant_id=Ce)const bool Df=true;layout(constant_id=De)const bool Ef=true;layout(constant_id=Ee)const bool Ff=true;layout(constant_id=Fe)const uint Gf=0;
#define L xf
#define Z yf
#define FB zf
#define HB Af
#define HC Bf
#define NC Cf
#define RB Df
#define WC Ef
#define JB Ff
#define SC Gf
#else
#define L true
#define Z true
#define FB true
#define HB true
#define HC true
#define NC true
#define RB true
#define WC true
#define JB true
#define SC 0
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive