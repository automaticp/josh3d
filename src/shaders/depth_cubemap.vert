#version 330 core

layout (location = 0) in vec3 in_pos;


#ifdef ENABLE_ALPHA_TESTING

layout (location = 2) in vec2 in_tex_coords;
out vec2 in_geom_tex_coords;

#endif // ENABLE_ALPHA_TESTING


uniform mat4 model;


void main() {

#ifdef ENABLE_ALPHA_TESTING
    in_geom_tex_coords = in_tex_coords;
#endif // ENABLE_ALPHA_TESTING

    gl_Position = /* projection * view * */ model * vec4(in_pos, 1.0);
}
