#pragma once
#include "AssimpModelLoader.hpp"
#include "Attachments.hpp"
#include "FrustumCuller.hpp"
#include "GLTextures.hpp"
#include "PerspectiveCamera.hpp"
#include "CubemapData.hpp"
#include "GlobalsUtil.hpp"
#include "ImGuiContextWrapper.hpp"
#include "ImGuiRegistryHooks.hpp"
#include "ImGuiStageHooks.hpp"
#include "ImGuiVFSControl.hpp"
#include "ImGuiWindowSettings.hpp"
#include "Input.hpp"
#include "InputFreeCamera.hpp"
#include "LightCasters.hpp"
#include "Model.hpp"
#include "RenderComponents.hpp"
#include "RenderEngine.hpp"
#include "Shared.hpp"
#include "Transform.hpp"
#include "VPath.hpp"
#include "VirtualFilesystem.hpp"
#include "hooks/BoundingSphereDebugStageHook.hpp"
#include "hooks/CascadedShadowMappingStageHook.hpp"
#include "hooks/DeferredShadingStageHook.hpp"
#include "hooks/GBufferStageHook.hpp"
#include "hooks/LightComponentsRegistryHook.hpp"
#include "hooks/ModelComponentsRegistryHook.hpp"
#include "hooks/PerspectiveCameraHook.hpp"
#include "hooks/PointLightSourceBoxStageHook.hpp"
#include "hooks/PointShadowMappingStageHook.hpp"
#include "hooks/PostprocessBloomStageHook.hpp"
#include "hooks/PostprocessGBufferDebugOverlayStageHook.hpp"
#include "hooks/PostprocessGammaCorrectionStageHook.hpp"
#include "hooks/PostprocessHDREyeAdaptationStageHook.hpp"
#include "hooks/PostprocessFogStageHook.hpp"
#include "hooks/SkyboxRegistryHook.hpp"
#include "hooks/SkyboxStageHook.hpp"
#include "stages/BoundingSphereDebugStage.hpp"
#include "stages/CascadedShadowMappingStage.hpp"
#include "stages/DeferredGeometryStage.hpp"
#include "stages/DeferredShadingStage.hpp"
#include "stages/GBufferStage.hpp"
#include "stages/PointLightSourceBoxStage.hpp"
#include "stages/PointShadowMappingStage.hpp"
#include "stages/PostprocessBloomStage.hpp"
#include "stages/PostprocessGBufferDebugOverlayStage.hpp"
#include "stages/PostprocessGammaCorrectionStage.hpp"
#include "stages/PostprocessHDREyeAdaptationStage.hpp"
#include "stages/PostprocessFogStage.hpp"
#include "stages/SkyboxStage.hpp"
#include <entt/entity/fwd.hpp>
#include <entt/entt.hpp>
#include <glbinding/gl/enum.h>
#include <glfwpp/window.h>
#include <imgui.h>


namespace leaksjosh {

using namespace josh;


class DemoScene {
private:
    glfw::Window& window_;

    entt::registry registry_;

    PerspectiveCamera cam_{
        PerspectiveCameraParams{
            .fovy_rad = glm::radians(90.f),
            .aspect_ratio = globals::window_size.size_ref().aspect_ratio(),
            .z_near  = 0.1f,
            .z_far   = 100.f
        },
        Transform().translate({ 0.f, 1.f, 0.f })
    };

    SimpleInputBlocker   input_blocker_;
    BasicRebindableInput input_{ window_, input_blocker_ };
    InputFreeCamera      input_freecam_{ cam_ };

    RenderEngine rengine_{
        registry_, cam_,
        globals::window_size.size_ref(), globals::frame_timer
    };

    CascadeViewsBuilder csm_info_builder_{ 5 };
    FrustumCuller culler_{ registry_ };

    ImGuiContextWrapper imgui_{ window_ };
    ImGuiWindowSettings imgui_window_settings_{ window_ };
    ImGuiVFSControl     imgui_vfs_control_{ vfs() };
    ImGuiStageHooks     imgui_stage_hooks_;
    ImGuiRegistryHooks  imgui_registry_hooks_{ registry_ };

public:
    DemoScene(glfw::Window& window)
        : window_{ window }
    {
        configure_input();

        auto psmapping    = rengine_.make_primary_stage<PointShadowMappingStage>();
        auto csmapping    = rengine_.make_primary_stage<CascadedShadowMappingStage>(csm_info_builder_.view_output());
        auto gbuffer      = rengine_.make_primary_stage<GBufferStage>(
            rengine_.window_size(),
            // This is me sharing the depth target between the GBuffer and the
            // main framebuffer of the RenderEngine, so that deferred and forward draws
            // would overlap properly. Seems to work so far...
            ViewAttachment<RawTexture2D>{ rengine_.main_depth() }
        );

        auto defgeom     = rengine_.make_primary_stage<DeferredGeometryStage>(gbuffer.target().get_write_view());

        auto defshad     = rengine_.make_primary_stage<DeferredShadingStage>(
            gbuffer.target().get_read_view(),
            psmapping.target().view_output(),
            csmapping.target().view_output()
        );

        auto plightboxes = rengine_.make_primary_stage<PointLightSourceBoxStage>();
        auto cullspheres = rengine_.make_primary_stage<BoundingSphereDebugStage>();

        auto skyboxing   = rengine_.make_primary_stage<SkyboxStage>();


        auto fog         = rengine_.make_postprocess_stage<PostprocessFogStage>();
        auto blooming    = rengine_.make_postprocess_stage<PostprocessBloomStage>();
        auto hdreyeing   = rengine_.make_postprocess_stage<PostprocessHDREyeAdaptationStage>();
        auto whatsgamma  = rengine_.make_postprocess_stage<PostprocessGammaCorrectionStage>();

        auto gbugger     = rengine_.make_postprocess_stage<PostprocessGBufferDebugOverlayStage>(gbuffer.target().get_read_view());


        imgui_stage_hooks_.add_hook("Point Shadow Mapping",
            imguihooks::PointShadowMappingStageHook(psmapping));

        imgui_stage_hooks_.add_hook("Cascaded Shadow Mapping",
            imguihooks::CascadedShadowMappingStageHook(csm_info_builder_, csmapping));

        imgui_stage_hooks_.add_hook("GBuffer",
            imguihooks::GBufferStageHook(gbuffer));

        imgui_stage_hooks_.add_hook("Deferred Shading",
            imguihooks::DeferredShadingStageHook(defshad));

        imgui_stage_hooks_.add_hook("Point Light Boxes",
            imguihooks::PointLightSourceBoxStageHook(plightboxes));

        imgui_stage_hooks_.add_hook("Bounding Spheres",
            imguihooks::BoundingSphereDebugStageHook(cullspheres));

        imgui_stage_hooks_.add_hook("Sky",
            imguihooks::SkyboxStageHook(skyboxing));

        imgui_stage_hooks_.add_postprocess_hook("Fog",
            imguihooks::PostprocessFogStageHook(fog));

        imgui_stage_hooks_.add_postprocess_hook("Bloom",
            imguihooks::PostprocessBloomStageHook(blooming));

        imgui_stage_hooks_.add_postprocess_hook("HDR Eye Adaptation",
            imguihooks::PostprocessHDREyeAdaptationStageHook(hdreyeing));

        imgui_stage_hooks_.add_postprocess_hook("GBuffer Debug Overlay",
            imguihooks::PostprocessGBufferDebugOverlayStageHook(gbugger));

        imgui_stage_hooks_.add_postprocess_hook("Gamma Correction",
            imguihooks::PostprocessGammaCorrectionStageHook(whatsgamma));



        rengine_.add_next_primary_stage(std::move(psmapping));
        rengine_.add_next_primary_stage(std::move(csmapping));
        rengine_.add_next_primary_stage(std::move(gbuffer));
        rengine_.add_next_primary_stage(std::move(defgeom));
        rengine_.add_next_primary_stage(std::move(defshad));
        rengine_.add_next_primary_stage(std::move(plightboxes));
        rengine_.add_next_primary_stage(std::move(cullspheres));
        rengine_.add_next_primary_stage(std::move(skyboxing));

        rengine_.add_next_postprocess_stage(std::move(fog));
        rengine_.add_next_postprocess_stage(std::move(blooming));
        rengine_.add_next_postprocess_stage(std::move(hdreyeing));
        // FIXME: It should be put *after* all of the effects to
        // act as an overlay. But because my sRGB gamma correction
        // is broken but works correctly accidentally because
        // it's the last stage, I can't just move the overlay after it until I fix that.
        rengine_.add_next_postprocess_stage(std::move(gbugger));
        rengine_.add_next_postprocess_stage(std::move(whatsgamma));


        imgui_registry_hooks_.add_hook("Lights", imguihooks::LightComponentsRegistryHook());
        imgui_registry_hooks_.add_hook("Models", imguihooks::ModelComponentsRegistryHook());
        imgui_registry_hooks_.add_hook("Camera", imguihooks::PerspectiveCameraHook(cam_));
        imgui_registry_hooks_.add_hook("Skybox", imguihooks::SkyboxRegistryHook());

        init_registry();
    }

    void process_input() {}

    void update() {
        input_freecam_.update();

        csm_info_builder_.build_from_camera(rengine_.camera(),
            // FIXME: Scuffed, but who finds the light?
            registry_.view<light::Directional>().storage().begin()->direction);

        // TODO: Cull for shadow mapping
        // culler_.cull_from_bounding_spheres<tags::CulledFromCascadedShadowMapping>(csm_info_builder_.view_output()->cascades.back().frustum);
        culler_.cull_from_bounding_spheres<tags::Culled>(cam_.get_frustum_as_planes());
    }

    void render() {
        imgui_.new_frame();

        rengine_.render();

        imgui_window_settings_.display();
        imgui_vfs_control_.display();
        imgui_registry_hooks_.display();
        imgui_stage_hooks_.display();

        imgui_.render();

        update_input_blocker_from_imgui_io_state();
    }

private:
    void configure_input();
    void init_registry();
    void update_input_blocker_from_imgui_io_state();

};





inline void DemoScene::configure_input() {

    input_freecam_.configure(input_);

    input_.set_keybind(glfw::KeyCode::T, [this](const KeyCallbackArgs& args) {
        if (args.is_released()) {
            imgui_window_settings_.hidden ^= true;
            imgui_vfs_control_.hidden ^= true;
            imgui_stage_hooks_.hidden ^= true;
            imgui_registry_hooks_.hidden ^= true;
        }
    });

    window_.framebufferSizeEvent.setCallback(
        [this](glfw::Window& /* window */ , int w, int h) {
            using namespace gl;

            globals::window_size.set_to({ w, h });
            glViewport(0, 0, w, h);
            rengine_.reset_size({ w, h });
            // or
            // rengine_.reset_size_from_window_size();
        }
    );

}




inline void DemoScene::init_registry() {
    auto& r = registry_;

    using enum GLenum;

    constexpr const char* path = "data/models/shadow_scene/shadow_scene.obj";
    constexpr const char* name = "shadow_scene.obj";

    entt::handle model{ r, r.create() };

    ModelComponentLoader()
        .load_into(model, VPath(path));

    model.emplace<Transform>();
    model.emplace<components::Path>(path);
    model.emplace<components::Name>(name);

    r.emplace<light::Ambient>(r.create(), light::Ambient{
        .color = { 0.15f, 0.15f, 0.1f }
    });

    auto e = r.create();
    r.emplace<light::Directional>(e, light::Directional{
        .color = { 0.15f, 0.15f, 0.1f },
        .direction = { -0.2f, -1.0f, -0.3f }
    });
    r.emplace<tags::ShadowCasting>(e);

    components::Skybox skybox{ std::make_shared<UniqueCubemap>() };
    skybox.cubemap->bind()
        .and_then([&](BoundCubemap<GLMutable>& cubemap) {
            attach_data_to_cubemap(
                cubemap,
                CubemapData::from_files(
                    VPath("data/skyboxes/lake/right.png"),
                    VPath("data/skyboxes/lake/left.png"),
                    VPath("data/skyboxes/lake/top.png"),
                    VPath("data/skyboxes/lake/bottom.png"),
                    VPath("data/skyboxes/lake/front.png"),
                    VPath("data/skyboxes/lake/back.png")
                ),
                gl::GL_SRGB_ALPHA
            );
        })
        // FIXME: There's gotta be a better place to put this.
        // Maybe a create_skybox() function or something...
        .set_parameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR)
        .set_parameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR)
        .set_parameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE)
        .set_parameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE)
        .set_parameter(GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    r.emplace<components::Skybox>(r.create(), std::move(skybox));

}




inline void DemoScene::update_input_blocker_from_imgui_io_state() {
    // FIXME: Need a way to stop the ImGui window from recieving
    // mouse events when I'm in free cam.
    input_blocker_.block_keys = ImGui::GetIO().WantCaptureKeyboard;
    input_blocker_.block_scroll =
        ImGui::GetIO().WantCaptureMouse &&
        input_freecam_.state().is_cursor_mode;
}




} // namespace leaksjosh

using leaksjosh::DemoScene;
