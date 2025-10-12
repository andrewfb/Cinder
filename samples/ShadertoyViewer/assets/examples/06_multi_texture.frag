// Multi-Texture Demo - Smooth Blending
// Demonstrates using multiple texture channels with smooth transitions
// Uses iChannel0, iChannel1, iChannel2 with different patterns
// License: CC0 (Public Domain)

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
	vec2 uv = fragCoord / iResolution.xy;

	// Slowly animated displacement from iChannel1
	vec2 displace = texture(iChannel1, uv * 0.5 + iTime * 0.02).rg - 0.5;

	// Sample iChannel0 with gentle distortion
	vec2 uv1 = uv + displace * 0.1;
	vec3 tex1 = texture(iChannel0, uv1).rgb;

	// Sample iChannel2 with rotation
	vec2 center = uv - 0.5;
	float angle = iTime * 0.2;
	mat2 rot = mat2(cos(angle), -sin(angle), sin(angle), cos(angle));
	vec2 uv2 = rot * center + 0.5;
	vec3 tex2 = texture(iChannel2, uv2).rgb;

	// Smooth radial blend between the two textures
	float dist = length(uv - 0.5);
	float blend = smoothstep(0.2, 0.8, dist);

	vec3 col = mix(tex1, tex2, blend);

	// Subtle color enhancement
	col = pow(col, vec3(0.9));
	col *= 1.0 - dist * 0.3;

	fragColor = vec4(col, 1.0);
}
