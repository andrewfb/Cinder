// Simple Time and Resolution Demo
// This shader demonstrates iTime and iResolution uniforms
// License: CC0 (Public Domain)
// Created for Shadertoy Runner educational purposes

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
	// Normalize coordinates (0.0 to 1.0)
	vec2 uv = fragCoord / iResolution.xy;

	// Create animated color using time
	vec3 col = 0.5 + 0.5 * cos( iTime + uv.xyx + vec3(0, 2, 4) );

	// Output final color
	fragColor = vec4( col, 1.0 );
}
