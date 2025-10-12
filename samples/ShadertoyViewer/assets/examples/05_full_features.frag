// Full Features Demo
// Demonstrates multiple Shadertoy uniforms together
// License: CC0 (Public Domain)
// Created for Shadertoy Runner educational purposes

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
	// Normalized pixel coordinates
	vec2 uv = fragCoord / iResolution.xy;
	vec2 centered = ( fragCoord - 0.5 * iResolution.xy ) / iResolution.y;

	// Mouse interaction
	vec2 mouse = iMouse.xy / iResolution.xy;
	float mouseDist = length( uv - mouse );

	// Time-based animation
	float t = iTime * 0.5;

	// Create layered pattern
	float layer1 = sin( centered.x * 5.0 + t ) * cos( centered.y * 5.0 + t );
	float layer2 = sin( length( centered ) * 10.0 - t * 2.0 );

	// Combine patterns
	float pattern = mix( layer1, layer2, 0.5 + 0.5 * sin( t * 0.3 ) );

	// Color palette cycling with date
	float dayPhase = mod( iDate.w / 86400.0, 1.0 );  // Cycle through day
	vec3 col = 0.5 + 0.5 * cos( dayPhase * 6.28 + pattern + vec3( 0, 2, 4 ) );

	// Add mouse highlight
	col = mix( col, vec3( 1.0, 1.0, 0.5 ), smoothstep( 0.2, 0.0, mouseDist ) );

	// Frame counter creates subtle pulse
	float pulse = 0.5 + 0.5 * sin( float( iFrame ) * 0.05 );
	col *= 0.8 + 0.2 * pulse;

	fragColor = vec4( col, 1.0 );
}
