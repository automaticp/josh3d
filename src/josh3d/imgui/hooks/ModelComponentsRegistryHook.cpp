#include "ModelComponentsRegistryHook.hpp"
#include "FrustumCuller.hpp"
#include "ImGuiHelpers.hpp"
#include "RenderComponents.hpp"
#include "Transform.hpp"
#include "GLTextures.hpp"
#include "AssimpModelLoader.hpp"
#include "VPath.hpp"
#include <entt/entity/entity.hpp>
#include <entt/entity/fwd.hpp>
#include <glbinding/gl/enum.h>
#include <glm/gtc/type_ptr.hpp>
#include <glbinding/gl/gl.h>
#include <imgui.h>
#include <imgui_stdlib.h>
#include <filesystem>
#include <stdexcept>



namespace josh::imguihooks {



static void transform_widget(Transform& transform) noexcept {

    ImGui::DragFloat3(
        "Position", glm::value_ptr(transform.position()),
        0.2f, -100.f, 100.f
    );

    // FIXME: This is slightly more usable, but the singularity for Pitch around 90d
    // is still unstable. In general: Local X is Pitch, Global Y is Yaw, and Local Z is Roll.
    // Stll very messy to use, but should get the ball rolling.
    const glm::quat& q = transform.rotation();
    // Swap quaternion axes to make pitch around (local) X axis.
    // Also GLM for some reason assumes that the locking [-90, 90] axis is
    // associated with Yaw, not Pitch...? Maybe I am confused here but
    // I want it Pitch, so we also have to swap the euler representation.
    // (In my mind, Pitch and Yaw are Theta and Phi in spherical coordinates respectively).
    const glm::quat q_shfl = glm::quat{ q.w, q.y, q.x, q.z };
    glm::vec3 euler = glm::degrees(glm::vec3{
        glm::yaw(q_shfl),   // Pitch
        glm::pitch(q_shfl), // Yaw
        glm::roll(q_shfl)   // Roll
        // Dont believe what GLM says
    });
    if (ImGui::DragFloat3("Pitch/Yaw/Roll", glm::value_ptr(euler), 1.0f, -360.f, 360.f, "%.3f")) {
        euler.x = glm::clamp(euler.x, -89.999f, 89.999f);
        euler.y = glm::mod(euler.y, 360.f);
        euler.z = glm::mod(euler.z, 360.f);
        // Un-shuffle back both the euler angles and quaternions.
        glm::quat p = glm::quat(glm::radians(glm::vec3{ euler.y, euler.x, euler.z }));
        transform.rotation() = glm::quat{ p.w, p.y, p.x, p.z };
    }

    ImGui::DragFloat3(
        "Scale", glm::value_ptr(transform.scaling()),
        0.1f, 0.01f, 100.f, "%.3f", ImGuiSliderFlags_Logarithmic
    );

}




void ModelComponentsRegistryHook::load_model_widget(
    entt::registry& registry)
{

    auto try_load_model = [&] {

        last_load_error_message_ = {};
        entt::handle model_handle{ registry, registry.create() };

        try {

            Path path{ load_path_ };
            if (path.is_absolute()) {
                File file{ path };
                ModelComponentLoader().load_into(model_handle, file);
                model_handle.emplace<components::Path>(std::filesystem::canonical(file.path()));
            } else /* is_relative */ {
                VPath vpath{ path };
                File file{ vpath };
                ModelComponentLoader().load_into(model_handle, file);
                model_handle.emplace<components::VPath>(std::move(vpath));
                model_handle.emplace<components::Path>(std::filesystem::canonical(file.path()));
            }
            model_handle.emplace<components::Name>(path.filename());

            model_handle.emplace<Transform>();

        } catch (const error::RuntimeError& e) {
            model_handle.destroy();
            last_load_error_message_ = e.what();
        } catch (...) {
            model_handle.destroy();
            throw;
        }

    };

    auto load_model_signal = on_value_change_from(false, try_load_model);

    if (ImGui::InputText("##Path or VPath", &load_path_,
        ImGuiInputTextFlags_EnterReturnsTrue))
    {
        load_model_signal.set(true);
    }
    ImGui::SameLine();
    if (ImGui::Button("Load")) {
        load_model_signal.set(true);
    }

    ImGui::TextColored({ 1.0f, 0.5f, 0.5f, 1.0f }, "%s", last_load_error_message_.c_str());

}




static void mesh_subwidget(entt::handle mesh) {

    const char* name = mesh.all_of<components::Name>() ?
        mesh.get<components::Name>().name.c_str() : "(No Name)";

    const char* culled_cstr =
        mesh.all_of<tags::Culled>() ? "(Culled)" : "";

    if (ImGui::TreeNode(void_id(mesh.entity()), "Mesh [%d]%s: %s",
        entt::to_entity(mesh.entity()), culled_cstr, name))
    {

        transform_widget(mesh.get<Transform>());

        bool is_alpha_tested = mesh.all_of<tags::AlphaTested>();
        if (ImGui::Checkbox("Alpha-Testing", &is_alpha_tested)) {
            if (is_alpha_tested) {
                mesh.emplace<tags::AlphaTested>();
            } else {
                mesh.remove<tags::AlphaTested>();
            }
        }

        if (ImGui::TreeNode("Material")) {

            // FIXME:
            // There's gotta be a better way.
            auto get_size = [](const UniqueTexture2D& tex) -> Size2I {
                Size2I out{ 0, 0 };
                tex.bind()
                    .and_then([&] {
                        using enum GLenum;
                        gl::glGetTexLevelParameteriv(tex.target_type, 0, GL_TEXTURE_WIDTH,  &out.width);
                        gl::glGetTexLevelParameteriv(tex.target_type, 0, GL_TEXTURE_HEIGHT, &out.height);
                    })
                    .unbind();
                return out;
            };

            // FIXME: Not sure if scaling to max size is always preferrable.
            auto imsize = [&](const UniqueTexture2D& tex) -> ImVec2 {
                const float w = ImGui::GetContentRegionAvail().x;
                const float h = w / get_size(tex).aspect_ratio();
                return { w, h };
            };

            if (auto material = mesh.try_get<components::MaterialDiffuse>()) {
                if (ImGui::TreeNode("Diffuse")) {
                    ImGui::Unindent();

                    ImGui::ImageGL(void_id(material->diffuse->id()), imsize(*material->diffuse));

                    ImGui::Indent();
                    ImGui::TreePop();
                }
            }

            if (auto material = mesh.try_get<components::MaterialSpecular>()) {
                if (ImGui::TreeNode("Specular")) {
                    ImGui::Unindent();

                    ImGui::ImageGL(void_id(material->specular->id()), imsize(*material->specular));

                    ImGui::DragFloat(
                        "Shininess", &material->shininess,
                        1.0f, 0.1f, 1.e4f, "%.3f", ImGuiSliderFlags_Logarithmic
                    );

                    ImGui::Indent();
                    ImGui::TreePop();
                }
            }

            if (auto material = mesh.try_get<components::MaterialNormal>()) {
                if (ImGui::TreeNode("Normal")) {
                    ImGui::Unindent();

                    ImGui::ImageGL(void_id(material->normal->id()), imsize(*material->normal));

                    ImGui::Indent();
                    ImGui::TreePop();
                }
            }

            ImGui::TreePop();
        }
        ImGui::TreePop();
    }

}




void ModelComponentsRegistryHook::model_list_widget(
    entt::registry& registry)
{

    auto to_remove = on_value_change_from<entt::entity>(
        entt::null,
        [&](const entt::entity& model_ent) {
            auto& model = registry.get<ModelComponent>(model_ent);
            registry.destroy(model.meshes().begin(), model.meshes().end());
            registry.destroy(model_ent);
        }
    );

    for (auto [e, transform, model_component]
        : registry.view<Transform, ModelComponent>().each())
    {
        components::Path* path = registry.try_get<components::Path>(e);
        const char* path_cstr = path ? path->c_str() : "(No Path)";
        components::Name* name = registry.try_get<components::Name>(e);
        const char* name_cstr = name ? name->name.c_str() : "(No Name)";

        ImGui::PushID(void_id(e));

        const bool display_node =
            ImGui::TreeNode(void_id(e), "Model [%d]: %s",
                entt::to_entity(e), name_cstr);


        ImGui::SameLine();
        if (ImGui::SmallButton("Remove")) {
            to_remove.set(e);
        }

        if (display_node) {
            ImGui::TextUnformatted(path_cstr);

            transform_widget(transform);

            for (auto mesh_entity : model_component.meshes()) {    ;
                mesh_subwidget({ registry, mesh_entity });
            }

            ImGui::TreePop();
        }
        ImGui::PopID();
    }

}




void ModelComponentsRegistryHook::operator()(
    entt::registry& registry)
{
    load_model_widget(registry);
    ImGui::Separator();
    model_list_widget(registry);
}




} // namespace josh::imguihooks
