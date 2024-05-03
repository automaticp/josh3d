// #version 430 core


struct GBuffer {
    sampler2D tex_position_draw;
    sampler2D tex_normals;
    sampler2D tex_albedo_spec;
};


struct GSample {
    vec3  position;
    bool  drawn;
    vec3  normal;
    vec3  albedo;
    float specular;
};


GSample unpack_gbuffer(GBuffer gbuffer, vec2 uv) {
    vec4 pos_draw    = textureLod(gbuffer.tex_position_draw, uv, 0);
    vec4 normals     = textureLod(gbuffer.tex_normals,       uv, 0);
    vec4 albedo_spec = textureLod(gbuffer.tex_albedo_spec,   uv, 0);
    return GSample(
        pos_draw.xyz,
        pos_draw.a != 0.0,
        normals.xyz,
        albedo_spec.rgb,
        albedo_spec.a
    );
}

