#pragma once
#include "ECS.hpp"
#include "GLObjects.hpp"
#include "ShaderPool.hpp"
#include "RenderEngine.hpp"
#include "UploadBuffer.hpp"
#include "VPath.hpp"


namespace josh {


struct LightDummies
{
    bool  display         = true;
    float light_scale     = 0.1f;
    bool  attenuate_color = true;

    void operator()(RenderEnginePrimaryInterface& engine);

    struct PLightParamsGPU
    {
        alignas(std430::align_vec3)  vec3  position;
        alignas(std430::align_float) float scale;
        alignas(std430::align_vec3)  vec3  color;
        alignas(std430::align_uint)  uint  id;
    };

    UploadBuffer<PLightParamsGPU> _plight_params;
    UniqueFramebuffer             _fbo;

    void _restage_plight_params(const Registry& registry);
    void _relink_attachments(RenderEnginePrimaryInterface& engine);

    ShaderToken _sp = shader_pool().get({
        .vert = VPath("src/shaders/light_dummies_point.vert"),
        .frag = VPath("src/shaders/light_dummies_point.frag")});
};


} // namespace josh
