#pragma once
#include "MaterialConcept.hpp"
#include "SharedStorage.hpp"
#include "GBuffer.hpp"
#include "RenderEngine.hpp"
#include <entt/entt.hpp>


namespace josh {


/*
Not really "any", but as long as the shaders match the uniforms, all is fine.

Uniforms:

    uniform mat4 projection;
    uniform mat4 view;
    uniform mat4 model;
    uniform mat3 normal_model;

Plus whatever the MaterialT requires.
*/
template<material MaterialT>
class DeferredGeometryAnyMaterialStage {
private:
    ShaderProgram sp_;

    SharedStorageMutableView<GBuffer> gbuffer_;

public:
    DeferredGeometryAnyMaterialStage(
        SharedStorageMutableView<GBuffer> gbuffer_view,
        ShaderProgram sp)
        : gbuffer_{ std::move(gbuffer_view) }
        , sp_{ std::move(sp) }
    {}

    void operator()(const RenderEnginePrimaryInterface&, const entt::registry&);

};




template<material MaterialT>
inline void DeferredGeometryAnyMaterialStage<MaterialT>::operator()(
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
                : registry.view<Transform, Mesh, MaterialT>(entt::exclude<components::ChildMesh>).each())
            {
                auto model_transform = transform.mtransform();
                ashp.uniform("model", model_transform.model())
                    .uniform("normal_model", model_transform.normal_model());

                material.apply(ashp);
                mesh.draw();
            }


            for (auto [_, transform, mesh, material, as_child]
                : registry.view<Transform, Mesh, MaterialT, components::ChildMesh>().each())
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







} // namespace josh
