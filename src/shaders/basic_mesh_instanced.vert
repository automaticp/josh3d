#version 430 core

layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec2 in_tex_coords;


layout (std430, binding = 0) restrict readonly buffer model_layout {
    mat4 models[];
};


uniform mat4 projection;
uniform mat4 view;


out vec2 tex_coords;
out vec3 normal;
out vec3 frag_pos;


void main() {
    mat4 model = models[gl_InstanceID];

    tex_coords = in_tex_coords;

    frag_pos = vec3(model * vec4(in_pos, 1.0));

    gl_Position = projection * view * vec4(frag_pos, 1.0);

    // FIXME: do on cpu?
    // Will have to transfer a whole other SSBO.
    normal = mat3(transpose(inverse(model))) * in_normal;
}
