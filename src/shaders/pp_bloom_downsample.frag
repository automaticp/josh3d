#version 460 core
in  vec2 tex_coords;
out vec3 frag_color;


uniform sampler2D source;


vec3 weighted(vec3 samples[13]) {

    const vec3 a = samples[ 0];
    const vec3 b = samples[ 1];
    const vec3 c = samples[ 2];
    const vec3 d = samples[ 3];
    const vec3 e = samples[ 4];
    const vec3 f = samples[ 5];
    const vec3 g = samples[ 6];
    const vec3 h = samples[ 7];
    const vec3 i = samples[ 8];
    const vec3 j = samples[ 9];
    const vec3 k = samples[10];
    const vec3 l = samples[11];
    const vec3 m = samples[12];

    const vec3 result =
        0.125   * (e)             +
        0.03125 * (a + c + g + i) +
        0.0625  * (b + d + f + h) +
        0.125   * (j + k + l + m);

    return result;
}


void main() {
    const int  lod     = 0; // Base lod is set in the application.
    const vec2 uv      = tex_coords;
    const vec2 px_size = 1.0 / textureSize(source, lod);

    const ivec2 offsets[13] = {
        ivec2(-2, +2),
        ivec2( 0, +2),
        ivec2(+2, +2),
        ivec2(-2,  0),
        ivec2( 0,  0),
        ivec2(+2,  0),
        ivec2(-2, -2),
        ivec2( 0, -2),
        ivec2(+2, -2),
        ivec2(-1, +1),
        ivec2(+1, +1),
        ivec2(-1, -1),
        ivec2(+1, -1),
    };

    const vec3 samples[13] = {
        textureLodOffset(source, uv, lod, offsets[ 0]).rgb,
        textureLodOffset(source, uv, lod, offsets[ 1]).rgb,
        textureLodOffset(source, uv, lod, offsets[ 2]).rgb,
        textureLodOffset(source, uv, lod, offsets[ 3]).rgb,
        textureLodOffset(source, uv, lod, offsets[ 4]).rgb,
        textureLodOffset(source, uv, lod, offsets[ 5]).rgb,
        textureLodOffset(source, uv, lod, offsets[ 6]).rgb,
        textureLodOffset(source, uv, lod, offsets[ 7]).rgb,
        textureLodOffset(source, uv, lod, offsets[ 8]).rgb,
        textureLodOffset(source, uv, lod, offsets[ 9]).rgb,
        textureLodOffset(source, uv, lod, offsets[10]).rgb,
        textureLodOffset(source, uv, lod, offsets[11]).rgb,
        textureLodOffset(source, uv, lod, offsets[12]).rgb,
    };

    frag_color = weighted(samples);

}
