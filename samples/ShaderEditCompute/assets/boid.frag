#version 150

in vec4 Color;
out vec4 FragColor;

// === TRY: Change these for different looks! ===
uniform float uBrightness;
uniform float uSaturation;

void main() {
	// Make points circular
	vec2 coord = gl_PointCoord - vec2(0.5);
	float dist = length(coord);
	if( dist > 0.5 ) discard;

	// Soft edge
	float alpha = 1.0 - smoothstep(0.3, 0.5, dist);

	// Apply brightness and saturation
	vec3 color = Color.rgb;
	float gray = dot(color, vec3(0.299, 0.587, 0.114));
	color = mix(vec3(gray), color, uSaturation);
	color *= uBrightness;

	// === TRY: Uncomment for glow effect ===
	// color = pow(color, vec3(0.7));

	FragColor = vec4(color, alpha);
}
