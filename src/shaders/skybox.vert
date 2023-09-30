#version 330 core

layout (location = 0) in vec3 in_pos;

out vec3 tex_coords;

uniform mat4 projection;
uniform mat4 view;


void main() {
    // Cubemaps amright
    tex_coords = vec3(in_pos.xy, -in_pos.z);

    vec4 position = projection * view * vec4(in_pos, 1.0);
    gl_Position = position.xyww; // Force set z = 1.0 in NDC
}
