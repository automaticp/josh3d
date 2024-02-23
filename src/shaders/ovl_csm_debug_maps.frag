#version 330 core

out vec4 frag_color;

in vec2 tex_coords;

uniform sampler2DArray cascades;
uniform uint cascade_id;

void main() {
    float depth = texture(cascades, vec3(tex_coords, cascade_id)).r;
    // TODO: Have to linearize the depth though.
    frag_color = vec4(vec3(depth), 1.0);
}


