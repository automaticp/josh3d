#pragma once
#include "Attachments.hpp"
#include "GLAPIBinding.hpp"
#include "GLBuffers.hpp"
#include "GLObjectHelpers.hpp"
#include "GLObjects.hpp"
#include "SharedStorage.hpp"
#include "UniformTraits.hpp" // IWYU pragma: keep (traits)
#include "LightCasters.hpp"
#include "RenderEngine.hpp"
#include "ShaderBuilder.hpp"
#include "VPath.hpp"
#include "Mesh.hpp"
#include "stages/primary/IDBufferStorage.hpp"
#include <algorithm>
#include <entt/entity/entity.hpp>
#include <entt/entity/fwd.hpp>
#include <entt/entt.hpp>




namespace josh::stages::primary {


class LightDummies {
public:
    bool  display{ true };
    float light_scale{ 0.1f };
    bool  attenuate_color{ false };

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
    UniqueProgram sp_{
        ShaderBuilder()
            .load_vert(VPath("src/shaders/light_dummies_point.vert"))
            .load_frag(VPath("src/shaders/light_dummies_point.frag"))
            .get()
    };

    using Target = RenderTarget<
        SharedAttachment<Renderable::Texture2D>, // Depth
        SharedAttachment<Renderable::Texture2D>, // Color
        SharedAttachment<Renderable::Texture2D>  // ObjectID
    >;

    Target target_;

    SharedStorageMutableView<IDBuffer> idbuffer_;

    void relink_attachments(RenderEnginePrimaryInterface& engine);

    struct PLightParams {
        alignas(std430::align_vec3)  glm::vec3  position;
        alignas(std430::align_float) float      scale;
        alignas(std430::align_vec3)  glm::vec3  color;
        alignas(std430::align_uint)  uint       id;
    };

    UniqueBuffer<PLightParams> plight_params_;

    void update_plight_params(const entt::registry& registry);

};




inline void LightDummies::operator()(RenderEnginePrimaryInterface& engine) {

    if (!display) { return; }

    relink_attachments(engine);
    update_plight_params(engine.registry());
    const auto num_plights = GLsizei(plight_params_->get_num_elements());

    if (num_plights) {
        BindGuard bound_camera_ubo = engine.bind_camera_ubo();

        plight_params_->bind_to_index<BufferTargetIndexed::ShaderStorage>(0);

        BindGuard bound_program = sp_->use();
        BindGuard bound_fbo     = target_.bind_draw();

        const Mesh& mesh = engine.primitives().sphere_mesh();
        BindGuard bound_vao{ mesh.vertex_array().bind() };

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
    auto plights_view = registry.view<PointLight>();
    const size_t num_plights = plights_view.size();

    resize_to_fit(plight_params_, NumElems{ num_plights });

    if (num_plights) {

        auto mapped = plight_params_->map_for_write();
        do {

            auto params_view = plights_view.each() |
                std::views::transform([this](auto each) {
                    auto& [e, plight] = each;
                    auto light_color =
                        attenuate_color ?
                            // This doesn't really look that great...
                            plight.color * plight.attenuation.get_attenuation(light_scale) :
                            plight.color;
                    return PLightParams{
                        .position = plight.position,
                        .scale    = light_scale,
                        .color    = light_color,
                        .id       = entt::to_integral(e)
                    };
                });
            std::ranges::copy(params_view, mapped.begin());

        } while (!plight_params_->unmap_current());

    }
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
