#pragma once

#include <vector>

#include "cinder/Path2d.h"

void findZeroes( const cinder::Shape2d &shape, const cinder::vec2 &pt, std::vector<cinder::vec2> *result );
void findZeroes( const cinder::Path2d &path, const cinder::vec2 &pt, std::vector<cinder::vec2> *result );
