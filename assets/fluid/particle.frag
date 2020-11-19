#version 460 core

const float AMBIENT_STRENGTH = 0.4;
const float SPECULAR_STRENGTH = 0.6;
const float SHININESS = 32.0;
const vec3 PARTICLE_COLOR = vec3(0.2, 0.2, 1);

in vec3 vPosition;
in vec3 vColor;
out vec4 outColor;

uniform vec3 lightPos;
uniform vec3 cameraPos;
uniform int renderMode;

void main() {
    if (renderMode > 0) {
        outColor = vec4(vColor, 1);
        return;
    }

    // calculate normal from texture coordinates
    vec3 N;
    N.xy = gl_PointCoord * 2.0 - vec2(1.0);    
    
    const float mag = dot(N.xy, N.xy);
    if (mag > 1.0) discard;   // kill pixels outside circle
    
    N.z = sqrt(1.0 - mag);

    // calculate lighting
    vec3 color = PARTICLE_COLOR;
	const vec3 lightDir = normalize(lightPos - vPosition);

    // Ambient
	const vec3 ambient = AMBIENT_STRENGTH * color;

    // Diffuse
    const float diff = max(0.0, dot(lightDir, N));
    const vec3 diffuse = diff * color;

    // specular
    const vec3 viewDir = normalize(cameraPos - vPosition);
	const vec3 reflectDir = reflect(-lightDir, N);
	const float spec = pow(max(dot(viewDir, reflectDir), 0.0), SHININESS);
	const vec3 specular = spec * SPECULAR_STRENGTH * color;

    outColor = vec4(ambient + diffuse + specular, 1);
}
