#version 460 core
#extension GL_GOOGLE_include_directive : enable
#include "camera_ubo.glsl"



// Expects: glDrawArrays(GL_LINES, 0, 2 * num_lines);


struct Line {
    vec3 start;
    vec3 end;
};


layout(std430, binding = 0) restrict readonly
buffer LinesBlock {
    Line lines[];
};


out Interface {
    float t; // Line parameter [0, 1] - how far along each line the pixel is.
} out_;




void main() {
    const Line line     = lines[gl_VertexID / 2];
    const bool is_start = bool(gl_VertexID % 2);
    const vec3 vert_pos = is_start ? line.start : line.end;

    out_.t      = float(is_start);
    gl_Position = camera.projview * vec4(vert_pos, 1.0);
}
