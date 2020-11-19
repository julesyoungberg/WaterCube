#version 460 core
#extension GL_ARB_shader_storage_buffer_object : require

layout(location = 0) in int particleID;
out vec3 vColor;
out vec3 vPosition;

layout(std430, binding = 0) restrict readonly buffer Particles {
    ivec4 particles[];
};

layout(std430, binding = 1) restrict readonly buffer Density {
    uint densities[];
};

layout(binding = 2, r32f) uniform image3D pressureField;
uniform mat4 ciModelViewProjection;
uniform float size;
uniform int res;

vec3 coordToPoint(ivec3 c) {
    return (vec3(c) / float(res)) * size;
} 

void main() {
    const ivec3 c = particles[particleID].xyz;
    const uint index = c.z * res * res + c.y * res + c.x;

    const float density = densities[index];
    const bool densityInvalid = isnan(density) || isinf(density);

    const float pressure = imageLoad(pressureField, c).x;
    const bool pressureInvalid = isnan(pressure) || isinf(pressure);
    
    const float binSize = size / float(res);
    const bool invalid = densityInvalid || pressureInvalid;
    
    vPosition = coordToPoint(c) + vec3(binSize / 2);
    vColor = mix(vec3(0, density, pressure), vec3(1, 0, 0), float(invalid));
    
    gl_Position = ciModelViewProjection * vec4(vPosition, 1);
}
