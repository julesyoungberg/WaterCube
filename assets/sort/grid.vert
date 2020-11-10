#version 460 core
#extension GL_ARB_shader_storage_buffer_object : require

layout(location = 0) in int gridID;
out vec3 vColor;
out vec3 vPosition;

layout(std430, binding = 0) restrict readonly buffer Grid {
    ivec3 grid[];
};

layout(binding = 0, r32ui) uniform restrict readonly uimage3D countGrid;
layout(binding = 1, r32ui) uniform restrict readonly uimage3D offsetGrid;
uniform mat4 ciModelViewProjection;
uniform float binSize;
uniform int gridRes;
uniform float size;
uniform int numItems;

vec3 coordToPoint(ivec3 c) {
    return (vec3(c) / float(gridRes)) * size;
} 

void main() {
    const ivec3 coord = grid[gridID];

    const uint count = imageLoad(countGrid, coord).x;
    const uint offset = imageLoad(offsetGrid, coord).x;

    const vec3 color = vec3(0, float(count) / float(numItems), float(offset) / float(numItems));
    const bool invalid = any(lessThan(color, vec3(0))) || any(isnan(color)) || any(isinf(color));

    vPosition = coordToPoint(coord) + vec3(binSize / 2.0);
    vColor = mix(color, vec3(1, 0, 0), float(invalid));
    gl_Position = ciModelViewProjection * vec4(vPosition, 1);
}
