#version 460 core

layout (location = 0) in vec3 in_pos;

layout (std140, binding = 0) restrict readonly
buffer InstanceTransforms
{
    mat4 instance_transforms[];
};

uniform mat4 projection;
uniform mat4 view;


void main()
{
    const mat4 model = instance_transforms[gl_DrawID];
    gl_Position = projection * view * model * vec4(in_pos, 1.0);
}
