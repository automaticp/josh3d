#version 330 core
#ifdef ENABLE_ALPHA_TESTING

layout (location = 0) in vec3 in_pos;
layout (location = 2) in vec2 in_uv;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

out vec2 uv;


void main()
{
    uv = in_uv;
    gl_Position = projection * view * model * vec4(in_pos, 1.0);
}


#else // !ENABLE_ALPHA_TESTING

layout (location = 0) in vec3 in_pos;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;


void main()
{
    gl_Position = projection * view * model * vec4(in_pos, 1.0);
}


#endif // ENABLE_ALPHA_TESTING
