#version 460 core

layout(location = 0) in int gridID;
out vec3 oNormal;

struct Grid {
	vec4 position;
	vec4 normal;
};

layout(std430, binding = 0) buffer grids {
	Grid grid[];
};

uniform mat4 ciModelViewProjection;

void main() {
    Grid grid = grid[gridID];
	gl_Position = ciModelViewProjection * vec4(grid.position.xyz, 1);
	oNormal = grid.normal.xyz;
}
