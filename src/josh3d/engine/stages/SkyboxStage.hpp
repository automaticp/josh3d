#pragma once
#include "DefaultResources.hpp"
#include "LightCasters.hpp"
#include "RenderEngine.hpp"
#include "ShaderBuilder.hpp"
#include "components/Skybox.hpp"
#include "VPath.hpp"
#include <entt/entt.hpp>
#include <glbinding/gl/functions.h>
#include <glbinding/gl/gl.h>
#include <glm/geometric.hpp>
#include <glm/matrix.hpp>




namespace josh {


// TODO: Rename to SkyStage?
class SkyboxStage {
public:
    enum class SkyType {
        none, debug, skybox, procedural
    };

    struct ProceduralSkyParams {
        glm::vec3 sky_color{ 0.173f, 0.382f, 0.5f };
        glm::vec3 sun_color{ 1.f, 1.f, 1.f  };
        float     sun_size_deg{ 0.5f };
    };

private:
    UniqueShaderProgram sp_skybox_{
        ShaderBuilder()
            .load_vert(VPath("src/shaders/skybox.vert"))
            .load_frag(VPath("src/shaders/skybox.frag"))
            .get()
    };

    UniqueShaderProgram sp_proc_{
        ShaderBuilder()
            .load_vert(VPath("src/shaders/sky_procedural.vert"))
            .load_frag(VPath("src/shaders/sky_procedural.frag"))
            .get()
    };

public:
    SkyType sky_type{ SkyType::skybox };
    ProceduralSkyParams procedural_sky_params{};

    void operator()(
        const RenderEnginePrimaryInterface& engine,
        const entt::registry& registry)
    {
        switch (sky_type) {
            using enum SkyType;
            case none:       return;
            case debug:      draw_debug_skybox(engine);             break;
            case skybox:     draw_skybox(engine, registry);         break;
            case procedural: draw_procedural_sky(engine, registry); break;
        }
    }

private:
    void draw_debug_skybox(
        const RenderEnginePrimaryInterface& engine)
    {
        using namespace gl;

        glm::mat4 projection = engine.camera().projection_mat();
        glm::mat4 view       = engine.camera().view_mat();

        engine.draw([&, this] {

            glDepthMask(GL_FALSE);
            glDepthFunc(GL_LEQUAL);

            globals::debug_skybox_cubemap().bind_to_unit_index(0);
            sp_skybox_.use()
                .uniform("projection", projection)
                .uniform("view", glm::mat4{ glm::mat3{ view } })
                .uniform("cubemap", 0)
                .and_then([] {
                    globals::box_primitive_mesh().draw();
                });

            glDepthMask(GL_TRUE);
            glDepthFunc(GL_LESS);

        });

    }

    void draw_skybox(
        const RenderEnginePrimaryInterface& engine,
        const entt::registry& registry)
    {
        using namespace gl;

        glm::mat4 projection = engine.camera().projection_mat();
        glm::mat4 view       = engine.camera().view_mat();

        engine.draw([&, this] {

            for (auto [e, skybox] : registry.view<const components::Skybox>().each()) {

                glDepthMask(GL_FALSE);
                glDepthFunc(GL_LEQUAL);

                skybox.cubemap->bind_to_unit_index(0);
                sp_skybox_.use()
                    .uniform("projection", projection)
                    .uniform("view", glm::mat4{ glm::mat3{ view } })
                    .uniform("cubemap", 0)
                    .and_then([] {
                        globals::box_primitive_mesh().draw();
                    });

                glDepthMask(GL_TRUE);
                glDepthFunc(GL_LESS);

            }

        });
    }

    void draw_procedural_sky(
        const RenderEnginePrimaryInterface& engine,
        const entt::registry& registry)
    {
        using namespace gl;

        const auto& cam        = engine.camera();
        const auto& cam_params = cam.get_params();

        const glm::mat4 inv_proj =
            glm::inverse(cam.projection_mat());

        // UB if no light, lmao
        const auto& light =
            *registry.storage<light::Directional>().begin();

        const glm::vec3 light_dir_view_space =
            glm::normalize(glm::vec3{ cam.view_mat() * glm::vec4{ light.direction, 0.f } });

        engine.draw([&, this] {

            sp_proc_.use()
                .uniform("z_far",                cam_params.z_far)
                .uniform("inv_proj",             inv_proj)
                .uniform("light_dir_view_space", light_dir_view_space)
                .uniform("sky_color", procedural_sky_params.sky_color)
                .uniform("sun_color", procedural_sky_params.sun_color)
                .uniform("sun_size_rad",
                    glm::radians(procedural_sky_params.sun_size_deg))
                .and_then([] {
                    glDepthMask(GL_FALSE);
                    glDepthFunc(GL_LEQUAL);

                    globals::quad_primitive_mesh().draw();

                    glDepthMask(GL_TRUE);
                    glDepthFunc(GL_LESS);
                });

        });

    }
};


} // namspace josh
