#version 330 core

in vec2 tex_coords;

uniform sampler2D color;

out vec4 frag_color;


void main()
{
    frag_color = texture(color, tex_coords);
}


