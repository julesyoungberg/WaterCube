#version 460 core

const float FLOAT_MIN = 1.175494351e-38;
const float GRID_EPS = 0.000001;
const float MAX_DENSITY = 600.0;
const vec3 PARTICLE_COLOR = vec3(0.1, 0.1, 0.5);

out vec3 vColor;
out vec3 vPosition;

struct Particle {
    vec3 position;
    float density;
    vec3 velocity;
    float pressure;
};

layout(binding = 0, std430) restrict readonly buffer Particles {
    Particle particles[];
};

uniform mat4 ciModelViewProjection;
uniform mat4 ciViewMatrix;
uniform int renderMode;
uniform float size;
uniform float binSize;
uniform int gridRes;
uniform float pointRadius;

void main() {
	Particle p = particles[gl_VertexID];
    vPosition = p.position;
    bool invalid;

    switch(renderMode) {
        case 1:
            const float w = max(p.velocity.x, p.velocity.y);
            invalid = any(isnan(p.velocity)) || any(isinf(p.velocity)) || any(lessThan(p.position, vec3(0))) || any(greaterThan(p.position, vec3(size)));
            vColor = mix(vec3(0, 0, length(p.velocity) / w), vec3(1, 0, 0), float(invalid));
            break;
        case 2:
            const float norm = p.density / MAX_DENSITY;
            invalid = (p.density <= 0.0) || isnan(p.density) || isinf(p.density);
            vColor = mix(vec3(norm), vec3(1, 0, 0), float(invalid));
            break;
        case 3:
            const ivec3 binCoord = ivec3(floor(p.position / binSize));
            vColor = vec3(binCoord) / gridRes;
            break;
        default:
            vColor = PARTICLE_COLOR;
    }
    
    vec4 position = ciViewMatrix * vec4(p.position, 1);

    const float dist = max(length(position.xyz), FLOAT_MIN);

    gl_Position = ciModelViewProjection * vec4(p.position, 1);
    gl_PointSize = pointRadius * 2;
}
