#version 430 core

in vec2 uv;

uniform sampler2D noisy_occlusion;

out float frag_color;


void main()
{
    const vec2 tx_size = 1.0 / textureSize(noisy_occlusion, 0).xy;

    // Shitty box blur here we gooo
    float occlusion = 0.0;
    for (int x = -2; x <= 2; ++x)
    for (int y = -2; y <= 2; ++y)
    {
        const vec2 offset = vec2(float(x), float(y)) * tx_size;
        occlusion += texture(noisy_occlusion, uv + offset).r;
    }

    frag_color = occlusion / 25.f;
}
