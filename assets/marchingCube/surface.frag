#version 460 core

const float AMBIENT_STRENGTH = 0.3;
const float SPECULAR_STRENGTH = 0.5;
const float SHININESS = 25.0;
const vec3 PARTICLE_COLOR = vec3(0.4, 0.4, 0.9);

in vec3 vNormal;
in vec3 vPosition;
out vec4 fColor;

uniform vec3 lightPos;
uniform vec3 cameraPos;

void main() {
	const vec3 color = PARTICLE_COLOR;
	const vec3 lightDir = normalize(lightPos - vPosition);
	const vec3 norm = normalize(vNormal);

	// Ambient
	const vec3 ambient = AMBIENT_STRENGTH * color;

	// Diffuse
	const float diff = max(dot(norm, lightDir), 0.0);
	const vec3 diffuse = diff * color;

	// Specular (Blinn-Phong)
	const vec3 viewDir = normalize(cameraPos - vPosition);
	const vec3 reflectDir = reflect(-lightDir, norm);
	const float spec = pow(max(dot(viewDir, reflectDir), 0.0), SHININESS);
	const vec3 specular = spec * SPECULAR_STRENGTH * color;

	// Final
	fColor = vec4(ambient + diffuse + specular, 0.1);
}
