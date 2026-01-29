// ofxVase Vertex Shader - Pass through with dual UV coordinates
#version 150

uniform mat4 modelViewProjectionMatrix;

in vec4 position;
in vec4 color;
in vec2 texcoord;   // Primary UV (position in stroke)
in vec2 texcoord2;  // Secondary UV (fade control / rr values)

out vec4 vColor;
out vec2 vTexCoord;
out vec2 vTexCoord2;

void main() {
    vColor = color;
    vTexCoord = texcoord;
    vTexCoord2 = texcoord2;
    gl_Position = modelViewProjectionMatrix * position;
}
