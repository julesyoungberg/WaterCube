#version 460 core

in vec3 oNormal;
out vec4 color;

void main() {
	color = vec4((oNormal + 1.0) * 0.5, 1);
}
