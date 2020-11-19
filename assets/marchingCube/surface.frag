#version 460 core

const float AMBIENT_STRENGTH = 0.3;
const float SHININESS = 25.0;
const vec3 PARTICLE_COLOR = vec3(0.3, 0.3, 0.9);

in vec3 vNormal;
in vec3 vPosition;
out vec4 fColor;

uniform float size;
uniform vec3 lightPos;

void main() {
	const vec3 color = PARTICLE_COLOR;

	// Ambient
	const vec3 ambient = AMBIENT_STRENGTH * color;

	// Diffuse
	const vec3 norm = normalize(vNormal);
	const vec3 lightDir = normalize(lightPos - vPosition);
	const float diff = max(dot(norm, lightDir), 0.0);
	const vec3 diffuse = diff * color;

	// // Specular (Blinn-Phong)
	// const vec3 incidence = normalize(lightPosEye - eyeSpacePos.xyz);
	// const vec3 viewDir = normalize(vec3(0.0) - eyeSpacePos.xyz);
	// const vec3 halfDir = normalize(incidence + viewDir);
	// const float cosAngle = max(dot(halfDir, normal), 0.0);
	// const float scoeff = pow(cosAngle, SHININESS);
	// const vec3 specColor = vec3(1.0) * scoeff;

	// Final
	fColor = vec4(ambient + diffuse, 0.5);
}
