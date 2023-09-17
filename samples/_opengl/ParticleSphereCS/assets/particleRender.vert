#version 420
#extension GL_ARB_shader_storage_buffer_object : require

layout( location = 0 ) in vec3 particleColor;
out vec3 vColor;

struct Particle
{
	vec3	pos;
	vec3	ppos;
	vec3	home;
	float	damping;
};

layout( std140, binding = 0 ) buffer Particles
{
    Particle particles[];
};

uniform mat4 ciModelViewProjection;


void main()
{
	gl_Position = ciModelViewProjection * vec4( particles[gl_VertexID].pos, 1 );
	vColor = particleColor;
}