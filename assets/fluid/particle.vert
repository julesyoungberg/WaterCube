#version 460 core
#extension GL_ARB_shader_storage_buffer_object : require

const float FLOAT_MIN = 1.175494351e-38;
const float GRID_EPS = 0.000001;
const float MAX_DENSITY = 50.0;
const vec3 PARTICLE_COLOR = vec3(0.0, 0.0, 0.6);

layout(location = 0) in int particleId;
out vec3 vColor;
out vec3 vPosition;

uniform mat4 ciModelViewProjection;

struct Particle {
    vec3 position;
    vec3 velocity;
    float density;
    float pressure;
};

layout(std430, binding = 1) buffer Particles {
    Particle particles[];
};

void main() {
	vec3 position = particles[particleId].position;
    vColor = PARTICLE_COLOR;
    vPosition = position;
	gl_Position = ciModelViewProjection * vec4(position, 1);
}
