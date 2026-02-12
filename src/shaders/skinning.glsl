#ifndef SKINNING_GLSL
#define SKINNING_GLSL


#ifndef SKIN_MATRICES_SSBO_BINDING
#define SKIN_MATRICES_SSBO_BINDING 0
#endif

layout (std430, binding = SKIN_MATRICES_SSBO_BINDING) restrict readonly
buffer SkinMatricesBlock
{
    mat4 skin_mats[];
} _skmb;


mat4 compute_skin_matrix(uvec4 joint_ids, vec4 joint_weights)
{
    return
        joint_weights[0] * _skmb.skin_mats[joint_ids[0]] +
        joint_weights[1] * _skmb.skin_mats[joint_ids[1]] +
        joint_weights[2] * _skmb.skin_mats[joint_ids[2]] +
        joint_weights[3] * _skmb.skin_mats[joint_ids[3]];
}


#endif
