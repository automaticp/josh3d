#pragma once
#include "Attachments.hpp"
#include "GLAPIBinding.hpp"
#include "GLObjects.hpp"
#include "GLTextures.hpp"
#include "ShaderPool.hpp"
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

    void operator()(RenderEnginePrimaryInterface& engine);

private:
    ShaderToken sp_ = shader_pool().get({
        .vert = VPath("src/shaders/light_dummies_point.vert"),
        .frag = VPath("src/shaders/light_dummies_point.frag")});

    // FIXME: This exposes a dumb design where suddenly light
    // dummies cannot be drawn if there's no ID buffer???
    // We should just go back to framebuffers, it is much simpler.
    // using Target = RenderTarget<
    //     SharedAttachment<Renderable::Texture2D>, // Depth
    //     SharedAttachment<Renderable::Texture2D>, // Color
    //     SharedAttachment<Renderable::Texture2D>  // ObjectID
    // >;

    // Optional<Target> target_;

    UniqueFramebuffer target_;

    void relink_attachments(RenderEnginePrimaryInterface& engine);

    struct PLightParamsGPU {
        alignas(std430::align_vec3)  vec3  position;
        alignas(std430::align_float) float scale;
        alignas(std430::align_vec3)  vec3  color;
        alignas(std430::align_uint)  uint  id;
    };

    UploadBuffer<PLightParamsGPU> plight_params_;

    void update_plight_params(const entt::registry& registry);

};


inline void LightDummies::operator()(RenderEnginePrimaryInterface& engine) {

    if (not display) return;

    relink_attachments(engine);
    update_plight_params(engine.registry());

    const auto num_plights = GLsizei(plight_params_.num_staged());

    if (num_plights) {
        BindGuard bound_camera_ubo      = engine.bind_camera_ubo();
        BindGuard bound_instance_buffer = plight_params_.bind_to_ssbo_index(0);

        BindGuard bound_program = sp_.get().use();
        BindGuard bound_fbo     = target_->bind_draw();

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

            const float shell_attenuation =
                1.f / (4.f * glm::pi<float>() * light_scale * light_scale);

            const auto color = plight.hdr_color() *
                (attenuate_color ? shell_attenuation : 1.f);

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
    // TODO: Should be able to query the current attachment, no?
    // We just attach every frame for now.

    // NOTE: This is exactly the case where `const` brings unneccessary
    // amounts of friction. RenderTarget is very hostile to giving mutable
    // access to textures, which we sorta-kinda need for framebuffers.
    auto unconst = [](RawTexture2D<GLConst> texture) { return RawTexture2D<>::from_id(texture.id()); };

    target_->attach_texture_to_depth_buffer(unconst(engine.main_depth_attachment().texture()));
    target_->attach_texture_to_color_buffer(unconst(engine.main_color_attachment().texture()), 0);
    if (auto* idbuffer = engine.belt().try_get<IDBuffer>())
        target_->attach_texture_to_color_buffer(unconst(idbuffer->object_id_texture()), 1);
}


} // namespace josh::stages::primary
