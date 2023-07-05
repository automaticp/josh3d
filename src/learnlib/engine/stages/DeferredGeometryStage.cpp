#include "DeferredGeometryStage.hpp"
#include "GLShaders.hpp"
#include "MaterialDS.hpp"
#include "RenderEngine.hpp"
#include <entt/entt.hpp>


using namespace gl;

namespace learn {


void DeferredGeometryStage::operator()(
    const RenderEnginePrimaryInterface& engine,
    const entt::registry& registry)
{

    gbuffer_->framebuffer().bind_draw().and_then([&, this] {

        sp_.use().and_then([&, this](ActiveShaderProgram& ashp) {
            ashp.uniform("projection",
                    engine.camera().perspective_projection_mat(
                        engine.window_size().aspect_ratio()
                    )
                )
                .uniform("view", engine.camera().view_mat());


            for (auto [_, transform, mesh, material]
                : registry.view<Transform, Mesh, MaterialDS>(entt::exclude<components::ChildMesh>).each())
            {
                auto model_transform = transform.mtransform();
                ashp.uniform("model", model_transform.model())
                    .uniform("normal_model", model_transform.normal_model());

                material.apply(ashp);
                mesh.draw();
            }


            for (auto [_, transform, mesh, material, as_child]
                : registry.view<Transform, Mesh, MaterialDS, components::ChildMesh>().each())
            {
                auto model_transform =
                    registry.get<Transform>(as_child.parent).mtransform() *
                        transform.mtransform();

                ashp.uniform("model", model_transform.model())
                    .uniform("normal_model", model_transform.normal_model());

                material.apply(ashp);
                mesh.draw();
            }


        });

    });
}



} // namespace learn
