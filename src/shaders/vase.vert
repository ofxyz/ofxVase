#version 150

uniform mat4 modelViewProjectionMatrix;

in vec4 position;
in vec4 color;
in vec2 texcoord;
in vec3 normal;  // repurposed: xy = fade factors (rx, ry)

out vec4 vColor;
out vec2 vTexCoord;
out vec3 vNormal;

void main() {
    vColor = color;
    vTexCoord = texcoord;
    vNormal = normal;
    gl_Position = modelViewProjectionMatrix * position;
}
