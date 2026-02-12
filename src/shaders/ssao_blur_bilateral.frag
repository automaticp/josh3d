#version 430 core
#extension GL_GOOGLE_include_directive : enable
#include "utils.coordinates.glsl"
#include "camera_ubo.glsl"

in vec2 uv;

layout (std430, binding = 0) restrict readonly
buffer GaussianKernelWeightsBlock
{
    // Not required to be normalized. We renormalize here anyway.
    // Size N = (2*L + 1), where L is the limb size. N must be odd.
    float kernel_weights[];
};

uniform sampler2D noisy_occlusion;
uniform sampler2D depth;
uniform float     depth_limit; // In world-space units. Difference of depths above this will give 0 occlusion.
uniform bool      blur_dim;    // 0 for x, 1 for y.

out float frag_color;


float linear_depth(vec2 uv)
{
    const float depth_nss = texture(depth, uv).r;
    return get_view_space_depth(depth_nss, camera.z_near, camera.z_far);
}

void main()
{
    const vec2  tx_size      = 1.0 / textureSize(noisy_occlusion, 0).xy;
    const vec2  dim_mask     = vec2(equal(bvec2(blur_dim), bvec2(0, 1)));
    const int   kernel_size  = kernel_weights.length();
    const int   limb_size    = (kernel_size - 1) / 2;

    const float center_depth = linear_depth(uv);

    float total_occlusion = 0;
    float norm            = 0;

    // NOTE: `h` is either x or y depending on blur_dim.
    for (int h = -limb_size; h <= limb_size; ++h)
    {
        const vec2  offset    = dim_mask * vec2(h, h) * tx_size;
        const vec2  sample_uv = uv + offset;

        const float occlusion     = texture(noisy_occlusion, sample_uv).r;
        const int   bin_idx       = h + limb_size;
        const float kernel_weight = kernel_weights[bin_idx];

        // FIXME: This somewhat works, but blending the occlusion buffer at half-resolution
        // creates ugly aliasing at the edges of objects. It looks bad enough that it makes
        // non depth-aware blur look like a decent choice...

        // NOTE: Only the occlusion is using gaussian weights.
        // For depth-awareness a simple smoothstep is enough.
        const float depth        = linear_depth(sample_uv);
        const float depth_delta  = abs(center_depth - depth);
        const float depth_factor = 1.0 - smoothstep(0.0, 1.0, depth_delta / depth_limit);

        const float weight = kernel_weight * depth_factor;

        norm            += weight;
        total_occlusion += occlusion * weight;
    }

    frag_color = total_occlusion / norm;
}
