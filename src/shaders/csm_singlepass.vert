#version 330 core
#ifdef ENABLE_ALPHA_TESTING

layout (location = 0) in vec3 in_pos;
layout (location = 2) in vec2 in_uv;

uniform mat4 model;

out Interface
{
    vec2 uv;
} out_;


void main()
{
    out_.uv     = in_uv;
    gl_Position = model * vec4(in_pos, 1.0);
}


#else // !ENABLE_ALPHA_TESTING

layout (location = 0) in vec3 in_pos;

uniform mat4 model;


void main()
{
    gl_Position = model * vec4(in_pos, 1.0);
}


#endif // ENABLE_ALPHA_TESTING
