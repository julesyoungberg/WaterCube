#version 420
#extension GL_ARB_shader_storage_buffer_object : require

layout(location = 0) in int particleId;
out vec3 vColor;

struct Particle {
	vec3 	position;
    vec3 	velocity;
    float 	density;
    float 	pressure;
};

layout(std140, binding = 0) buffer Part {
    Particle particles[];
};

uniform mat4 ciModelViewProjection;

void main() {
	gl_Position = ciModelViewProjection * vec4(particles[particleId].position, 1);
	vColor = particles[particleId].position.xyz;
}
