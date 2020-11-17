#version 460 core

layout(location = 0) in int gridID;
out vec3 vColor;
out vec3 vPosition;

layout(std430, binding = 0) restrict readonly buffer Grid {
    ivec4 grid[];
};

layout(std430, binding = 1) restrict buffer Counts {
    uint counts[];
};

layout(std430, binding = 2) restrict buffer Offsets {
    uint offsets[];
};

uniform mat4 ciModelViewProjection;
uniform float binSize;
uniform int gridRes;
uniform float size;
uniform int numItems;

vec3 coordToPoint(ivec3 c) {
    return (vec3(c) / float(gridRes)) * size;
} 

void main() {
    const ivec3 coord = grid[gridID].xyz;
    const uint index = coord.z * gridRes * gridRes + coord.y * gridRes + coord.x;

    const uint count = counts[index];
    const uint offset = offsets[index];

    const float n = float(numItems) / 100.0;
    const vec3 color = vec3(0, float(count) / n, float(offset) / n);
    const bool invalid = any(lessThan(color, vec3(0))) || any(isnan(color)) || any(isinf(color));

    vPosition = coordToPoint(coord) + vec3(binSize / 2.0);
    vColor = mix(color, vec3(1, 0, 0), float(invalid));
    gl_Position = ciModelViewProjection * vec4(vPosition, 1);
}
