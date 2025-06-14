#include "DemoScene.hpp"
#include "Active.hpp"
#include "AssetManager.hpp"
#include "AssetUnpacker.hpp"
#include "AsyncCradle.hpp"
#include "CategoryCasts.hpp"
#include "ContainerUtils.hpp"
#include "DefaultResources.hpp"
#include "GLPixelPackTraits.hpp"
#include "GLTextures.hpp"
#include "ImGuiDefaultResourceInspectors.hpp"
#include "ImGuiApplicationAssembly.hpp"
#include "Logging.hpp"
#include "Camera.hpp"
#include "Input.hpp"
#include "InputFreeCamera.hpp"
#include "LightCasters.hpp"
#include "ObjectLifecycle.hpp"
#include "OffscreenContext.hpp"
#include "ResourceDatabase.hpp"
#include "Runtime.hpp"
#include "Scalars.hpp"
#include "SceneGraph.hpp"
#include "ShaderPool.hpp"
#include "WindowSizeCache.hpp"
#include "FrameTimer.hpp"
#include "RenderEngine.hpp"
#include "Transform.hpp"
#include "VPath.hpp"
#include "stages/overlay/SceneOverlays.hpp"
#include "stages/precompute/AnimationSystem.hpp"
#include "stages/primary/SkinnedGeometry.hpp"
#include "tags/Selected.hpp"
#include "tags/ShadowCasting.hpp"
#include "Filesystem.hpp"
#include "Tags.hpp"
#include "stages/precompute/PointLightSetup.hpp"
#include "stages/precompute/TransformResolution.hpp"
#include "stages/precompute/BoundingVolumeResolution.hpp"
#include "stages/precompute/FrustumCulling.hpp"
#include "stages/primary/CascadedShadowMapping.hpp"
#include "stages/primary/PointShadowMapping.hpp"
#include "stages/primary/SSAO.hpp"
#include "stages/primary/IDBufferStorage.hpp"
#include "stages/primary/GBufferStorage.hpp"
#include "stages/primary/DeferredGeometry.hpp"
#include "stages/primary/TerrainGeometry.hpp"
#include "stages/primary/DeferredShading.hpp"
#include "stages/primary/LightDummies.hpp"
#include "stages/primary/Sky.hpp"
#include "stages/postprocess/Bloom2.hpp"
#include "stages/postprocess/FXAA.hpp"
#include "stages/postprocess/HDREyeAdaptation.hpp"
#include "stages/postprocess/Fog.hpp"
#include "stages/overlay/CSMDebug.hpp"
#include "stages/overlay/SSAODebug.hpp"
#include "stages/overlay/GBufferDebug.hpp"
#include "hooks/precompute/AllPrecomputeHooks.hpp"
#include "hooks/primary/AllPrimaryHooks.hpp"
#include "hooks/postprocess/AllPostprocessHooks.hpp"
#include "hooks/overlay/AllOverlayHooks.hpp"
#include <entt/entity/fwd.hpp>
#include <entt/entt.hpp>
#include <entt/entity/view.hpp>
#include <glbinding/gl/enum.h>
#include <glfwpp/window.h>
#include <exception>
#include <glm/ext.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/trigonometric.hpp>


using namespace josh;

auto DemoScene::is_done() const noexcept
    -> bool
{
    return window.shouldClose();
}

void DemoScene::execute_frame()
{
    josh::globals::frame_timer.update();

    render();

    glfw::pollEvents();
    process_input();

    update();

    window.swapBuffers();
}

void DemoScene::process_input()
{
    if (const auto camera = get_active<Camera, Transform>(runtime.registry))
    {
        _input_freecam.update(
            globals::frame_timer.delta<float>(),
            camera.get<Camera>(),
            camera.get<Transform>()
        );
    }
}

void DemoScene::update()
{
    update_input_blocker_from_imgui_io_state();

    runtime.async_cradle.local_context.flush_nonblocking();

    // TODO: Can this be removed too?
    runtime.resource_database.update();
    // TODO: Remove.
    runtime.asset_manager.update();

    runtime.asset_unpacker.retire_completed_requests();
    while (runtime.asset_unpacker.can_unpack_more())
    {
        Handle unpacked_handle;
        try
        {
            runtime.asset_unpacker.unpack_one_retired(unpacked_handle);

            const Entity entity = unpacked_handle.entity();
            logstream() << "[UNPACKED ASSET]: [" << to_entity(entity) << "]";

            if (const auto* path = unpacked_handle.try_get<Path>())
            {
                logstream() << ", Path: " << *path;
            }
            else if (const auto* asset_path = unpacked_handle.try_get<AssetPath>())
            {
                logstream() << ", AssetPath: " << asset_path->entry();
                if (asset_path->subpath().size()) {
                    logstream() << "##" << asset_path->subpath();
                }
            }
            logstream() << "\n";
        }
        catch (const std::exception& e)
        {
            const Entity entity = unpacked_handle.entity();
            logstream() << "[ERROR UNPACKING ASSET]: [" << to_entity(entity) << "] " << e.what() << "\n";
        }
    }

    if (shader_pool().supports_hot_reload())
        shader_pool().hot_reload();
}

void DemoScene::render()
{
    _imgui.new_frame(globals::frame_timer);

    runtime.renderer.render(
        runtime.registry,
        runtime.mesh_registry,
        runtime.primitives,
        globals::window_size.size(),
        globals::frame_timer
    );

    _imgui.display();
}

DemoScene::DemoScene(
    glfw::Window&  window,
    josh::Runtime& runtime
)
    : window(window)
    , runtime(runtime)
    , _input(window, _input_blocker)
    , _imgui(window, runtime)
{
    auto& renderer = runtime.renderer;

    auto plightsetup   = stages::precompute::PointLightSetup();
    auto tfresolution  = stages::precompute::TransformResolution();
    auto bvresolution  = stages::precompute::BoundingVolumeResolution();
    auto frustumculler = stages::precompute::FrustumCulling();
    auto skinning      = stages::precompute::AnimationSystem();
    auto psmapping     = stages::primary::PointShadowMapping();
    auto csmapping     = stages::primary::CascadedShadowMapping();
    auto idbuffer      = stages::primary::IDBufferStorage();
    auto gbuffer       = stages::primary::GBufferStorage();
    auto defgeom       = stages::primary::DeferredGeometry();
    auto skingeom      = stages::primary::SkinnedGeometry();
    auto terraingeom   = stages::primary::TerrainGeometry();
    auto ssao          = stages::primary::SSAO();
    auto defshad       = stages::primary::DeferredShading();
    auto lightdummies  = stages::primary::LightDummies();
    auto sky           = stages::primary::Sky();
    auto fog           = stages::postprocess::Fog();
    auto bloom2        = stages::postprocess::Bloom2();
    auto hdreyeing     = stages::postprocess::HDREyeAdaptation();
    auto fxaaaaaaa     = stages::postprocess::FXAA();
    auto gbugger       = stages::overlay::GBufferDebug();
    auto csmbugger     = stages::overlay::CSMDebug();
    auto ssaobugger    = stages::overlay::SSAODebug();
    auto sceneovers    = stages::overlay::SceneOverlays();

    _imgui.stage_hooks.add_hook(imguihooks::precompute::PointLightSetup());
    _imgui.stage_hooks.add_hook(imguihooks::primary::PointShadowMapping());
    _imgui.stage_hooks.add_hook(imguihooks::primary::CascadedShadowMapping());
    _imgui.stage_hooks.add_hook(imguihooks::primary::DeferredGeometry());
    _imgui.stage_hooks.add_hook(imguihooks::primary::SSAO());
    _imgui.stage_hooks.add_hook(imguihooks::primary::DeferredShading());
    _imgui.stage_hooks.add_hook(imguihooks::primary::LightDummies());
    _imgui.stage_hooks.add_hook(imguihooks::primary::Sky());
    _imgui.stage_hooks.add_hook(imguihooks::postprocess::Fog());
    _imgui.stage_hooks.add_hook(imguihooks::postprocess::Bloom2());
    _imgui.stage_hooks.add_hook(imguihooks::postprocess::HDREyeAdaptation());
    _imgui.stage_hooks.add_hook(imguihooks::postprocess::FXAA());
    _imgui.stage_hooks.add_hook(imguihooks::overlay::GBufferDebug());
    _imgui.stage_hooks.add_hook(imguihooks::overlay::CSMDebug());
    _imgui.stage_hooks.add_hook(imguihooks::overlay::SSAODebug());
    _imgui.stage_hooks.add_hook(imguihooks::overlay::SceneOverlays());

    renderer.add_next_precompute_stage ("Point Light Setup",         MOVE(plightsetup));   // NOLINT(performance-move-const-arg)
    renderer.add_next_precompute_stage ("Transfrom Resolution",      MOVE(tfresolution));  // NOLINT(performance-move-const-arg)
    renderer.add_next_precompute_stage ("BV Resolution",             MOVE(bvresolution));  // NOLINT(performance-move-const-arg)
    renderer.add_next_precompute_stage ("Frustum Culling",           MOVE(frustumculler)); // NOLINT(performance-move-const-arg)
    renderer.add_next_precompute_stage ("Skinning",                  MOVE(skinning));      // NOLINT(performance-move-const-arg)
    renderer.add_next_primary_stage    ("Point Shadow Mapping",      MOVE(psmapping));
    renderer.add_next_primary_stage    ("Cascaded Shadow Mapping",   MOVE(csmapping));
    renderer.add_next_primary_stage    ("IDBuffer",                  MOVE(idbuffer));
    renderer.add_next_primary_stage    ("GBuffer",                   MOVE(gbuffer));
    renderer.add_next_primary_stage    ("Deferred Mesh Geometry",    MOVE(defgeom));
    renderer.add_next_primary_stage    ("Deferred Skinned Geometry", MOVE(skingeom));
    renderer.add_next_primary_stage    ("Deferred Terrain Geometry", MOVE(terraingeom));
    renderer.add_next_primary_stage    ("SSAO",                      MOVE(ssao));
    renderer.add_next_primary_stage    ("Deferred Shading",          MOVE(defshad));
    renderer.add_next_primary_stage    ("Light Dummies",             MOVE(lightdummies));
    renderer.add_next_primary_stage    ("Sky",                       MOVE(sky));
    renderer.add_next_postprocess_stage("Fog",                       MOVE(fog));       // NOLINT(performance-move-const-arg)
    renderer.add_next_postprocess_stage("Bloom2",                    MOVE(bloom2));
    renderer.add_next_postprocess_stage("HDR Eye Adaptation",        MOVE(hdreyeing));
    renderer.add_next_postprocess_stage("FXAA",                      MOVE(fxaaaaaaa)); // NOLINT(performance-move-const-arg)
    renderer.add_next_overlay_stage    ("GBuffer Debug",             MOVE(gbugger));
    renderer.add_next_overlay_stage    ("CSM Debug",                 MOVE(csmbugger));
    renderer.add_next_overlay_stage    ("SSAO Debug",                MOVE(ssaobugger));
    renderer.add_next_overlay_stage    ("Scene Overlays",            MOVE(sceneovers));

    configure_input();
    register_default_resource_info   (resource_info());
    register_default_resource_storage(runtime.resource_registry);
    register_default_importers       (runtime.asset_importer);
    register_default_loaders         (runtime.resource_loader);
    register_default_unpackers       (runtime.resource_unpacker);
    init_registry();
    register_default_resource_inspectors(_imgui.resource_viewer);
}

DemoScene::~DemoScene() noexcept
{
    // NOTE: We have to drain tasks manually before destruction
    // of any of the class members, since some of the tasks might
    // depend on those members being alive.
    usize tasks_drained = -1;
    do
    {
        try
        {
            tasks_drained = runtime.async_cradle.local_context.drain_all_tasks();
        }
        catch (const std::exception& e)
        {
            logstream() << "[ERROR]:" << e.what() << '\n';
        }
        catch (...)
        {
            logstream() << "[UNKNOWN ERROR]\n";
        }
    }
    while (tasks_drained != 0);
}

void DemoScene::configure_input()
{
    _input_freecam.configure(_input);

    _input.bind_key(glfw::KeyCode::T, [this](const KeyCallbackArgs& args)
    {
        if (args.is_released())
            _imgui.hidden ^= true;
    });

    // FIXME: This whole thing is really in the wrong place though.
    _input.bind_mouse_button(glfw::MouseButton::Left,
        [this](const MouseButtonCallbackArgs& args)
        {
            // Bail if there's no ID buffer to peek at.
            if (not runtime.renderer.belt.has<IDBuffer>()) return;

            auto& registry = runtime.registry;
            auto& idbuffer = runtime.renderer.belt.get<IDBuffer>();

            if (args.is_pressed())
            {
                const bool select_exact = args.mods & glfw::ModifierKeyBit::Control;
                const bool toggle_mode  = args.mods & glfw::ModifierKeyBit::Shift;

                const auto [x,    y   ] = args.window.getCursorPos();
                const auto [sz_x, sz_y] = args.window.getFramebufferSize();
                int ix = int(x);
                int iy = int(y);

                entt::id_type id{ entt::null };

                // FIXME: Is this off-by-one?
                const Region2I target_pixel = { { ix, sz_y - iy }, { 1, 1 } };
                const auto     pdformat     = PixelDataFormat::RedInteger;
                const auto     pdtype       = PixelDataType::UInt;

                // NOTE: This is a guaranteed way to get a stall, but we
                // don't really care since it's not really noticeable.
                // But it does show up as a nasty spike on the framegraph.
                idbuffer.object_id_texture().download_image_region_into(
                    target_pixel, pdformat, pdtype, make_span(&id, 1));

                Handle provoking_handle = { registry, Entity(id) };

                // Either the click intersected null value (background),
                // or something else has destroyed the entity after
                // the ID buffer was generated on the previous frame.
                // (Can this even happen? Either way, bail if so.)
                if (not provoking_handle.valid())
                {
                    if (toggle_mode)
                    {
                        // If we are in the toggle mode, then do nothing.
                    }
                    else /* unique mode */
                    {
                        // Otherwise we probably want to deselect all current selections.
                        registry.clear<Selected>();
                    }
                    return;
                }

                Handle target_handle = eval%[&]() -> Handle {
                    if (select_exact)
                    {
                        // Select mesh same id as returned.
                        return provoking_handle;
                    }
                    else /* select root */
                    {
                        // Select the root of the tree if the mesh has parents.
                        // This will just return itself, if it has no parents.
                        return get_root_handle(provoking_handle);
                    }
                };

                if (toggle_mode)
                {
                    // We add to current selection if not selected,
                    // and deselect if it was. Don't touch others.
                    switch_tag<Selected>(target_handle);
                }
                else /* unique mode */
                {
                    // We deselect all others, and force select target.
                    registry.clear<Selected>();
                    set_tag<Selected>(target_handle);
                }
            }
        });

    _input.bind_mouse_button(glfw::MouseButton::Middle,
        [&, this](const MouseButtonCallbackArgs& args)
        {
            if (args.is_pressed())
            {
                switch (_imgui.gizmos.active_operation)
                {
                    using enum GizmoOperation;
                    case Translation: _imgui.gizmos.active_operation = Rotation;    break;
                    case Rotation:    _imgui.gizmos.active_operation = Scaling;     break;
                    case Scaling:     _imgui.gizmos.active_operation = Translation; break;
                }
            }
        });

    _input.bind_mouse_button(glfw::MouseButton::Right,
        [&, this](const MouseButtonCallbackArgs& args)
        {
            if (args.is_pressed())
            {
                switch (_imgui.gizmos.active_space)
                {
                    using enum GizmoSpace;
                    case World: _imgui.gizmos.active_space = Local; break;
                    case Local: _imgui.gizmos.active_space = World; break;
                }
            }
        });

    // EWW: Do this somewhere else.
    window.framebufferSizeEvent.setCallback(
        [this](glfw::Window&, int w, int h)
        {
            globals::window_size.set_to({ w, h });
            runtime.renderer.main_resolution = { w, h };
        });
}

void DemoScene::init_registry()
{
    auto& registry = runtime.registry;

    const Handle alight_handle = create_handle(registry);
    alight_handle.emplace<AmbientLight>(AmbientLight{ .color = { 0.15f, 0.15f, 0.1f } });
    make_active<AmbientLight>(alight_handle);

    const Handle dlight_handle      = create_handle(registry);
    const quat   dlight_orientation = glm::quatLookAt(vec3{ -0.2f, -1.f, -0.3f }, { 0.f, 1.f, 0.f });
    dlight_handle.emplace<DirectionalLight>(DirectionalLight{ .color = { 0.15f, 0.15f, 0.1f } });
    dlight_handle.emplace<Transform>(Transform().rotate(dlight_orientation));
    set_tag<ShadowCasting>(dlight_handle);
    make_active<DirectionalLight>(dlight_handle);

    using namespace josh::filesystem_literals;

    const auto      model_vpath = "data/models/shadow_scene/shadow_scene.obj"_vpath;
    const AssetPath model_apath = { File(model_vpath),  {} };

    const Handle model_handle = create_handle(registry);
    model_handle.emplace<Transform>();
    runtime.asset_unpacker.submit_model_for_unpacking(model_handle, runtime.asset_manager.load_model(model_apath));
    runtime.asset_unpacker.wait_until_all_pending_are_complete(runtime.asset_manager);

    const Handle camera_handle = create_handle(registry);
    const Camera::Params camera_params = {
        .fovy_rad     = glm::radians(80.f),
        .aspect_ratio = globals::window_size.size_ref().aspect_ratio(),
        .z_near       = 0.1f,
        .z_far        = 500.f,
    };
    camera_handle.emplace<Camera>(camera_params);
    camera_handle.emplace<Transform>().translate({ 0.f, 1.f, 0.f });
    make_active<Camera>(camera_handle);
}

void DemoScene::update_input_blocker_from_imgui_io_state()
{
    auto wants = _imgui.get_io_wants();
    _input_blocker.block_keys = wants.capture_keyboard;
    // FIXME: Need a way to stop the ImGui window from recieving
    // mouse events when I'm in free cam.
    _input_blocker.block_mouse_buttons = wants.capture_mouse;
    _input_blocker.block_scroll = wants.capture_mouse and _input_freecam.state().is_cursor_mode;
}


