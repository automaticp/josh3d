#version 460 core

layout (location = 0) in vec3 in_pos;

layout (std430, binding = 0) restrict readonly
buffer InstanceDataBlock
{
    mat4 instance_transforms[];
};

uniform mat4 projection; // NOTE: "Shadow camera" params.
uniform mat4 view;       // "


void main()
{
    const mat4 model = instance_transforms[gl_DrawID];
    gl_Position = projection * view * model * vec4(in_pos, 1.0);
}
