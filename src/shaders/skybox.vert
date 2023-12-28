#version 330 core

layout (location = 0) in vec3 in_pos;

out vec3 tex_coords;

uniform mat4 projection;
uniform mat4 view;


void main() {
    // There are 2 options that I see of how to deal with the cubemaps:
    //
    // 1. Textures are flipped for Y coordinate and then the Z axis is negated
    // in the shader.
    //
    // This is intrusive because the cubemap textures are now special
    // and are flipped, or are going to be wasting resources on copying the texture
    // to reverse the coordinates every time you load a cubemap face.
    //
    // tex_coords = vec3(1.0, 1.0, -1.0) * in_pos;

    // 2. The textures for the +Y and -Y faces are swapped when
    // attaching them to the cubemaps. The sampling direction is rotated by 180 degrees
    // around the X axis (which can be done by negating Y and Z).
    //
    tex_coords = vec3(-1.0, -1.0, 1.0) * in_pos;

    // Oh, yeah, also, forgot to say, but OpenGL is damn insane for using a LH system for cubemaps.

    vec4 position = projection * view * vec4(in_pos, 1.0);
    gl_Position = position.xyww; // Force set z = 1.0 in NDC
}
