#version 150

uniform mat4 ciModelViewProjection;
in vec4 ciPosition;
in vec4 ciColor;

out vec4 Color;

// === TRY: Change point size based on depth! ===
uniform float uPointSize;

void main() {
	Color = ciColor;
	gl_Position = ciModelViewProjection * ciPosition;

	// Depth-based sizing: points get smaller as they get farther away
	float depth = gl_Position.z / gl_Position.w;
	// Convert from [-1, 1] to [0, 1] and scale point size
	float depthFactor = (depth + 1.0) * 0.5;
	gl_PointSize = uPointSize * mix(1.5, 0.5, depthFactor);
}
