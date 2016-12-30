#include "ZeroCrossings.h"

using namespace std;
using namespace cinder;

namespace {
float linearYatX( const vec2 p[2], float x )
{
	if( p[0].x == p[1].x ) 	return p[0].y;
	return p[0].y + (p[1].y - p[0].y) * (x - p[0].x) / (p[1].x - p[0].x);
}

void linearCrossings( const vec2 p[2], const vec2 &pt, vector<vec2> *result )
{
	// vertical line exactly equal to pt.x
	if( fabs( p[0].x - p[1].x ) < 0.00001f && fabs( p[0].x - pt.x ) < 0.00001f ) {
		if( p[0].y < pt.y )
			result->push_back( p[0] );
		if( p[1].y < pt.y )
			result->push_back( p[1] );
		return;
	}

	if( fabs( p[0].x - pt.x ) < 0.000001f && p[0].y < pt.y ) {
		if( p[1].x > pt.x )
			result->push_back( p[0] );
	}
	else if( fabs( p[1].x - pt.x ) < 0.000001f && p[1].y < pt.y ) {
		if( p[0].x > pt.x ) {
			result->push_back( p[1] );
		}
	} 
	else if( (p[0].x < pt.x && pt.x < p[1].x ) ||
		(p[1].x < pt.x && pt.x < p[0].x )) {
		float intersection = linearYatX( p, pt.x ); 
		if( intersection < pt.y ) {
			result->push_back( { pt.x, intersection } );
		}
	}
}

void cubicBezierCrossings( const vec2 p[4], const vec2 &pt, vector<vec2> *result )
{
	double Ax =     -p[0].x + 3 * p[1].x - 3 * p[2].x + p[3].x;
	double Bx =  3 * p[0].x - 6 * p[1].x + 3 * p[2].x;
	double Cx = -3 * p[0].x + 3 * p[1].x;
	double Dx =		p[0].x - pt.x;

	double Ay =     -p[0].y + 3 * p[1].y - 3 * p[2].y + p[3].y;
	double By =  3 * p[0].y - 6 * p[1].y + 3 * p[2].y;
	double Cy = -3 * p[0].y + 3 * p[1].y;
	double Dy =		p[0].y;

	double roots[3];
	int numRoots = solveCubic<double>( Ax, Bx, Cx, Dx, roots );

	if( numRoots < 1)
		return;

	for( int n = 0; n < numRoots; ++n ) {
		if( roots[n] > 0 && roots[n] < 1 ) {
			if( Ay * roots[n] * roots[n] * roots[n] + By * roots[n] * roots[n] + Cy * roots[n] + Dy < pt.y )
				result->push_back( vec2( pt.x, Ay * roots[n] * roots[n] * roots[n] + By * roots[n] * roots[n] + Cy * roots[n] + Dy ) );
		}
		else if( fabs( roots[n] ) < 0.000001 ) {
			if( Ay * roots[n] * roots[n] * roots[n] + By * roots[n] * roots[n] + Cy * roots[n] + Dy && Path2d::calcCubicBezierDerivative( p, 0.00001f ).x > 0 )
				result->push_back( vec2( pt.x, Ay * roots[n] * roots[n] * roots[n] + By * roots[n] * roots[n] + Cy * roots[n] + Dy ) );
		}
		else if( fabs( 1.0 - roots[n] ) < 0.000001 ) {
			if( Ay * roots[n] * roots[n] * roots[n] + By * roots[n] * roots[n] + Cy * roots[n] + Dy && Path2d::calcCubicBezierDerivative( p, 0.99999f ).x < 0 )
				result->push_back( vec2( pt.x, Ay * roots[n] * roots[n] * roots[n] + By * roots[n] * roots[n] + Cy * roots[n] + Dy ) );
		}
	}
}

void quadraticBezierCrossings( const vec2 p[3], const vec2 &pt, vector<vec2> *result )
{
	double Ax = 1.0 * p[0].x - 2.0 * p[1].x + 1.0 * p[2].x;
	double Bx = -2.0 * p[0].x + 2.0 * p[1].x;
	double Cx = 1.0 * p[0].x - pt.x;

	double Ay = 1.0 * p[0].y - 2.0 * p[1].y + 1.0 * p[2].y;
	double By = -2.0 * p[0].y + 2.0 * p[1].y;
	double Cy = 1.0 * p[0].y;

	double roots[2];
	int numRoots = solveQuadratic( Ax, Bx, Cx, roots );

	if( numRoots < 1 ) {
		return;
	}

	for( int n = 0; n < numRoots; ++n ) {
		if( roots[n] == 0.0 ) { // is this root unique
			if( numRoots == 1 || roots[n^1] <= 0 || roots[n^1] >= 1 ) {
				float deriv = Path2d::calcQuadraticBezierDerivative( p, 0.01f ).x;
				if( Ay * roots[n] * roots[n] + By * roots[n] + Cy < pt.y ) {
					if( deriv > 0 )
						result->push_back( vec2( pt.x, Ay * roots[n] * roots[n] + By * roots[n] + Cy ) );				
					else if( deriv == 0 && p[2].x > pt.x ) {
						result->push_back( vec2( pt.x, Ay * roots[n] * roots[n] + By * roots[n] + Cy ) );
					}
				}
			}
		}
		else if( roots[n] == 1.0 ) { //fabs( 1.0 - roots[n] ) < 0.000001 ) {
			if( numRoots == 1 || roots[n^1] <= 0 || roots[n^1] >= 1 ) {
				float deriv = Path2d::calcQuadraticBezierDerivative( p, 0.99f ).x;
				if( Ay * roots[n] * roots[n] + By * roots[n] + Cy < pt.y ) {
					if( deriv < 0 ) //
						result->push_back( vec2( pt.x, Ay * roots[n] * roots[n] + By * roots[n] + Cy ) );
					else if( deriv == 0 && p[0].x > pt.x ) {				
						result->push_back( vec2( pt.x, Ay * roots[n] * roots[n] + By * roots[n] + Cy ) );
					}
				}
			}
		}
		else if (roots[n] > 0 && roots[n] < 1 ) {
			if( Ay * roots[n] * roots[n] + By * roots[n] + Cy < pt.y )
				result->push_back( vec2( pt.x, Ay * roots[n] * roots[n] + By * roots[n] + Cy ) );
		}
	}
}
} // anonymous namespace

void findZeroes( const Shape2d &shape, const vec2 &pt, vector<vec2> *result )
{
	result->clear();
	for( size_t c = 0; c < shape.getNumContours(); ++c )
		findZeroes( shape.getContour( c ), pt, result );
}

void findZeroes( const Path2d &path, const vec2 &pt, vector<vec2> *result )
{
	auto points = path.getPoints();

	if( points.size() <= 2 )
		return;

	size_t firstPoint = 0;
	for( size_t s = 0; s < path.getSegments().size(); ++s ) {
		switch( path.getSegmentType( s ) ) {
			case Path2d::CUBICTO:
				cubicBezierCrossings( &(points[firstPoint]), pt, result );
			break;
			case Path2d::QUADTO:
				quadraticBezierCrossings( &(points[firstPoint]), pt, result );
			break;
			case Path2d::LINETO:
				linearCrossings( &(points[firstPoint]), pt, result );
			break;
			case Path2d::CLOSE: // ignore - we always assume closed
			break;
			default:
				;//throw Path2dExc();
		}
		
		firstPoint += Path2d::sSegmentTypePointCounts[path.getSegments()[s]];
	}

	vec2 temp[2];
	temp[0] = points[points.size()-1];
	temp[1] = points[0];
	if( distance2( temp[0], temp[1] ) > 0.00001 )
		linearCrossings( &(temp[0]), pt, result );
}
