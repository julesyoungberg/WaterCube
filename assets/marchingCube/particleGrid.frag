#version 460 core

in vec3 vPosition;
in vec3 vColor;
out vec4 outColor;

void main() {
    outColor = vec4(vColor, 1);
}
