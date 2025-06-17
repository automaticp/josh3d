#include "TerrainGeometry.hpp"
#include "GLAPICore.hpp"
#include "stages/primary/GBufferStorage.hpp"
#include "GLProgram.hpp"
#include "UniformTraits.hpp"
#include "RenderEngine.hpp"
#include "Transform.hpp"
#include "TerrainChunk.hpp"
#include <entt/entt.hpp>


namespace josh {


void TerrainGeometry::operator()(
    RenderEnginePrimaryInterface& engine)
{
    const auto& registry = engine.registry();
    auto*       gbuffer  = engine.belt().try_get<GBuffer>();

    if (not gbuffer) return;

    const RawProgram<> sp = _sp;

    glapi::set_viewport({ {}, gbuffer->resolution() });

    const BindGuard bcam = engine.bind_camera_ubo();
    const BindGuard bfb  = gbuffer->bind_draw();
    const BindGuard bsp  = sp.use();

    for (auto [entity, world_mtf, chunk]
        : registry.view<MTransform, TerrainChunk>().each())
    {
        chunk.heightmap->bind_to_texture_unit(0);

        sp.uniform("model",        world_mtf.model());
        sp.uniform("normal_model", world_mtf.normal_model());
        sp.uniform("object_id",    entt::to_integral(entity));
        sp.uniform("test_color",   0);

        chunk.mesh.draw(bsp, bfb);
    }
}


} // namespace josh
