#include "TerrainGeometry.hpp"
#include "GLMutability.hpp"
#include "GLShaders.hpp"
#include "RenderEngine.hpp"
#include "Transform.hpp"
#include "components/TerrainChunk.hpp"
#include <entt/entt.hpp>




namespace josh::stages::primary {


void TerrainGeometry::operator()(
    RenderEnginePrimaryInterface& engine)
{
    const auto& registry = engine.registry();

    gbuffer_->bind_draw().and_then([&] {
        sp_.use().and_then([&](ActiveShaderProgram<GLMutable>& ashp) {

            ashp.uniform("projection", engine.camera().projection_mat())
                .uniform("view",       engine.camera().view_mat());

            for (auto [entity, world_mtf, chunk]
                : registry.view<MTransform, components::TerrainChunk>().each())
            {
                chunk.heightmap.bind_to_unit_index(0);

                ashp.uniform("model",        world_mtf.model())
                    .uniform("normal_model", world_mtf.normal_model())
                    .uniform("object_id",    entt::to_integral(entity))
                    .uniform("test_color",   0);

                chunk.mesh.draw();
            }

        });
    });


}


} // namespace josh::stages::primary
