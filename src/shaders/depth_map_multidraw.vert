#version 460 core

layout (location = 0) in vec3 in_pos;


layout (std430, binding = 0) restrict readonly
buffer WorldMatrixBlock {
    mat4 W2Ls[];
};


uniform mat4 projection;
uniform mat4 view;


void main() {
    const mat4 model = W2Ls[gl_DrawID];
    gl_Position = projection * view * model * vec4(in_pos, 1.0);
}
