// Plasma Effect - CC0 License
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
	vec2 uv = fragCoord / iResolution.xy;
	vec2 p = uv * 2.0 - 1.0;
	p.x *= iResolution.x / iResolution.y;

	// Multiple sine waves create plasma effect
	float v = 0.0;
	v += sin((p.x + iTime) * 3.0);
	v += sin((p.y + iTime * 0.7) * 3.0);
	v += sin((p.x + p.y + iTime * 0.5) * 3.0);

	vec2 c = p + vec2(sin(iTime * 0.3), cos(iTime * 0.4));
	v += sin(sqrt(c.x * c.x + c.y * c.y + 1.0) * 5.0);

	v *= 0.5;

	// Map to color
	vec3 col = vec3(
		0.5 + 0.5 * sin(v + iTime * 2.0),
		0.5 + 0.5 * sin(v + iTime * 2.0 + 2.0),
		0.5 + 0.5 * sin(v + iTime * 2.0 + 4.0)
	);

	fragColor = vec4(col, 1.0);
}
