#pragma once
#include "Attachments.hpp"
#include "Camera.hpp"
#include "Layout.hpp"
#include "RenderEngine.hpp"
#include "RenderTarget.hpp"
#include "ShaderBuilder.hpp"
#include "SharedStorage.hpp"
#include "VPath.hpp"
#include "ViewFrustum.hpp"
#include <entt/entity/fwd.hpp>
#include <glbinding/gl/enum.h>
#include <glm/glm.hpp>
#include <cassert>
#include <vector>


namespace josh::stages::primary {


class CascadedShadowMapping {
public:
    enum class Strategy {
        SinglepassGS,
        PerCascadeCulling
    };

    Strategy strategy{ Strategy::PerCascadeCulling };

    Size2I resolution{ 2048, 2048 }; // TODO: Only rectangular is allowed anyway, I think.

    // Limited in [1, max_cascades()].
    auto num_cascades() const noexcept -> size_t;

    // Due to implementation limits.
    auto max_cascades() const noexcept -> size_t;

    // Will not exceed max_cascades().
    void set_num_cascades(size_t desired_num) noexcept;

    // Splitting params.
    float split_log_weight{ 0.95f };
    float split_bias      { 0.f   };

    // Normal GPU vertex culling.
    bool enable_face_culling = true;
    Face faces_to_cull       = Face::Back;

    // This does not blend the cascades by themselves, only adjusts the output
    // of this stage for blending to be correctly supported by the shading stage.
    bool  support_cascade_blending = true;
    // Size of the blending region in texel space of each inner cascade.
    float blend_size_inner_tx      = 50.f;


    struct CascadeView {
        OrthographicCamera::Params params;        // Orthographic frustum width, height, z near/far.
        glm::vec2                  tx_scale;      // Shadowmap texel scale in shadow space.
        ViewFrustumAsPlanes        frustum_world;
        ViewFrustumAsPlanes        frustum_padded_world; // Padded frustum for blending. Used for culling if blending is enabled.
        glm::mat4                  view_mat;
        glm::mat4                  proj_mat;

        // TODO: Probably have a separate place for this.
        // NOTE: It's convinient to keep this accessible as output for others to look at.
        struct CSMDrawLists {
            std::vector<entt::entity> visible_at;
            std::vector<entt::entity> visible_noat;
        };
        CSMDrawLists draw_lists;
    };


    // TODO: Used mainly in deferred shading and debug.
    struct CascadeViewGPU {
        alignas(std430::align_vec4) glm::mat4 projview{};
        alignas(std430::align_vec3) glm::vec3 scale   {};

        // Convinience. Static c-tor to keep this as aggregate type.
        static auto create_from(const CascadeView& cascade) noexcept
            -> CascadeViewGPU
        {
            const auto [w, h, zn, zf] = cascade.params;
            return {
                .projview = cascade.proj_mat * cascade.view_mat,
                .scale    = { w, h, zf - zn },
            };
        }
    };


    /*
    Primary output type. Collection of useful info about the cascades.
    */
    struct Cascades {
        using Target = RenderTarget<ShareableAttachment<Renderable::Texture2DArray>>;
        Target                   maps;
        Size2I                   resolution;
        std::vector<CascadeView> views;

        // If false, draw lists were not used. They contain garbage.
        bool  draw_lists_active{ false };
        // If true, blending is supported. This does not blend anything by itself,
        // only adjusts the culling to cull correctly in the blend region.
        bool  blend_possible   { false };
        // Size of the blended region. Technically, is a *max* blend size.
        // In texels of the inner cascade when blending any inner-outer pair.
        // TODO: World-space representation of this might be more useful.
        float blend_max_size_inner_tx{ 0.f };
    };


    auto share_output_view() const noexcept -> SharedView<Cascades> { return cascades_.share_view(); }
    auto view_output()       const noexcept -> const Cascades&      { return *cascades_;             }

    CascadedShadowMapping();
    CascadedShadowMapping(size_t num_desired_cascades, const Size2I& resolution);

    void operator()(RenderEnginePrimaryInterface& engine);


private:
    auto allowed_num_cascades(size_t desired_num) const noexcept -> size_t;
    size_t num_cascades_;

    SharedStorage<Cascades> cascades_;


    void draw_all_cascades_with_geometry_shader(RenderEnginePrimaryInterface& engine);

    UniqueProgram sp_singlepass_gs_with_alpha_{
        ShaderBuilder()
            .load_vert(VPath("src/shaders/depth_map_cascade.vert"))
            .load_geom(VPath("src/shaders/depth_map_cascade.geom"))
            .load_frag(VPath("src/shaders/depth_map_cascade.frag"))
            .define("MAX_VERTICES", std::to_string(3ull * max_cascades()))
            .define("ENABLE_ALPHA_TESTING")
            .get()
    };

    UniqueProgram sp_singlepass_gs_no_alpha_{
        ShaderBuilder()
            .load_vert(VPath("src/shaders/depth_map_cascade.vert"))
            .load_geom(VPath("src/shaders/depth_map_cascade.geom"))
            .load_frag(VPath("src/shaders/depth_map_cascade.frag"))
            .define("MAX_VERTICES", std::to_string(3ull * max_cascades()))
            .get()
    };


    void draw_with_culling_per_cascade(RenderEnginePrimaryInterface& engine);

    UniqueProgram sp_per_cascade_with_alpha_{
        ShaderBuilder()
            .load_vert(VPath("src/shaders/depth_map.vert"))
            .load_frag(VPath("src/shaders/depth_map.frag"))
            .define("ENABLE_ALPHA_TESTING")
            .get()
    };

    UniqueProgram sp_per_cascade_no_alpha_{
        ShaderBuilder()
            .load_vert(VPath("src/shaders/depth_map.vert"))
            .load_frag(VPath("src/shaders/depth_map.frag"))
            .get()
    };

    using CascadeLayerTarget = RenderTarget<SharedLayerAttachment<Renderable::Texture2DArray>>;
    CascadeLayerTarget cascade_layer_target_{ cascades_->maps.share_depth_attachment_layer(Layer{ 0 }) };

};




} // namespace josh::stages::primary
