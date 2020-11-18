#version 460 core
#extension GL_ARB_shader_storage_buffer_object : require

layout(location = 0) in int particleID;
out vec3 vColor;
out vec3 vPosition;

layout(std430, binding = 0) restrict readonly buffer Particles {
    ivec4 particles[];
};

layout(std430, binding = 1) buffer Density {
    uint densities[];
};

uniform mat4 ciModelViewProjection;
uniform float size;
uniform int res;

vec3 coordToPoint(ivec3 c) {
    return (vec3(c) / float(res)) * size;
} 

void main() {
    const ivec3 c = particles[particleID].xyz;
    const uint index = c.z * res * res + c.y * res + c.x;
    const float density = uintBitsToFloat(densities[index]);
    const bool invalid = isnan(density) || isinf(density);
    const float binSize = size / float(res);
    vPosition = coordToPoint(c) + vec3(binSize / 2);
    vColor = mix(vec3(density), vec3(1, 0, 0), float(invalid));
    gl_Position = ciModelViewProjection * vec4(vPosition, 1);
}
