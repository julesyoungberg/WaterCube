#version 460 core
#extension GL_ARB_shader_storage_buffer_object : require

layout(location = 0) in int particleID;
out vec3 vColor;
out vec3 vPosition;

layout(std430, binding = 0) restrict readonly buffer Particles {
    ivec3 particles[];
};

layout(binding = 1, r32ui) uniform restrict uimage3D densityField;
uniform mat4 ciModelViewProjection;
uniform float size;
uniform int res;

vec3 coordToPoint(ivec3 c) {
    return (vec3(c) / float(res)) * size;
} 

void main() {
    const ivec3 position = particles[particleID];
    const float density = uintBitsToFloat(imageLoad(densityField, position).x);
    const bool invalid = (density < 0.0) || isnan(density) || isinf(density);
    vPosition = coordToPoint(position);
    vColor = mix(vec3(density), vec3(1, 0, 0), float(invalid));
    gl_Position = ciModelViewProjection * vec4(vPosition, 1);
}
