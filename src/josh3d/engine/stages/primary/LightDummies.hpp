#pragma once
#include "Attachments.hpp"
#include "GLAPIBinding.hpp"
#include "GLObjects.hpp"
#include "ShaderPool.hpp"
#include "SharedStorage.hpp"
#include "Transform.hpp"
#include "UniformTraits.hpp" // IWYU pragma: keep (traits)
#include "LightCasters.hpp"
#include "RenderEngine.hpp"
#include "UploadBuffer.hpp"
#include "VPath.hpp"
#include "Mesh.hpp"
#include "stages/primary/IDBufferStorage.hpp"
#include <entt/entity/entity.hpp>
#include <entt/entity/fwd.hpp>
#include <entt/entt.hpp>
#include <ranges>




namespace josh::stages::primary {


class LightDummies {
public:
    bool  display{ true };
    float light_scale{ 0.1f };
    bool  attenuate_color{ true };

    LightDummies(
        SharedAttachment<Renderable::Texture2D> depth,
        SharedAttachment<Renderable::Texture2D> color,
        SharedStorageMutableView<IDBuffer>      idbuffer)
        : target_{
            std::move(depth),
            std::move(color),
            idbuffer->share_object_id_attachment()
        }
        , idbuffer_{ std::move(idbuffer) }
    {}

    void operator()(RenderEnginePrimaryInterface& engine);

private:
    ShaderToken sp_ = shader_pool().get({
        .vert = VPath("src/shaders/light_dummies_point.vert"),
        .frag = VPath("src/shaders/light_dummies_point.frag"),
    });

    using Target = RenderTarget<
        SharedAttachment<Renderable::Texture2D>, // Depth
        SharedAttachment<Renderable::Texture2D>, // Color
        SharedAttachment<Renderable::Texture2D>  // ObjectID
    >;

    Target target_;

    SharedStorageMutableView<IDBuffer> idbuffer_;

    void relink_attachments(RenderEnginePrimaryInterface& engine);

    struct PLightParamsGPU {
        alignas(std430::align_vec3)  glm::vec3  position;
        alignas(std430::align_float) float      scale;
        alignas(std430::align_vec3)  glm::vec3  color;
        alignas(std430::align_uint)  uint       id;
    };

    UploadBuffer<PLightParamsGPU> plight_params_;

    void update_plight_params(const entt::registry& registry);

};




inline void LightDummies::operator()(RenderEnginePrimaryInterface& engine) {

    if (!display) { return; }

    relink_attachments(engine);
    update_plight_params(engine.registry());

    const auto num_plights = plight_params_.num_staged();

    if (num_plights) {
        BindGuard bound_camera_ubo      = engine.bind_camera_ubo();
        BindGuard bound_instance_buffer = plight_params_.bind_to_ssbo_index(0);

        BindGuard bound_program = sp_.get().use();
        BindGuard bound_fbo     = target_.bind_draw();

        const Mesh& mesh = engine.primitives().sphere_mesh();
        BindGuard bound_vao = mesh.vertex_array().bind();

        glapi::draw_elements_instanced(
            bound_vao,
            bound_program,
            bound_fbo,
            num_plights,
            mesh.primitive_type(),
            mesh.element_type(),
            mesh.element_offset_bytes(),
            mesh.num_elements()
        );

    }
}




inline void LightDummies::update_plight_params(
    const entt::registry& registry)
{
    auto plight_params_view = registry.view<PointLight, MTransform>().each()
        | std::views::transform([this](auto tuple) {
            const auto& [entity, plight, mtf] = tuple;
            const auto color = plight.color *
                // This doesn't really look that great...
                (attenuate_color ? plight.attenuation.get_attenuation(light_scale) : 1.f);
            return PLightParamsGPU{
                .position = mtf.decompose_position(),
                .scale    = light_scale,
                .color    = color,
                .id       = to_integral(entity),
            };
        });

    plight_params_.restage(plight_params_view);
}




inline void LightDummies::relink_attachments(
    RenderEnginePrimaryInterface& engine)
{
    if (!target_.depth_attachment().is_shared_from(engine.main_depth_attachment())) {
        target_.reset_depth_attachment(engine.share_main_depth_attachment());
    }

    if (!target_.color_attachment<0>().is_shared_from(engine.main_color_attachment())) {
        target_.reset_color_attachment<0>(engine.share_main_color_attachment());
    }

    if (!target_.color_attachment<1>().is_shared_from(idbuffer_->object_id_attachment())) {
        target_.reset_color_attachment<1>(idbuffer_->share_object_id_attachment());
    }
}


} // namespace josh::stages::primary
