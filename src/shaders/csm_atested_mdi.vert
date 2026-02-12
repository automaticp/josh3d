#version 460 core

layout (location = 0) in vec3 in_pos;
layout (location = 2) in vec2 in_uv;

layout (std430, binding = 0) restrict readonly
buffer InstanceDataBlock
{
    mat4 instance_transforms[];
};

uniform mat4 projection; // NOTE: "Shadow camera" params.
uniform mat4 view;       // "

out flat uint draw_id;
out      vec2 uv;


void main()
{
    draw_id = gl_DrawID;
    const mat4 model = instance_transforms[draw_id];

    uv          = in_uv;
    gl_Position = projection * view * model * vec4(in_pos, 1.0);
}
