#pragma once
#include "AssimpModelLoader.hpp"
#include "Attachments.hpp"
#include "ComponentLoaders.hpp"
#include "EnumUtils.hpp"
#include "FrustumCuller.hpp"
#include "GLTextures.hpp"
#include "ImGuiApplicationAssembly.hpp"
#include "PerspectiveCamera.hpp"
#include "ImGuiRegistryHooks.hpp"
#include "ImGuiStageHooks.hpp"
#include "Input.hpp"
#include "InputFreeCamera.hpp"
#include "LightCasters.hpp"
#include "SharedStorage.hpp"
#include "WindowSizeCache.hpp"
#include "FrameTimer.hpp"
#include "components/Path.hpp"
#include "components/Name.hpp"
#include "tags/Selected.hpp"
#include "hooks/TerrainComponentRegistryHook.hpp"
#include "stages/TerrainGeometryStage.hpp"
#include "tags/ShadowCasting.hpp"
#include "RenderEngine.hpp"
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
#include "hooks/OverlayGBufferDebugStageHook.hpp"
#include "hooks/PostprocessFXAAStageHook.hpp"
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
#include "stages/OverlayGBufferDebugStage.hpp"
#include "stages/PostprocessFXAAStage.hpp"
#include "stages/PostprocessHDREyeAdaptationStage.hpp"
#include "stages/PostprocessFogStage.hpp"
#include "stages/SkyboxStage.hpp"
#include <entt/entt.hpp>
#include <glbinding/gl/enum.h>
#include <glfwpp/window.h>


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

    ImGuiApplicationAssembly imgui_{ window_, registry_, vfs() };

public:
    DemoScene(glfw::Window& window)
        : window_{ window }
    {

        auto psmapping    = rengine_.make_primary_stage<PointShadowMappingStage>();
        auto csmapping    = rengine_.make_primary_stage<CascadedShadowMappingStage>(csm_info_builder_.view_output());
        auto gbuffer      = rengine_.make_primary_stage<GBufferStage>(
            rengine_.window_size(),
            // This is me sharing the depth target between the GBuffer and the
            // main framebuffer of the RenderEngine, so that deferred and forward draws
            // would overlap properly. Seems to work so far...
            ViewAttachment<RawTexture2D>{ rengine_.main_depth() }
        );

        auto gbuffer_read_view = gbuffer.target().get_read_view();

        auto defgeom     = rengine_.make_primary_stage<DeferredGeometryStage>(gbuffer.target().get_write_view());
        auto terraingeom = rengine_.make_primary_stage<TerrainGeometryStage>(gbuffer.target().get_write_view());

        auto defshad     = rengine_.make_primary_stage<DeferredShadingStage>(
            gbuffer_read_view,
            psmapping.target().view_output(),
            csmapping.target().view_output()
        );

        auto plightboxes = rengine_.make_primary_stage<PointLightSourceBoxStage>();
        auto cullspheres = rengine_.make_primary_stage<BoundingSphereDebugStage>();

        auto skyboxing   = rengine_.make_primary_stage<SkyboxStage>();


        auto fog         = rengine_.make_postprocess_stage<PostprocessFogStage>();
        auto blooming    = rengine_.make_postprocess_stage<PostprocessBloomStage>();
        auto hdreyeing   = rengine_.make_postprocess_stage<PostprocessHDREyeAdaptationStage>();
        auto fxaaaaaaa   = rengine_.make_postprocess_stage<PostprocessFXAAStage>();
        // auto whatsgamma  = rengine_.make_postprocess_stage<PostprocessGammaCorrectionStage>();

        auto gbugger     = rengine_.make_overlay_stage<OverlayGBufferDebugStage>(gbuffer.target().get_read_view());


        imgui_.stage_hooks().add_primary_hook("Point Shadow Mapping",
            imguihooks::PointShadowMappingStageHook(psmapping));

        imgui_.stage_hooks().add_primary_hook("Cascaded Shadow Mapping",
            imguihooks::CascadedShadowMappingStageHook(csm_info_builder_, csmapping));

        imgui_.stage_hooks().add_primary_hook("GBuffer",
            imguihooks::GBufferStageHook(gbuffer));

        imgui_.stage_hooks().add_primary_hook("Deferred Shading",
            imguihooks::DeferredShadingStageHook(defshad));

        imgui_.stage_hooks().add_primary_hook("Point Light Boxes",
            imguihooks::PointLightSourceBoxStageHook(plightboxes));

        imgui_.stage_hooks().add_primary_hook("Bounding Spheres",
            imguihooks::BoundingSphereDebugStageHook(cullspheres));

        imgui_.stage_hooks().add_primary_hook("Sky",
            imguihooks::SkyboxStageHook(skyboxing));

        imgui_.stage_hooks().add_postprocess_hook("Fog",
            imguihooks::PostprocessFogStageHook(fog));

        imgui_.stage_hooks().add_postprocess_hook("Bloom",
            imguihooks::PostprocessBloomStageHook(blooming));

        imgui_.stage_hooks().add_postprocess_hook("HDR Eye Adaptation",
            imguihooks::PostprocessHDREyeAdaptationStageHook(hdreyeing));

        imgui_.stage_hooks().add_postprocess_hook("FXAA",
            imguihooks::PostprocessFXAAStageHook(fxaaaaaaa));

        imgui_.stage_hooks().add_overlay_hook("GBuffer Debug Overlay",
            imguihooks::OverlayGBufferDebugStageHook(gbugger));



        rengine_.add_next_primary_stage(std::move(psmapping));
        rengine_.add_next_primary_stage(std::move(csmapping));
        rengine_.add_next_primary_stage(std::move(gbuffer));
        rengine_.add_next_primary_stage(std::move(defgeom));
        rengine_.add_next_primary_stage(std::move(terraingeom));
        rengine_.add_next_primary_stage(std::move(defshad));
        rengine_.add_next_primary_stage(std::move(plightboxes));
        rengine_.add_next_primary_stage(std::move(cullspheres));
        rengine_.add_next_primary_stage(std::move(skyboxing));

        rengine_.add_next_postprocess_stage(std::move(fog));
        rengine_.add_next_postprocess_stage(std::move(blooming));
        rengine_.add_next_postprocess_stage(std::move(hdreyeing));
        rengine_.add_next_postprocess_stage(std::move(fxaaaaaaa));

        rengine_.add_next_overlay_stage(std::move(gbugger));


        imgui_.registry_hooks().add_hook("Lights",  imguihooks::LightComponentsRegistryHook());
        imgui_.registry_hooks().add_hook("Models",  imguihooks::ModelComponentsRegistryHook());
        imgui_.registry_hooks().add_hook("Camera",  imguihooks::PerspectiveCameraHook(cam_));
        imgui_.registry_hooks().add_hook("Skybox",  imguihooks::SkyboxRegistryHook());
        imgui_.registry_hooks().add_hook("Terrain", imguihooks::TerrainComponentRegistryHook());

        configure_input(gbuffer_read_view);

        init_registry();
    }

    void process_input() {}

    void update() {
        update_input_blocker_from_imgui_io_state();

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

        imgui_.display();

    }

private:
    void configure_input(SharedStorageView<GBuffer> gbuffer);
    void init_registry();
    void update_input_blocker_from_imgui_io_state();

};





inline void DemoScene::configure_input(SharedStorageView<GBuffer> gbuffer) {

    input_freecam_.configure(input_);

    input_.bind_key(glfw::KeyCode::T, [this](const KeyCallbackArgs& args) {
        if (args.is_released()) {
            imgui_.toggle_hidden();
        }
    });

    input_.bind_mouse_button(glfw::MouseButton::Left,
        [gbuffer=std::move(gbuffer), this](const MouseButtonCallbackArgs& args) {
            if (args.is_pressed()) {
                auto [x, y] = args.window.getCursorPos();
                auto [sz_x, sz_y] = args.window.getFramebufferSize();
                int ix = int(x);
                int iy = int(y);

                using namespace gl;

                entt::id_type id{ entt::null };

                auto attachment_id =
                    GL_COLOR_ATTACHMENT0 + to_underlying(GBuffer::Slot::object_id);

                gbuffer->bind_read().set_read_buffer(attachment_id).and_then([&] {
                    // FIXME: Is this off-by-one?
                    glReadPixels(ix, sz_y - iy, 1, 1, GL_RED_INTEGER, GL_UNSIGNED_INT, &id);
                }).unbind();


                entt::handle target_handle{ registry_, entt::entity{ id } };
                const bool had_selection =
                    target_handle.valid() && target_handle.any_of<tags::Selected>();

                // Deselect all selected.
                registry_.clear<tags::Selected>();

                // Uhh...
                if (!had_selection && target_handle.valid()) {
                    target_handle.emplace<tags::Selected>();
                }

            }
        }
    );

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

    load_skybox_into(entt::handle{ r, r.create() }, VPath("data/skyboxes/yokohama/skybox.json"));
}




inline void DemoScene::update_input_blocker_from_imgui_io_state() {

    auto wants = imgui_.get_io_wants();
    input_blocker_.block_keys = wants.capture_keyboard;
    // FIXME: Need a way to stop the ImGui window from recieving
    // mouse events when I'm in free cam.
    input_blocker_.block_mouse_buttons = wants.capture_mouse;
    input_blocker_.block_scroll =
        wants.capture_mouse &&
        input_freecam_.state().is_cursor_mode;
}




} // namespace leaksjosh

using leaksjosh::DemoScene;
