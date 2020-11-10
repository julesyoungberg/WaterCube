#version 460 core
#extension GL_ARB_shader_storage_buffer_object : require

const float FLOAT_MIN = 1.175494351e-38;
const float GRID_EPS = 0.000001;
const float MAX_DENSITY = 0.2;
const vec3 PARTICLE_COLOR = vec3(0.0, 0.0, 0.6);

layout(location = 0) in int particleID;
out vec3 vColor;
out vec3 vPosition;

uniform mat4 ciModelViewProjection;
uniform mat4 ciViewMatrix;
uniform int renderMode;
uniform float binSize;
uniform int gridRes;
uniform float pointRadius;
uniform float pointScale;

struct Particle {
    vec3 position;
    vec3 velocity;
    float density;
    float pressure;
};

layout(std430, binding = 0) restrict readonly buffer Particles {
    Particle particles[];
};

void main() {
	Particle p = particles[particleID];
    vPosition = p.position;
    bool invalid;

    switch(renderMode) {
        case 1:
            const float w = max(p.velocity.x, p.velocity.y);
            invalid = any(isnan(p.velocity)) || any(isinf(p.velocity));
            vColor = mix(p.velocity / w, vec3(1, 0, 0), float(invalid));
            break;
        case 2:
            const float speed = length(p.velocity);
            vColor = vec3(speed, speed, 0);
            break;
        case 3:
            const float norm = uintBitsToFloat(floatBitsToUint(p.density)) / MAX_DENSITY;
            invalid = (p.density <= 0.0) || isnan(p.density) || isinf(p.density);
            vColor = mix(vec3(0, 0, norm), vec3(1, 0, 0), float(invalid));
            break;
        case 4:
            const ivec3 binCoord = ivec3(floor(p.position / binSize));
            vColor = vec3(binCoord) / gridRes;
            break;
        default:
            vColor = PARTICLE_COLOR;
    }
    
    vec4 position = ciViewMatrix * vec4(p.position, 1);

    const float dist = max(length(position.xyz), FLOAT_MIN);

    gl_PointSize = pointRadius * (pointScale / dist);
    gl_Position = ciModelViewProjection * vec4(p.position, 1);
}
