#include "ZeroCrossings2.h"

using namespace cinder;
using namespace std;

int windingLine( const vec2 points[2], const vec2 &test, int *onCurveCount );
int windingQuad( const vec2 points[], const vec2 &test, int *onCurveCount );
int windingCubic( const vec2 pts[], const vec2 &test, int *onCurveCount );

inline int signAsInt( float x ) { return x < 0 ? -1 : (x > 0); }
inline bool between( float a, float b, float c ) { return (a - b) * (c - b) <= 0; }
bool isMonoQuad( float y0, float y1, float y2 )
{
	if( y0 == y1 )
		return true;
	if( y0 < y1 )
		return y1 <= y2;
	else
		return y1 >= y2;
}
int isNotMonotonic( float a, float b, float c )
{
    float ab = a - b;
    float bc = b - c;
    if( ab < 0 ) {
        bc = -bc;
    }
    return ab == 0 || bc < 0;
}
int validUnitDivide( float numer, float denom, float* ratio ) {
    if( numer < 0 ) {
        numer = -numer;
        denom = -denom;
    }

    if( denom == 0 || numer == 0 || numer >= denom ) {
        return 0;
    }

    float r = numer / denom;
    if( std::isnan( r ) ) {
        return 0;
    }
    if( r == 0 ) { // catch underflow if numer <<<< denom
        return 0;
    }
    *ratio = r;
    return 1;
}
inline vec2 interp( const vec2& v0, const vec2& v1, const vec2& t)
{
    return v0 + (v1 - v0) * t;
}
void chopQuadAt( const vec2 src[3], vec2 dst[5], float t )
{
	vec2 p0 = src[0];
	vec2 p1 = src[1];
	vec2 p2 = src[2];
	vec2 tt(t);

	vec2 p01 = interp( p0, p1, tt );
	vec2 p12 = interp( p1, p2, tt );

	dst[0] = p0;
	dst[1] = p01;
	dst[2] = interp( p01, p12, tt );
	dst[3] = p12;
	dst[4] = p2;
}
/** From Numerical Recipes in C.

    Q = -1/2 (B + sign(B) sqrt[B*B - 4*A*C])
    x1 = Q / A
    x2 = C / Q
*/
int findUnitQuadRoots( float A, float B, float C, float roots[2] )
{
    if( A == 0 ) {
        return validUnitDivide( -C, B, roots );
    }

    float* r = roots;

    float R = B*B - 4*A*C;
    if( R < 0 || ! std::isfinite( R ) ) {  // complex roots
        // if R is infinite, it's possible that it may still produce
        // useful results if the operation was repeated in doubles
        // the flipside is determining if the more precise answer
        // isn't useful because surrounding machinery (e.g., subtracting
        // the axis offset from C) already discards the extra precision
        // more investigation and unit tests required...
        return 0;
    }
    R = sqrtf( R );

    float Q = (B < 0) ? -(B-R)/2 : -(B+R)/2;
    r += validUnitDivide(Q, A, r);
    r += validUnitDivide(C, Q, r);
    if( r - roots == 2 ) {
        if( roots[0] > roots[1] )
            std::swap( roots[0], roots[1] );
        else if( roots[0] == roots[1] )  // nearly-equal?
            r -= 1; // skip the double root
    }
    return (int)(r - roots);
}
inline void flattenDoubleQuadExtrema( float coords[14] )
{
    coords[2] = coords[6] = coords[4];
}
inline void flattenDoubleCubicExtrema( float coords[14] )
{
    coords[4] = coords[8] = coords[6];
}
inline float scalarInterp( float A, float B, float t)
{
    return A + (B - A) * t;
}
template<size_t N>
void findMinMaxX( const vec2 pts[], float* minPtr, float* maxPtr) {
    float minX, maxX;
    minX = maxX = pts[0].x;
    for( size_t i = 1; i < N; ++i ) {
        minX = std::min( minX, pts[i].x );
        maxX = std::max( maxX, pts[i].x );
    }
    *minPtr = minX;
    *maxPtr = maxX;
}

void chopCubicAt( const vec2 src[4], vec2 dst[7], float t )
{
	vec2    p0 = src[0];
	vec2    p1 = src[1];
	vec2    p2 = src[2];
	vec2    p3 = src[3];
	vec2    tt( t );

	vec2    ab = interp( p0, p1, tt );
	vec2    bc = interp( p1, p2, tt );
	vec2    cd = interp( p2, p3, tt );
	vec2    abc = interp( ab, bc, tt );
	vec2    bcd = interp( bc, cd, tt );
	vec2    abcd = interp( abc, bcd, tt );

	dst[0] = src[0];
	dst[1] = ab;
	dst[2] = abc;
	dst[3] = abcd;
	dst[4] = bcd;
	dst[5] = cd;
	dst[6] = src[3];
}
void chopCubicAt( const vec2 src[4], vec2 dst[], const float tValues[], int roots )
{
	if( roots == 0 ) { // nothing to chop
		memcpy(dst, src, 4*sizeof(vec2));
	}
	else {
		float t = tValues[0];
		vec2 tmp[4];

		for( int i = 0; i < roots; i++ ) {
			chopCubicAt( src, dst, t );
			if( i == roots - 1 ) {
				break;
			}

			dst += 3;
			// have src point to the remaining cubic (after the chop)
			memcpy( tmp, dst, 4 * sizeof(vec2) );
			src = tmp;

			// watch out in case the renormalized t isn't in range
			if( ! validUnitDivide( tValues[i+1] - tValues[i], 1.0f - tValues[i], &t ) ) {
				// if we can't, just create a degenerate cubic
				dst[4] = dst[5] = dst[6] = src[3];
				break;
			}
		}
	}
}
bool chopMonoAtY( const vec2 pts[4], float y, float* t )
{
	float ycrv[4];
	ycrv[0] = pts[0].y - y;
	ycrv[1] = pts[1].y - y;
	ycrv[2] = pts[2].y - y;
	ycrv[3] = pts[3].y - y;

	// Initial guess.
	float t1 = ycrv[0] / (ycrv[0] - ycrv[3]);

	// Newton's iterations.
	const float tol = 1.0f / 16384;  // This leaves 2 fixed noise bits.
	float t0;
	const int maxiters = 5;
	int iters = 0;
	bool converged;
	do {
		t0 = t1;
		float y01   = scalarInterp( ycrv[0], ycrv[1], t0 );
		float y12   = scalarInterp( ycrv[1], ycrv[2], t0 );
		float y23   = scalarInterp( ycrv[2], ycrv[3], t0 );
		float y012  = scalarInterp( y01,  y12,  t0 );
		float y123  = scalarInterp( y12,  y23,  t0 );
		float y0123 = scalarInterp( y012, y123, t0 );
		float yder  = (y123 - y012) * 3;
		// TODO(turk): check for yder==0: horizontal.
		t1 -= y0123 / yder;
		converged = fabsf(t1 - t0) <= tol;  // NaN-safe
		++iters;
	} while( ! converged && (iters < maxiters) );
	*t = t1;                  // Return the result.

	// The result might be valid, even if outside of the range [0, 1], but
	// we never evaluate a Bezier outside this interval, so we return false.
	if( t1 < 0 || t1 > 1.0f ) 
		return false;         // This shouldn't happen, but check anyway.
	return converged;
}
/** Cubic'(t) = At^2 + Bt + C, where
    A = 3(-a + 3(b - c) + d)
    B = 6(a - 2b + c)
    C = 3(b - a)
    Solve for t, keeping only those that fit betwee 0 < t < 1
*/
int findCubicExtrema( float a, float b, float c, float d, float tValues[2] )
{
    // we divide A,B,C by 3 to simplify
	float A = d - a + 3*(b - c);
	float B = 2*(a - b - b + c);
	float C = b - a;

    return findUnitQuadRoots( A, B, C, tValues );
}

/** Given 4 points on a cubic bezier, chop it into 1, 2, 3 beziers such that
    the resulting beziers are monotonic in Y. This is called by the scan
    converter.  Depending on what is returned, dst[] is treated as follows:
    0   dst[0..3] is the original cubic
    1   dst[0..3] and dst[3..6] are the two new cubics
    2   dst[0..3], dst[3..6], dst[6..9] are the three new cubics
    If dst == null, it is ignored and only the count is returned.
*/
int chopCubicAtYExtrema( const vec2 src[4], vec2 dst[10] )
{
	float tValues[2];
	int roots = findCubicExtrema( src[0].y, src[1].y, src[2].y, src[3].y, tValues );

	chopCubicAt( src, dst, tValues, roots );
	if( dst && roots > 0 ) {
		// we do some cleanup to ensure our Y extrema are flat
		flattenDoubleCubicExtrema( &dst[0].y );
		if( roots == 2 ) {
			flattenDoubleCubicExtrema( &dst[3].y );
		}
	}
	return roots;
}

/*  Returns 0 for 1 quad, and 1 for two quads, either way the answer is
 stored in dst[]. Guarantees that the 1/2 quads will be monotonic.
 */
int chopQuadAtYExtrema( const vec2 src[3], vec2 dst[5] )
{
	float a = src[0].y;
	float b = src[1].y;
	float c = src[2].y;

	if( isNotMonotonic( a, b, c ) ) {
		float tValue;
		if( validUnitDivide( a - b, a - b - b + c, &tValue ) ) {
			chopQuadAt( src, dst, tValue );
			flattenDoubleQuadExtrema( &dst[0].y );
			return 1;
		}
		// if we get here, we need to force dst to be monotonic, even though
		// we couldn't compute a unit_divide value (probably underflow).
		b = fabsf(a - b) < fabsf(b - c) ? a : c;
	}
	
	dst[0] = { src[0].x, a };
	dst[1] = { src[1].x, b };
	dst[2] = { src[2].x, c };
	return 0;
}
float evalCubicCoeff( float A, float B, float C, float D, float t )
{
    return ((A * t + B) * t + C) * t + D;
}
float evalCubicPts( float c0, float c1, float c2, float c3, float t )
{
	float A = c3 + 3*(c1 - c2) - c0;
	float B = 3*(c2 - c1 - c1 + c0);
	float C = 3*(c1 - c0);
	float D = c0;
	return evalCubicCoeff( A, B, C, D, t );
}

bool checkOnCurve( const vec2 &test, const vec2& start, const vec2& end )
{
    if( start.y == end.y ) {
        return between( start.x, test.x, end.x ) && test.x != end.x;
    }
	else {
        return test.x == start.x && test.y == start.y;
    }
}

bool contains2( const ci::Shape2d &shape, const ci::vec2 &pt )
{
	int numPathsInside = 0;
	for( auto &cont : shape.getContours() ) {
		if( contains2( cont, pt ) )
			numPathsInside++;
	}
	
	return ( numPathsInside % 2 ) == 1;
}

bool contains2( const ci::Path2d &path, const ci::vec2 &pt )
{
bool evenOddFill = true;
	auto points = path.getPoints();

	int w = 0;
	int onCurveCount = 0;
	
	size_t firstPoint = 0;
	for( size_t s = 0; s < path.getSegments().size(); ++s ) {
		switch( path.getSegmentType( s ) ) {
			case Path2d::LINETO:
				w += windingLine( &points[firstPoint], pt, &onCurveCount );
			break;
			case Path2d::QUADTO:
				w += windingQuad( &points[firstPoint], pt, &onCurveCount );
			break;
			case Path2d::CUBICTO:
				w += windingCubic( &(points[firstPoint]), pt, &onCurveCount );
			break;
//			case Path2d::CLOSE: // ignore - we always assume closed
//			break;
//			default:
//				;//throw Path2dExc();
		}
		
		firstPoint += Path2d::sSegmentTypePointCounts[path.getSegments()[s]];
	}
	
	// handle close
	vec2 temp[2] = { path.getPoint( path.getNumPoints() - 1 ), path.getPoint( 0 ) };
	w += windingLine( temp, pt, &onCurveCount );

	if( evenOddFill ) {
		w &= 1;
	}
	if( w ) {
		return true;
	}
	if (onCurveCount <= 1) {
		return onCurveCount > 0;//SkToBool(onCurveCount) ^ isInverse;
	}
	if ((onCurveCount & 1) || evenOddFill) {
		return (onCurveCount & 1) > 0;
	}
	return false;
}

int windingLine( const vec2 points[2], const vec2 &test, int *onCurveCount )
{
	float x0 = points[0].x;
	float y0 = points[0].y;
	float x1 = points[1].x;
	float y1 = points[1].y;

	float dy = y1 - y0;

	int dir = 1;
	if( y0 > y1 ) {
		std::swap( y0, y1 );
		dir = -1;
	}
	if( test.y < y0 || test.y > y1 ) {
		return 0;
	}
	if( checkOnCurve( test, points[0], points[1] ) ) {
		*onCurveCount += 1;
		return 0;
	}
	if( test.y == y1 ) {
		return 0;
	}
	float cross = (x1 - x0) * (test.y - points[0].y) - dy * ( test.x - x0 );

	if( ! cross ) {
		// zero cross means the point is on the line, and since the case where
		// y of the query point is at the end point is handled above, we can be
		// sure that we're on the line (excluding the end point) here
		if( test.x != x1 || test.y != points[1].y ) {
			*onCurveCount += 1;
		}
		dir = 0;
	}
	else if( signAsInt(cross) == dir ) {
		dir = 0;
	}

	return dir;
}

int windingMonoQuad( const vec2 pts[], const vec2 &test, int* onCurveCount )
{
	float y0 = pts[0].y;
	float y2 = pts[2].y;

	int dir = 1;
	if( y0 > y2 ) {
		std::swap( y0, y2 );
		dir = -1;
	}
	if( test.y < y0 || test.y > y2 ) {
		return 0;
	}
	if( checkOnCurve( test, pts[0], pts[2] ) ) {
		*onCurveCount += 1;
		return 0;
	}
	if( test.y == y2 ) {
		return 0;
	}

	float roots[2];
	int n = findUnitQuadRoots( pts[0].y - 2 * pts[1].y + pts[2].y,
				2 * (pts[1].y - pts[0].y),
				pts[0].y - test.y,
				roots);
	
	float xt;
	if( 0 == n ) {
		// zero roots are returned only when y0 == y
		// Need [0] if dir == 1
		// and  [2] if dir == -1
		xt = pts[1 - dir].x;
	}
	else {
		float t = roots[0];
		float C = pts[0].x;
		float A = pts[2].x - 2 * pts[1].x + C;
		float B = 2 * (pts[1].x - C);
		xt = (A * t + B) * t + C;
	}
	if( fabs( xt - test.x ) < ( 1.0f / ( 1 << 12 ) ) ) {
		if( test.x != pts[2].x || test.y != pts[2].y ) {  // don't test end points; they're start points
			*onCurveCount += 1;
			return 0;
		}
	}
	return xt < test.x ? dir : 0;
}

int windingQuad( const vec2 points[], const vec2 &test, int *onCurveCount )
{
	vec2 dst[5];
	int n = 0;

	if( ! isMonoQuad( points[0].y, points[1].y, points[2].y ) ) {
		n = chopQuadAtYExtrema( points, dst );
		points = dst;
	}
	int w = windingMonoQuad( points, test, onCurveCount );
	if( n > 0 ) {
		w += windingMonoQuad( &points[2], test, onCurveCount );
	}

	return w;
}

int windingMonoCubic( const vec2 pts[], const vec2 &test, int *onCurveCount )
{
	float y0 = pts[0].y;
	float y3 = pts[3].y;

	int dir = 1;
	if( y0 > y3 ) {
		std::swap( y0, y3 );
		dir = -1;
	}
	if( test.y < y0 || test.y > y3 ) {
		return 0;
	}
	if( checkOnCurve( test, pts[0], pts[3] ) ) {
		*onCurveCount += 1;
		return 0;
	}
	if( test.y == y3 ) {
		return 0;
	}

	// quickreject or quickaccept
//	float minX, maxX;
//	findMinMaxX<4>( pts, &minX, &maxX );
//	if( test.x < minX ) {
//		return 0;
//	}
//	if( test.x > maxX ) {
//		return dir;
//	}

	// compute the actual x(t) value
	float t;
	if( ! chopMonoAtY( pts, test.y, &t ) ) {
		return 0;
	}
	float xt = evalCubicPts( pts[0].x, pts[1].x, pts[2].x, pts[3].x, t );
	if( fabsf( xt - test.x ) < ( 1.0f / ( 1 << 12 ) ) ) {
		if( test.x != pts[3].x || test.y != pts[3].y ) {  // don't test end points; they're start points
			*onCurveCount += 1;
			return 0;
		}
	}
	return xt < test.x ? dir : 0;
}

int windingCubic( const vec2 pts[], const vec2 &test, int *onCurveCount )
{
	vec2 dst[10];
	int n = chopCubicAtYExtrema( pts, dst );
	int w = 0;
	for( int i = 0; i <= n; ++i )
		w += windingMonoCubic( &dst[i * 3], test, onCurveCount );

	return w;
}
