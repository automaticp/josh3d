#version 330 core

out vec4 frag_color;

in vec2 tex_coords;

uniform sampler2D tex_position_draw;
uniform sampler2D tex_normals;
uniform sampler2D tex_albedo_spec;
uniform sampler2D tex_depth;

uniform float z_near;
uniform float z_far;

/*
enum class OverlayMode : GLint {
    none         = 0,
    albedo       = 1,
    specular     = 2,
    position     = 3,
    depth        = 4,
    depth_linear = 5,
    normals      = 6,
    draw_region  = 7,
};
*/
uniform int mode = 0;



float get_linear_depth(float screen_depth) {
    // screen_depth = (1/z - 1/n) / (1/f - 1/n)
    return z_near / (z_far - screen_depth * (z_far - z_near));
}


void main() {
    vec3 out_color;

    switch (mode) {
        case 1: // albedo
            // TODO: Proper sRGB?
            out_color = pow(texture(tex_albedo_spec, tex_coords).rgb, vec3(1.0 / 2.2));
            break;
        case 2: // specular
            out_color = vec3(texture(tex_albedo_spec, tex_coords).a);
            break;
        case 3: // position
            out_color = texture(tex_position_draw, tex_coords).xyz;
            break;
        case 4: // depth
            out_color = vec3(texture(tex_depth, tex_coords).r);
            break;
        case 5: // depth_linear
            out_color = vec3(get_linear_depth(texture(tex_depth, tex_coords).r));
            break;
        case 6: // normals
            out_color = 0.5 * (1.0 + texture(tex_normals, tex_coords).rgb);
            break;
        case 7: // draw_region
            out_color = vec3(texture(tex_position_draw, tex_coords).a);
            break;
        default:
            out_color = texture(tex_albedo_spec, tex_coords).rgb;
    }

    frag_color = vec4(out_color, 1.0);
}


