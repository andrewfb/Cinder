// Voronoi Pattern - CC0 License
vec2 hash2( vec2 p )
{
	p = vec2(dot(p, vec2(127.1, 311.7)),
			 dot(p, vec2(269.5, 183.3)));
	return fract(sin(p) * 43758.5453);
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
	vec2 uv = fragCoord / iResolution.xy;
	vec2 p = uv * 8.0;

	// Grid cell
	vec2 i = floor(p);
	vec2 f = fract(p);

	// Find closest point
	float minDist = 1.0;
	vec2 minPoint;

	for(int y = -1; y <= 1; y++) {
		for(int x = -1; x <= 1; x++) {
			vec2 neighbor = vec2(float(x), float(y));
			vec2 point = hash2(i + neighbor);
			point = 0.5 + 0.5 * sin(iTime + 6.2831 * point);
			vec2 diff = neighbor + point - f;
			float dist = length(diff);

			if(dist < minDist) {
				minDist = dist;
				minPoint = point;
			}
		}
	}

	// Color based on distance and cell
	vec3 col = vec3(minDist);
	col *= vec3(minPoint, 0.5 + 0.5 * sin(iTime));

	// Edge highlight
	col += 1.0 - step(0.05, minDist);

	fragColor = vec4(col, 1.0);
}
