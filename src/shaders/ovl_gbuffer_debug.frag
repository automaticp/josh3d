#version 430 core
#extension GL_GOOGLE_include_directive : enable
#include "utils.coordinates.glsl"
#include "utils.color.glsl"




in vec2  tex_coords;
out vec4 frag_color;

uniform sampler2D tex_position_draw;
uniform sampler2D tex_normals;
uniform sampler2D tex_albedo_spec;
uniform sampler2D tex_depth;
uniform usampler2D tex_object_id;

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
    object_id    = 8,
};
*/
uniform int mode = 0;




vec3 get_color_from_id(uint id) {
    if (id == 0xFFFFFFFFu) /* entt::null */ { return vec3(0.0); }

    const uint entity_mask = 0x000FFFFFu;

    // Remove version info.
    id &= entity_mask;

    const uint d   = 0xFu;  // Hue position
    const uint dv  = 0xFu;  // Value falloff
    const uint dp  = 0xFu;  // Rectangular pattern
    const uint dh  = 0xFFu; // Hue offset

    uint bits = id;


    // Each "d" subsequent objects will be mapped across Hue from 0 to 1.
    float hue_pos = float(bits % d) / float(d);
    bits /= d;  // up to 0x0000FFFFu;


    // Value falls with each "d" block from 1.0 to 0.3 for "dv" bins.
    // Then after "d * dv" ids the value of value is reset to 1.0 and it falls again.
    uint value_bin = bits % dv;
    float value = (1.0 - float(value_bin) / float(dv)) * 0.7 + 0.3;

    bits /= dv; // up to 0x00000FFFu;

    // After first 256 objects we start adding a pattern, which grows in size
    // with each next 256 objects, repeating after 16 times.
    uvec4 crds = uvec4(gl_FragCoord);
    bool pattern_cond =
        (crds.x % d) >= (bits % dp) &&
        (crds.y % d) >= (bits % dp);

    vec3 pattern_dim = pattern_cond ? vec3(1.0) : vec3(0.9);

    bits /= dp; // up to 0x000000FFu;


    // Remaining byte is encoded with a Hue offset.
    // Probably not visible, but who cares at this point, eh.
    float offset = float(bits) / float(dh);


    return hsv2rgb(vec3(hue_pos + offset, 1.0, value)) * pattern_dim;
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
            out_color = mod((texture(tex_position_draw, tex_coords).xyz + 1.0), 20.0) * 0.05;
            break;
        case 4: // depth
            out_color = vec3(texture(tex_depth, tex_coords).r);
            break;
        case 5: // depth_linear
            out_color = vec3(get_linear_0to1_depth(texture(tex_depth, tex_coords).r, z_near, z_far));
            break;
        case 6: // normals
            out_color = 0.5 * (1.0 + texture(tex_normals, tex_coords).rgb);
            break;
        case 7: // draw_region
            out_color = vec3(texture(tex_position_draw, tex_coords).a);
            break;
        case 8: // object_id
            out_color = vec3(get_color_from_id(textureLod(tex_object_id, tex_coords, 0).r));
            break;
        default:
            out_color = texture(tex_albedo_spec, tex_coords).rgb;
    }

    frag_color = vec4(out_color, 1.0);
}


