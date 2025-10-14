#version 150

uniform mat4 ciModelViewProjection;
uniform mat4 ciModelView;
uniform mat3 ciNormalMatrix;

in vec4 ciPosition;
in vec3 ciNormal;

out vec3 vPosition;
out vec3 vNormal;

void main() {
	vPosition = vec3( ciModelView * ciPosition );
	vNormal = normalize( ciNormalMatrix * ciNormal );
	gl_Position = ciModelViewProjection * ciPosition;
}
