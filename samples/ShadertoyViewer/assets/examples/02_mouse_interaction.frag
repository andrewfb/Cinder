// Mouse Interaction - Flocking Balls Demo
// Move your mouse around to see colored balls flock to the cursor position
// License: CC0 (Public Domain)

float hash(vec2 p)
{
	return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
	vec2 uv = fragCoord / iResolution.xy;
	vec2 p = (uv - 0.5) * 2.0;
	p.x *= iResolution.x / iResolution.y;

	// Mouse position in screen space
	vec2 mouse = (iMouse.xy / iResolution.xy - 0.5) * 2.0;
	mouse.x *= iResolution.x / iResolution.y;

	vec3 col = vec3(0.05, 0.08, 0.12);

	// Glowing cursor indicator
	float cursorDist = length(p - mouse);
	float cursor = exp(-cursorDist * 15.0);
	col += vec3(1.0, 1.0, 1.0) * cursor * 1.5;

	// Cursor ring
	float ring = smoothstep(0.15, 0.12, cursorDist) * (1.0 - smoothstep(0.12, 0.10, cursorDist));
	col += vec3(0.8, 0.9, 1.0) * ring * 0.5;

	// Energetic particle system that explodes outward from mouse
	const float numBalls = 40.0;
	for(float i = 0.0; i < numBalls; i++) {
		// Each particle has a unique angle and speed
		float angle = hash(vec2(i, 0.0)) * 6.28318 + iTime * (1.2 + hash(vec2(i, 2.0)) * 1.5);
		float speed = 0.8 + hash(vec2(i, 1.0)) * 1.2;

		// Particles spiral outward from mouse with high energy
		float radius = mod(iTime * speed + hash(vec2(i, 4.0)) * 3.0, 2.5);
		vec2 direction = vec2(cos(angle), sin(angle));
		vec2 ballPos = mouse + direction * radius;

		// Add turbulent motion
		ballPos += vec2(
			sin(iTime * 2.5 + i * 0.5) * 0.3,
			cos(iTime * 2.0 + i * 0.7) * 0.3
		);

		float ballDist = length(p - ballPos);
		float ballSize = 0.025 + hash(vec2(i, 3.0)) * 0.015;
		float ball = smoothstep(ballSize, ballSize * 0.3, ballDist);

		// Vibrant, saturated colors
		vec3 ballCol = vec3(
			0.5 + 0.5 * sin(i * 1.2 + iTime),
			0.5 + 0.5 * sin(i * 1.2 + 2.094 + iTime * 0.5),
			0.5 + 0.5 * sin(i * 1.2 + 4.189 + iTime * 0.3)
		);

		// Strong glow with distance falloff
		float glow = exp(-ballDist * 3.0) * 0.6;

		// Fade particles as they get farther from mouse
		float distFromMouse = length(ballPos - mouse);
		float fadeFactor = 1.0 - smoothstep(1.0, 2.5, distFromMouse);

		col += ballCol * (ball + glow) * fadeFactor;
	}

	// Subtle grid in background
	float grid = max(
		smoothstep(0.02, 0.0, fract(p.x * 4.0) - 0.5) * 0.03,
		smoothstep(0.02, 0.0, fract(p.y * 4.0) - 0.5) * 0.03
	);
	col += vec3(grid);

	// Vignette
	float vignette = 1.0 - length(uv - 0.5) * 0.6;
	col *= vignette;

	fragColor = vec4(col, 1.0);
}
