#include "TerrainGeometry.hpp"
#include "UniformTraits.hpp" // IWYU pragma: keep (traits)
#include "RenderEngine.hpp"
#include "Transform.hpp"
#include "components/TerrainChunk.hpp"
#include <entt/entt.hpp>




namespace josh::stages::primary {


void TerrainGeometry::operator()(
    RenderEnginePrimaryInterface& engine)
{
    const auto& registry = engine.registry();

    BindGuard bound_camera  = engine.bind_camera_ubo();
    BindGuard bound_fbo     = gbuffer_->bind_draw();
    BindGuard bound_program = sp_->use();

    for (auto [entity, world_mtf, chunk]
        : registry.view<MTransform, components::TerrainChunk>().each())
    {
        chunk.heightmap->bind_to_texture_unit(0);

        sp_->uniform("model",        world_mtf.model());
        sp_->uniform("normal_model", world_mtf.normal_model());
        sp_->uniform("object_id",    entt::to_integral(entity));
        sp_->uniform("test_color",   0);

        chunk.mesh.draw(bound_program, bound_fbo);
    }

}


} // namespace josh::stages::primary
