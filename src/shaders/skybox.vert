#version 330 core

layout (location = 0) in vec3 in_pos;

out vec3 tex_coords;

uniform mat4 projection;
uniform mat4 view;


mat4 translationless(mat4 view) {
    return mat4(mat3(view));
}

void main() {
    // Cubemaps amright
    tex_coords = vec3(in_pos.xy, -in_pos.z);

    gl_Position = projection * translationless(view) * vec4(in_pos, 1.0);
}
