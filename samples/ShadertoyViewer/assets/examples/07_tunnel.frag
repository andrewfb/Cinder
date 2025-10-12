// Tunnel Effect - CC0 License
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
	vec2 uv = fragCoord / iResolution.xy;
	vec2 p = uv * 2.0 - 1.0;
	p.x *= iResolution.x / iResolution.y;

	// Tunnel coordinates
	float r = length(p);
	float a = atan(p.y, p.x);

	// Create tunnel effect
	float u = 1.0 / r + iTime * 0.5;
	float v = a / 3.14159;

	// Grid pattern
	vec2 grid = fract(vec2(u, v) * 10.0);
	float pattern = step(0.5, grid.x) * step(0.5, grid.y);

	// Color based on depth
	vec3 col = vec3(0.5 + 0.5 * sin(u * 3.0 + vec3(0.0, 2.0, 4.0)));
	col *= pattern * 0.5 + 0.5;

	// Vignette
	col *= 1.0 - 0.5 * r;

	fragColor = vec4(col, 1.0);
}
