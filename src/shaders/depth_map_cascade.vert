#version 330 core

layout (location = 0) in vec3 in_pos;

uniform mat4 model;



#ifdef ENABLE_ALPHA_TESTING

layout (location = 2) in vec2 in_tex_coords;

out Interface {
    vec2 tex_coords;
} out_;

#endif // ENABLE_ALPHA_TESTING



void main() {

#ifdef ENABLE_ALPHA_TESTING
    out_.tex_coords = in_tex_coords;
#endif // ENABLE_ALPHA_TESTING

    gl_Position = model * vec4(in_pos, 1.0);
}
