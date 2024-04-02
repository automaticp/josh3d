#pragma once
#include "GLObjects.hpp"
#include "UniformTraits.hpp" // IWYU pragma: keep (traits)
#include "LightCasters.hpp"
#include "RenderEngine.hpp"
#include "ShaderBuilder.hpp"
#include "Transform.hpp"
#include "VPath.hpp"
#include "Mesh.hpp"
#include "DefaultResources.hpp"
#include <entt/entt.hpp>




namespace josh::stages::primary {


class PointLightBox {
public:
    bool  display{ true };
    float light_box_scale{ 0.1f };

    PointLightBox() = default;

    void operator()(RenderEnginePrimaryInterface& engine) {

        if (!display) { return; }


        const auto& registry = engine.registry();


        sp_->uniform("projection", engine.camera().projection_mat());
        sp_->uniform("view",       engine.camera().view_mat());

        auto bound_program = sp_->use();

        engine.draw([&, this](auto bound_fbo) {

            for (auto [entity, plight] : registry.view<light::Point>().each()) {

                const auto t = Transform()
                    .translate(plight.position)
                    .scale(glm::vec3{ light_box_scale });

                sp_->uniform("model",       t.mtransform().model());
                sp_->uniform("light_color", plight.color);

                globals::box_primitive_mesh().draw(bound_program, bound_fbo);;
            }

        });

        bound_program.unbind();
    }


private:
    dsa::UniqueProgram sp_{
        ShaderBuilder()
            .load_vert(VPath("src/shaders/basic_mesh.vert"))
            .load_frag(VPath("src/shaders/light_source.frag"))
            .get()
    };

};


} // namespace josh::stages::primary
