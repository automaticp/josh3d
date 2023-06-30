#include "DeferredGeometryStage.hpp"
#include "GLShaders.hpp"
#include "RenderEngine.hpp"
#include "Shared.hpp"
#include "Model.hpp"
#include <entt/entt.hpp>

using namespace gl;

namespace learn {


void DeferredGeometryStage::operator()(
    const RenderEnginePrimaryInterface& engine,
    const entt::registry& registry)
{

    gbuffer_->framebuffer().bind_draw().and_then([&, this] {

        sp_.use().and_then_with_self([&, this](ActiveShaderProgram& ashp) {
            ashp.uniform("projection",
                    engine.camera().perspective_projection_mat(
                        engine.window_size().aspect_ratio()
                    )
                )
                .uniform("view", engine.camera().view_mat());

            for (auto [_, transform, model]
                : registry.view<Transform, Shared<Model>>().each())
            {
                auto model_transform = transform.mtransform();
                ashp.uniform("model", model_transform.model())
                    .uniform("normal_model", model_transform.normal_model());

                model->draw(ashp);
            }

        });

    });
}



} // namespace learn
