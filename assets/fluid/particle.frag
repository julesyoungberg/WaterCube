#version 460 core

in vec3 vPosition;
in vec3 vColor;
out vec4 outColor;

uniform mat4 ciViewProjection;
uniform int renderMode;
uniform float pointRadius;

void main() {
    vec4 eyeSpacePos;

    if (renderMode == 0) {
        eyeSpacePos = vec4(vPosition, 1);
    } else {
        vec3 N;
        N.xy = gl_PointCoord * vec2(2, -2) + vec2(-1, 1);
        const float r2 = dot(N.xy, N.xy);

        if (r2 > 1) {
            discard;
        }

        N.z = sqrt(1.0 - r2);

        eyeSpacePos = vec4(vPosition + N * pointRadius, 1.0);
    }

    const vec4 clipSpacePos = ciViewProjection * eyeSpacePos;
    const float ndcDepth = clipSpacePos.z / clipSpacePos.w;
    const float windowDepth = 0.5 * ndcDepth + 0.5;

    gl_FragDepth = windowDepth;
    outColor = vec4(vColor, 0.75);
}
