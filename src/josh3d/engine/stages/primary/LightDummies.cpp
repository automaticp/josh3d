#include "LightDummies.hpp"
#include "ECS.hpp"
#include "GLAPIBinding.hpp"
#include "GLObjects.hpp"
#include "Ranges.hpp"
#include "ShaderPool.hpp"
#include "Transform.hpp"
#include "UniformTraits.hpp" // IWYU pragma: keep (traits)
#include "LightCasters.hpp"
#include "RenderEngine.hpp"
#include "UploadBuffer.hpp"
#include "Mesh.hpp"
#include "stages/primary/IDBufferStorage.hpp"
#include <entt/entity/entity.hpp>
#include <entt/entity/fwd.hpp>
#include <entt/entt.hpp>
#include <ranges>


namespace josh::stages::primary {

void LightDummies::operator()(RenderEnginePrimaryInterface& engine)
{
    if (not display) return;

    _relink_attachments(engine);
    _restage_plight_params(engine.registry());

    const auto num_plights = GLsizei(_plight_params.num_staged());

    if (num_plights)
    {
        const BindGuard bound_camera_ubo      = engine.bind_camera_ubo();
        const BindGuard bound_instance_buffer = _plight_params.bind_to_ssbo_index(0);

        const Mesh&     mesh = engine.primitives().sphere_mesh();
        const BindGuard bsp = _sp.get().use();
        const BindGuard bfb = _fbo->bind_draw();
        const BindGuard bva = mesh.vertex_array().bind();

        glapi::draw_elements_instanced(
            bva, bsp, bfb,
            num_plights,
            mesh.primitive_type(),
            mesh.element_type(),
            mesh.element_offset_bytes(),
            mesh.num_elements()
        );
    }
}

void LightDummies::_restage_plight_params(const Registry& registry)
{
    auto plight_params_view =
        registry.view<PointLight, MTransform>().each() |
        transform([this](auto tuple)
        {
            const auto [entity, plight, mtf] = tuple;

            const float shell_attenuation =
                1 / (4 * glm::pi<float>() * light_scale * light_scale);

            const vec3 color = plight.hdr_color() *
                (attenuate_color ? shell_attenuation : 1);

            return PLightParamsGPU{
                .position = mtf.decompose_position(),
                .scale    = light_scale,
                .color    = color,
                .id       = to_integral(entity),
            };
        });

    _plight_params.restage(plight_params_view);
}

void LightDummies::_relink_attachments(RenderEnginePrimaryInterface& engine)
{
    // TODO: Should be able to query the current attachment, no?
    // We just attach every frame for now. Whatever.

    _fbo->attach_texture_to_depth_buffer(engine.main_depth_texture());
    _fbo->attach_texture_to_color_buffer(engine.main_color_texture(), 0);
    if (auto* idbuffer = engine.belt().try_get<IDBuffer>())
        _fbo->attach_texture_to_color_buffer(idbuffer->object_id_texture(), 1);
}


} // namespace josh::stages::primary
