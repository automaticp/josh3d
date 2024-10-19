#version 460 core


out vec4 frag_color;


in Interface {
    float t;
} in_;


uniform vec4  color; // Blending possible.
uniform float dash_size;


void main() {

    if (int(in_.t / dash_size) % 2 != 0) discard;

    frag_color = color;
}
