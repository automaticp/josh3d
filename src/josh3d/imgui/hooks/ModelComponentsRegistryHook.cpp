#include "ModelComponentsRegistryHook.hpp"
#include "ImGuiHelpers.hpp"
#include "RenderComponents.hpp"
#include "Transform.hpp"
#include "GLTextures.hpp"
#include "AssimpModelLoader.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <imgui_stdlib.h>
#include <stdexcept>



namespace josh::imguihooks {



static void display_transform_widget(Transform& transform) noexcept {

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








void josh::imguihooks::ModelComponentsRegistryHook::operator()(entt::registry& registry) {

    if (ImGui::Button("Load")) {
        entt::handle model_handle{ registry, registry.create() };
        try {

            ModelComponentLoader()
                .load_into(model_handle, File(load_path));

            model_handle.emplace<Transform>();
            model_handle.emplace<components::Path>(load_path);

            last_load_error_message = {};

        } catch (const std::runtime_error& e) {
            model_handle.destroy();
            last_load_error_message = e.what();
        }
    }


    ImGui::InputText("Path", &load_path);

    ImGui::TextUnformatted(last_load_error_message.c_str());

    ImGui::Separator();

    for (auto [e, transform, model] : registry.view<Transform, ModelComponent>().each()) {
        const char* path = registry.all_of<components::Path>(e) ?
            registry.get<components::Path>(e).c_str() : "(No Path)";

        if (ImGui::TreeNode(void_id(e), "Model [%d]: %s",
            static_cast<entt::id_type>(e), path))
        {

            display_transform_widget(transform);


            for (auto mesh_entity : model.meshes()) {
                entt::handle mesh{ registry, mesh_entity };

                const char* name = mesh.all_of<components::Name>() ?
                    mesh.get<components::Name>().name.c_str() : "(No Name)";

                if (ImGui::TreeNode(void_id(mesh_entity), "Mesh [%d]: %s",
                    entt::id_type(mesh_entity), name))
                {

                    display_transform_widget(mesh.get<Transform>());

                    bool is_alpha_tested = mesh.all_of<components::AlphaTested>();
                    if (ImGui::Checkbox("Alpha-Testing", &is_alpha_tested)) {
                        if (is_alpha_tested) {
                            mesh.emplace<components::AlphaTested>();
                        } else {
                            mesh.remove<components::AlphaTested>();
                        }
                    }

                    if (ImGui::TreeNode("Material")) {

                        if (auto material = mesh.try_get<components::MaterialDiffuse>(); material) {
                            ImGui::TextUnformatted("Diffuse");
                            ImGui::ImageGL(void_id(material->diffuse->id()), { 256.f, 256.f });
                        }

                        if (auto material = mesh.try_get<components::MaterialSpecular>(); material) {
                            ImGui::TextUnformatted("Specular");
                            ImGui::ImageGL(void_id(material->specular->id()), { 256.f, 256.f });
                            ImGui::DragFloat(
                                "Shininess", &material->shininess,
                                1.0f, 0.1f, 1.e4f, "%.3f", ImGuiSliderFlags_Logarithmic
                            );
                        }

                        if (auto material = mesh.try_get<components::MaterialNormal>(); material) {
                            ImGui::TextUnformatted("Normal");
                            ImGui::ImageGL(void_id(material->normal->id()), { 256.f, 256.f });
                        }

                        ImGui::TreePop();
                    }


                    ImGui::TreePop();
                }

            }


            ImGui::TreePop();
        }
    }

}





















} // namespace josh::imguihooks
