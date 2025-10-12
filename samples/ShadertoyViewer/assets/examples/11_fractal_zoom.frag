// Fractal Zoom - CC0 License
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
	vec2 uv = fragCoord / iResolution.xy;
	vec2 p = (uv - 0.5) * 2.0;
	p.x *= iResolution.x / iResolution.y;

	// Zoom and rotate over time
	float zoom = 0.5 + 0.5 * sin(iTime * 0.5);
	float angle = iTime * 0.2;
	mat2 rot = mat2(cos(angle), -sin(angle), sin(angle), cos(angle));
	p = rot * p * (1.0 + zoom);

	// Fractal pattern
	vec3 col = vec3(0.0);
	for(float i = 0.0; i < 5.0; i++) {
		p = abs(p) / dot(p, p) - 0.7;
		col += vec3(
			0.5 + 0.5 * cos(length(p) * 3.0 + i + iTime),
			0.5 + 0.5 * cos(length(p) * 3.0 + i + iTime + 2.0),
			0.5 + 0.5 * cos(length(p) * 3.0 + i + iTime + 4.0)
		) / 5.0;
	}

	fragColor = vec4(col, 1.0);
}
