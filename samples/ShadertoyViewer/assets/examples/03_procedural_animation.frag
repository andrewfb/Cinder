// Procedural Animation Demo
// Demonstrates time-based procedural generation
// License: CC0 (Public Domain)
// Created for Shadertoy Runner educational purposes

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
	// Normalize and center coordinates
	vec2 uv = ( fragCoord - 0.5 * iResolution.xy ) / iResolution.y;

	// Polar coordinates
	float angle = atan( uv.y, uv.x );
	float radius = length( uv );

	// Create animated pattern
	float pattern = sin( radius * 10.0 - iTime * 2.0 + angle * 3.0 );
	pattern = smoothstep( -0.5, 0.5, pattern );

	// Color based on angle and time
	vec3 col = 0.5 + 0.5 * vec3(
		cos( angle + iTime ),
		sin( angle + iTime * 0.7 ),
		cos( angle * 2.0 - iTime * 0.5 )
	);

	// Apply pattern
	col *= pattern;

	fragColor = vec4( col, 1.0 );
}
