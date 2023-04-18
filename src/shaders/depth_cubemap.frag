#version 330 core

in vec4 frag_pos;

uniform vec3 light_pos;
uniform float z_far;


void main() {
    float light_to_frag_distance =
        length(frag_pos.xyz - light_pos) / z_far;

    gl_FragDepth = light_to_frag_distance;
}
