#version 460 core

layout(location = 0) in int vertexID;
out vec3 oNormal;

struct Vertex {
	vec4 position;
	vec4 normal;
};

layout(std430, binding = 0) restrict readonly buffer Vertices {
	Vertex vertices[];
};

uniform mat4 ciModelViewProjection;

void main() {
    Vertex v = vertices[vertexID];
	gl_Position = ciModelViewProjection * vec4(v.position.xyz, 1);
	oNormal = v.normal.xyz;
}
