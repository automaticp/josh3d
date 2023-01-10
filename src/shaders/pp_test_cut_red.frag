#version 330 core

out vec4 frag_color;

in vec2 tex_coords;

uniform sampler2D color;

void main() {
    frag_color = texture(color, tex_coords);
    if (gl_FragCoord.x < 200) {
        frag_color *= vec4(1.0, 0.0, 0.0, 1.0);
    }
}
