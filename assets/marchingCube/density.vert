#version 460 core
#extension GL_ARB_shader_storage_buffer_object : require

layout(location = 0) in int particleID;
out vec3 vColor;
out vec3 vPosition;

layout(location = 0, r32ui) uniform restrict uimage3D densityField;
uniform mat4 ciModelViewProjection;
uniform float size;
uniform int res;

layout(std430, binding = 0) restrict readonly buffer Particles {
    ivec3 particles[];
};

vec3 coordToPoint(ivec3 c) {
    return (vec3(c) / float(res)) * size;
} 

void main() {
    const ivec3 position = particles[particleID];
    const float density = uintBitsToFloat(imageLoad(densityField, position).x);
    vPosition = coordToPoint(position);
    vColor = vec3(density);
    gl_PointSize = 1;
    gl_Position = ciModelViewProjection * vec4(vPosition, 1);
}
