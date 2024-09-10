#version 460 core
in  vec2 tex_coords;
out vec3 frag_color;


uniform sampler2D source;
uniform float     filter_scale_px;


vec3 weighted(vec3 samples[9]) {
    const vec3 a = samples[0];
    const vec3 b = samples[1];
    const vec3 c = samples[2];
    const vec3 d = samples[3];
    const vec3 e = samples[4];
    const vec3 f = samples[5];
    const vec3 g = samples[6];
    const vec3 h = samples[7];
    const vec3 i = samples[8];

    const vec3 sum =
        4.0 * (e)             +
        2.0 * (b + d + f + h) +
        1.0 * (a + c + g + i);

    return sum / 16.0;
}


void main() {
    const int  lod     = 0; // Base lod is set in the application.
    const vec2 uv      = tex_coords;
    const vec2 px_size = 1.0 / textureSize(source, lod);
    const vec2 o       = filter_scale_px * px_size;

    const vec3 samples[9] = {
        textureLod(source, vec2(uv.x - o.x, uv.y + o.y), lod).rgb,
        textureLod(source, vec2(uv.x,       uv.y + o.y), lod).rgb,
        textureLod(source, vec2(uv.x + o.x, uv.y + o.y), lod).rgb,
        textureLod(source, vec2(uv.x - o.x, uv.y      ), lod).rgb,
        textureLod(source, vec2(uv.x,       uv.y      ), lod).rgb,
        textureLod(source, vec2(uv.x + o.x, uv.y      ), lod).rgb,
        textureLod(source, vec2(uv.x - o.x, uv.y - o.y), lod).rgb,
        textureLod(source, vec2(uv.x,       uv.y - o.y), lod).rgb,
        textureLod(source, vec2(uv.x + o.x, uv.y - o.y), lod).rgb,
    };

    frag_color = weighted(samples);
}
