#version 460 core
#extension GL_ARB_shader_storage_buffer_object : require

layout(location = 0) in int gridID;
out vec3 vColor;
out vec3 vPosition;

layout(std430, binding = 0) restrict readonly buffer Grid {
    ivec4 grid[];
};

layout(binding = 1, rgba32f) uniform restrict readonly image3D tex;
uniform mat4 ciModelViewProjection;
uniform float binSize;
uniform int gridRes;
uniform float size;

vec3 coordToPoint(ivec3 c) {
    return (vec3(c) / float(gridRes)) * size;
} 

void main() {
    const ivec3 coord = grid[gridID].xyz;
    vColor = imageLoad(tex, coord).xyz / 0.1;
    vPosition = coordToPoint(coord) + vec3(binSize / 2.0);
    gl_Position = ciModelViewProjection * vec4(vPosition, 1);
}
