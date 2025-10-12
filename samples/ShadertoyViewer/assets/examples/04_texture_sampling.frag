// Texture Sampling Demo
// Demonstrates iChannel0 texture sampling with effects
// License: CC0 (Public Domain)
// Created for Shadertoy Runner educational purposes

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
	// Normalize coordinates
	vec2 uv = fragCoord / iResolution.xy;

	// Add wave distortion based on time
	vec2 distortion = vec2(
		sin( uv.y * 10.0 + iTime ) * 0.02,
		cos( uv.x * 10.0 + iTime ) * 0.02
	);

	vec2 sampledUV = uv + distortion;

	// Sample texture from iChannel0
	vec3 texColor = texture( iChannel0, sampledUV ).rgb;

	// Add color tint based on time
	vec3 tint = 0.5 + 0.5 * cos( iTime + vec3( 0, 2, 4 ) );
	vec3 col = texColor * mix( vec3( 1.0 ), tint, 0.3 );

	fragColor = vec4( col, 1.0 );
}
