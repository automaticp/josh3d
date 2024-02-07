#pragma once
#include "AssimpModelLoader.hpp"
#include "Attachments.hpp"
#include "ComponentLoaders.hpp"
#include "EnumUtils.hpp"
#include "FrustumCuller.hpp"
#include "GLTextures.hpp"
#include "ImGuiApplicationAssembly.hpp"
#include "ImGuizmoGizmos.hpp"
#include "PerspectiveCamera.hpp"
#include "ImGuiRegistryHooks.hpp"
#include "ImGuiStageHooks.hpp"
#include "Input.hpp"
#include "InputFreeCamera.hpp"
#include "LightCasters.hpp"
#include "SharedStorage.hpp"
#include "WindowSizeCache.hpp"
#include "FrameTimer.hpp"
#include "RenderEngine.hpp"
#include "Transform.hpp"
#include "VPath.hpp"
#include "VirtualFilesystem.hpp"

#include "components/ChildMesh.hpp"
#include "tags/Selected.hpp"
#include "tags/ShadowCasting.hpp"
#include "components/Path.hpp"
#include "components/Name.hpp"

#include "stages/primary/CascadedShadowMapping.hpp"
#include "stages/primary/PointShadowMapping.hpp"
#include "stages/primary/GBufferStorage.hpp"
#include "stages/primary/DeferredGeometry.hpp"
#include "stages/primary/TerrainGeometry.hpp"
#include "stages/primary/DeferredShading.hpp"
#include "stages/primary/PointLightBox.hpp"
#include "stages/primary/Sky.hpp"
#include "stages/postprocess/Bloom.hpp"
#include "stages/postprocess/FXAA.hpp"
#include "stages/postprocess/HDREyeAdaptation.hpp"
#include "stages/postprocess/Fog.hpp"
#include "stages/overlay/GBufferDebug.hpp"
#include "stages/overlay/SelectedObjectHighlight.hpp"
#include "stages/overlay/BoundingSphereDebug.hpp"

#include "hooks/primary/CascadedShadowMapping.hpp"
#include "hooks/primary/PointShadowMapping.hpp"
#include "hooks/primary/GBufferStorage.hpp"
#include "hooks/primary/DeferredShading.hpp"
#include "hooks/primary/PointLightBox.hpp"
#include "hooks/primary/Sky.hpp"
#include "hooks/postprocess/Bloom.hpp"
#include "hooks/postprocess/FXAA.hpp"
#include "hooks/postprocess/HDREyeAdaptation.hpp"
#include "hooks/postprocess/Fog.hpp"
#include "hooks/overlay/GBufferDebug.hpp"
#include "hooks/overlay/SelectedObjectHighlight.hpp"
#include "hooks/overlay/BoundingSphereDebug.hpp"
#include "hooks/registry/LightComponents.hpp"
#include "hooks/registry/ModelComponents.hpp"
#include "hooks/registry/TerrainComponents.hpp"
#include "hooks/registry/SkyboxComponents.hpp"
#include "hooks/registry/PerspectiveCamera.hpp"

#include <entt/entity/fwd.hpp>
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

    ImGuiApplicationAssembly imgui_{ window_, registry_, cam_, vfs() };

public:
    DemoScene(glfw::Window& window);
    void process_input() {}
    void update();
    void render();

private:
    void configure_input(SharedStorageView<GBuffer> gbuffer);
    void init_registry();
    void update_input_blocker_from_imgui_io_state();

};




inline DemoScene::DemoScene(glfw::Window& window)
    : window_{ window }
{

    auto psmapping =
        rengine_.make_primary_stage<stages::primary::PointShadowMapping>();

    auto csmapping =
        rengine_.make_primary_stage<stages::primary::CascadedShadowMapping>(csm_info_builder_.view_output());

    auto gbuffer =
        rengine_.make_primary_stage<stages::primary::GBufferStorage>(
            rengine_.window_size(),
            // This is me sharing the depth target between the GBuffer and the
            // main framebuffer of the RenderEngine, so that deferred and forward draws
            // would overlap properly. Seems to work so far...
            ViewAttachment<RawTexture2D>{ rengine_.main_depth() }
        );

    auto gbuffer_read_view = gbuffer.target().get_read_view();

    auto defgeom =
        rengine_.make_primary_stage<stages::primary::DeferredGeometry>(gbuffer.target().get_write_view());

    auto terraingeom =
        rengine_.make_primary_stage<stages::primary::TerrainGeometry>(gbuffer.target().get_write_view());

    auto defshad =
        rengine_.make_primary_stage<stages::primary::DeferredShading>(
            gbuffer_read_view,
            psmapping.target().view_output(),
            csmapping.target().view_output()
        );

    auto plightboxes =
        rengine_.make_primary_stage<stages::primary::PointLightBox>();

    auto sky =
        rengine_.make_primary_stage<stages::primary::Sky>();


    auto fog =
        rengine_.make_postprocess_stage<stages::postprocess::Fog>();

    auto blooming =
        rengine_.make_postprocess_stage<stages::postprocess::Bloom>();

    auto hdreyeing =
        rengine_.make_postprocess_stage<stages::postprocess::HDREyeAdaptation>();

    auto fxaaaaaaa =
        rengine_.make_postprocess_stage<stages::postprocess::FXAA>();

    // auto whatsgamma =
    //     rengine_.make_postprocess_stage<stages::postprocess::GammaCorrection>();


    auto gbugger =
        rengine_.make_overlay_stage<stages::overlay::GBufferDebug>(gbuffer.target().get_read_view());

    auto selected =
        rengine_.make_overlay_stage<stages::overlay::SelectedObjectHighlight>();

    auto cullspheres =
        rengine_.make_overlay_stage<stages::overlay::BoundingSphereDebug>();



    imgui_.stage_hooks().add_primary_hook("Point Shadow Mapping",
        imguihooks::primary::PointShadowMapping(psmapping));

    imgui_.stage_hooks().add_primary_hook("Cascaded Shadow Mapping",
        imguihooks::primary::CascadedShadowMapping(csm_info_builder_, csmapping));

    imgui_.stage_hooks().add_primary_hook("GBuffer",
        imguihooks::primary::GBufferStorage(gbuffer));

    imgui_.stage_hooks().add_primary_hook("Deferred Shading",
        imguihooks::primary::DeferredShading(defshad));

    imgui_.stage_hooks().add_primary_hook("Point Light Boxes",
        imguihooks::primary::PointLightBox(plightboxes));


    imgui_.stage_hooks().add_primary_hook("Sky",
        imguihooks::primary::Sky(sky));

    imgui_.stage_hooks().add_postprocess_hook("Fog",
        imguihooks::postprocess::Fog(fog));

    imgui_.stage_hooks().add_postprocess_hook("Bloom",
        imguihooks::postprocess::Bloom(blooming));

    imgui_.stage_hooks().add_postprocess_hook("HDR Eye Adaptation",
        imguihooks::postprocess::HDREyeAdaptation(hdreyeing));

    imgui_.stage_hooks().add_postprocess_hook("FXAA",
        imguihooks::postprocess::FXAA(fxaaaaaaa));


    imgui_.stage_hooks().add_overlay_hook("GBuffer Debug Overlay",
        imguihooks::overlay::GBufferDebug(gbugger));

    imgui_.stage_hooks().add_overlay_hook("Selected Object Highlight",
        imguihooks::overlay::SelectedObjectHighlight(selected));

    imgui_.stage_hooks().add_overlay_hook("Bounding Spheres",
        imguihooks::overlay::BoundingSphereDebug(cullspheres));


    rengine_.add_next_primary_stage(std::move(psmapping));
    rengine_.add_next_primary_stage(std::move(csmapping));
    rengine_.add_next_primary_stage(std::move(gbuffer));
    rengine_.add_next_primary_stage(std::move(defgeom));
    rengine_.add_next_primary_stage(std::move(terraingeom));
    rengine_.add_next_primary_stage(std::move(defshad));
    rengine_.add_next_primary_stage(std::move(plightboxes));
    rengine_.add_next_primary_stage(std::move(sky));

    rengine_.add_next_postprocess_stage(std::move(fog));
    rengine_.add_next_postprocess_stage(std::move(blooming));
    rengine_.add_next_postprocess_stage(std::move(hdreyeing));
    rengine_.add_next_postprocess_stage(std::move(fxaaaaaaa));

    rengine_.add_next_overlay_stage(std::move(gbugger));
    rengine_.add_next_overlay_stage(std::move(selected));
    rengine_.add_next_overlay_stage(std::move(cullspheres));


    imgui_.registry_hooks().add_hook("Lights",  imguihooks::registry::LightComponents());
    imgui_.registry_hooks().add_hook("Models",  imguihooks::registry::ModelComponents());
    imgui_.registry_hooks().add_hook("Skybox",  imguihooks::registry::SkyboxComponents());
    imgui_.registry_hooks().add_hook("Terrain", imguihooks::registry::TerrainComponents());
    imgui_.registry_hooks().add_hook("Camera",  imguihooks::registry::PerspectiveCamera(cam_));


    configure_input(gbuffer_read_view);

    init_registry();
}




inline void DemoScene::update() {
    update_input_blocker_from_imgui_io_state();

    input_freecam_.update();

    csm_info_builder_.build_from_camera(rengine_.camera(),
        // FIXME: Scuffed, but who finds the light?
        registry_.view<light::Directional>().storage().begin()->direction);

    // TODO: Cull for shadow mapping
    // culler_.cull_from_bounding_spheres<tags::CulledFromCascadedShadowMapping>(csm_info_builder_.view_output()->cascades.back().frustum);
    culler_.cull_from_bounding_spheres<tags::Culled>(cam_.get_frustum_as_planes());
}




inline void DemoScene::render() {
    imgui_.new_frame();
    rengine_.render();
    imgui_.display();
}




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


                entt::handle provoking_handle{ registry_, entt::entity{ id } };

                entt::handle target_handle = [&]() -> entt::handle {
                    if (args.mods & glfw::ModifierKeyBit::Control) {
                        // Select mesh (same id as returned)
                        return provoking_handle;
                    } else {
                        // Select the whole model if it's a ChildMesh
                        if (auto as_child = provoking_handle.try_get<components::ChildMesh>()) {
                            return { registry_, as_child->parent };
                        }
                        // Fallback for everything else.
                        return provoking_handle;
                    }
                }();

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

    input_.bind_mouse_button(glfw::MouseButton::Middle,
        [&, this](const MouseButtonCallbackArgs& args) {
            if (args.is_pressed()) {
                switch (imgui_.active_gizmo_operation()) {
                    using enum GizmoOperation;
                    case translation: imgui_.active_gizmo_operation() = rotation;    break;
                    case rotation:    imgui_.active_gizmo_operation() = scaling;     break;
                    case scaling:     imgui_.active_gizmo_operation() = translation; break;
                }
            }
        }
    );

    input_.bind_mouse_button(glfw::MouseButton::Right,
        [&, this](const MouseButtonCallbackArgs& args) {
            if (args.is_pressed()) {
                switch (imgui_.active_gizmo_space()) {
                    using enum GizmoSpace;
                    case world: imgui_.active_gizmo_space() = local; break;
                    case local: imgui_.active_gizmo_space() = world; break;
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
