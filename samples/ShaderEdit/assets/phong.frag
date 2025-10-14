#version 150

uniform vec3 uPointLightPosition;
uniform vec3 uDirectionalLightDir;
uniform vec3 uDirectionalLightColor;
uniform vec3 uAmbientColor;
uniform vec3 uDiffuseColor;
uniform vec3 uSpecularColor;
uniform float uShininess;

in vec3 vPosition;
in vec3 vNormal;

out vec4 oColor;

void main() {
	vec3 N = normalize( vNormal );
	vec3 V = normalize( -vPosition );

	// Ambient
	vec3 ambient = uAmbientColor;

	// Point Light
	vec3 L1 = normalize( uPointLightPosition - vPosition );
	vec3 R1 = reflect( -L1, N );
	float diff1 = max( dot( N, L1 ), 0.0 );
	float spec1 = pow( max( dot( V, R1 ), 0.0 ), uShininess );
	vec3 pointLight = diff1 * uDiffuseColor + spec1 * uSpecularColor;

	// Directional Light
	vec3 L2 = normalize( -uDirectionalLightDir );
	vec3 R2 = reflect( -L2, N );
	float diff2 = max( dot( N, L2 ), 0.0 );
	float spec2 = pow( max( dot( V, R2 ), 0.0 ), uShininess );
	vec3 directionalLight = diff2 * uDiffuseColor * uDirectionalLightColor + spec2 * uSpecularColor * uDirectionalLightColor;

	vec3 finalColor = ambient + pointLight + directionalLight;
	oColor = vec4( finalColor, 1.0 );
}
