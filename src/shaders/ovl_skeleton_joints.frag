#version 430 core
in  vec3 normal;
out vec4 frag_color;

uniform vec3 color = vec3(0.5);

void main() {
    frag_color = vec4(color, 1.0);
}
