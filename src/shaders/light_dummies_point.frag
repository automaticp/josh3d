#version 430 core

in flat vec3 light_color;
in flat uint light_id;

layout (location = 0) out vec4 frag_color;
layout (location = 1) out uint object_id;


void main() {
    frag_color = vec4(light_color, 1.0);
    object_id  = light_id;
}
