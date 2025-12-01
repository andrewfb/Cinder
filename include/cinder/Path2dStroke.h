/*
 Copyright (c) 2024, The Cinder Project, All rights reserved.

 Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and
	the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
	the following disclaimer in the documentation and/or other materials provided with the distribution.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.
*/

#pragma once

#include "cinder/Path2d.h"
#include "cinder/Shape2d.h"
#include <vector>

namespace cinder {

//=============================================================================
// Style Parameters
//=============================================================================

/// Join style for corners (used by both stroke and offset)
enum class Join {
	Bevel,  ///< Straight line connecting segments
	Miter,  ///< Extend segments to natural intersection
	Round   ///< Arc between segments
};

/// Cap style for stroke endpoints
enum class Cap {
	Butt,   ///< Flat cap
	Square, ///< Square cap extending half stroke width
	Round   ///< Rounded cap with radius = half stroke width
};

// Backwards compatibility aliases
using StrokeJoin = Join;
using StrokeCap = Cap;

/// Visual style for a stroke
struct CI_API StrokeStyle {
	float width = 1.0f;
	Join join = Join::Round;
	float miterLimit = 4.0f;
	Cap startCap = Cap::Round;
	Cap endCap = Cap::Round;
	std::vector<float> dashPattern;
	float dashOffset = 0.0f;

	StrokeStyle() = default;
	explicit StrokeStyle( float w ) : width( w ) {}

	StrokeStyle& withJoin( Join j ) { join = j; return *this; }
	StrokeStyle& withMiterLimit( float limit ) { miterLimit = limit; return *this; }
	StrokeStyle& withStartCap( Cap c ) { startCap = c; return *this; }
	StrokeStyle& withEndCap( Cap c ) { endCap = c; return *this; }
	StrokeStyle& withCaps( Cap c ) { startCap = endCap = c; return *this; }
	StrokeStyle& withDashes( float offset, const std::vector<float>& pattern ) {
		dashOffset = offset;
		dashPattern = pattern;
		return *this;
	}
};

//=============================================================================
// Path Offsetting
//=============================================================================

/// Offset a Path2d by a signed distance. Positive values offset outward (to the left
/// when traversing the path), negative values offset inward.
/// @param path Input path to offset
/// @param distance Signed offset distance
/// @param join Join style for corners
/// @param miterLimit Miter limit (ratio of miter length to offset distance)
/// @param tolerance Approximation tolerance (smaller = more accurate, larger = faster)
/// @param removeSelfIntersections If true, remove self-intersecting loops that occur at sharp corners
/// @return Offset path as a Shape2d (may contain multiple contours for paths with cusps)
CI_API Shape2d offset( const Path2d& path, float distance, Join join,
                       float miterLimit, float tolerance = 0.25f,
                       bool removeSelfIntersections = false );

/// Offset a Path2d with default join style (Round)
CI_API inline Shape2d offset( const Path2d& path, float distance, float tolerance = 0.25f,
                               bool removeSelfIntersections = false ) {
	return offset( path, distance, Join::Round, 4.0f, tolerance, removeSelfIntersections );
}

/// Offset a Shape2d by a signed distance.
/// @param shape Input shape to offset
/// @param distance Signed offset distance
/// @param join Join style for corners
/// @param miterLimit Miter limit (ratio of miter length to offset distance)
/// @param tolerance Approximation tolerance
/// @param removeSelfIntersections If true, remove self-intersecting loops that occur at sharp corners
/// @return Offset shape
CI_API Shape2d offset( const Shape2d& shape, float distance, Join join,
                       float miterLimit, float tolerance = 0.25f,
                       bool removeSelfIntersections = false );

/// Offset a Shape2d with default join style (Round)
CI_API inline Shape2d offset( const Shape2d& shape, float distance, float tolerance = 0.25f,
                               bool removeSelfIntersections = false ) {
	return offset( shape, distance, Join::Round, 4.0f, tolerance, removeSelfIntersections );
}

//=============================================================================
// Path Stroking
//=============================================================================

/// Expand a Path2d stroke into a filled Shape2d
/// @param path Input path to stroke
/// @param style Stroke style parameters
/// @param tolerance Approximation tolerance
/// @return The expanded stroke as a filled Shape2d
CI_API Shape2d stroke( const Path2d& path, const StrokeStyle& style, float tolerance = 0.25f );

/// Expand a Path2d stroke with simple parameters
/// @param path Input path to stroke
/// @param width Stroke width
/// @param tolerance Approximation tolerance
/// @return The expanded stroke as a filled Shape2d
CI_API Shape2d stroke( const Path2d& path, float width, float tolerance = 0.25f );

/// Expand a Shape2d stroke into a filled Shape2d
/// @param shape Input shape to stroke
/// @param style Stroke style parameters
/// @param tolerance Approximation tolerance
/// @return The expanded stroke as a filled Shape2d
CI_API Shape2d stroke( const Shape2d& shape, const StrokeStyle& style, float tolerance = 0.25f );

/// Expand a Shape2d stroke with simple parameters
/// @param shape Input shape to stroke
/// @param width Stroke width
/// @param tolerance Approximation tolerance
/// @return The expanded stroke as a filled Shape2d
CI_API Shape2d stroke( const Shape2d& shape, float width, float tolerance = 0.25f );

//=============================================================================
// Dashing
//=============================================================================

/// Apply a dash pattern to a Path2d
/// @param path Input path
/// @param offset Offset into the dash pattern
/// @param pattern Alternating on/off dash lengths
/// @return Dashed path as a Shape2d (multiple contours for each dash segment)
CI_API Shape2d dash( const Path2d& path, float offset, const std::vector<float>& pattern );

/// Apply a dash pattern to a Shape2d
/// @param shape Input shape
/// @param offset Offset into the dash pattern
/// @param pattern Alternating on/off dash lengths
/// @return Dashed shape
CI_API Shape2d dash( const Shape2d& shape, float offset, const std::vector<float>& pattern );

} // namespace cinder
