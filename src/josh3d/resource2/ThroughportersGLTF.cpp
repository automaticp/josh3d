#include "Common.hpp"
#include "CoroCore.hpp"
#include "ExternalScene.hpp"
#include "Errors.hpp"
#include "Scalars.hpp"
#include "Processing.hpp"
#include "Throughporters.hpp"
#include "VertexStatic.hpp"
#include "detail/CGLTF.hpp"
#include <cgltf.h>


namespace josh {

#if 0
using namespace detail::cgltf;

auto throughport_scene_gltf(
    Path                  path,
    Handle                dst_handle,
    GLTFThroughportParams params,
    AsyncCradleRef        async_cradle,
    MeshRegistry&         mesh_registry)
        -> Job<>
{
    co_await reschedule_to(async_cradle.loading_pool);

    const cgltf_options options = {};
    cgltf_data*         gltf    = {};
    cgltf_result        result  = {};

    result = cgltf_parse_file(&options, path.c_str(), &gltf);
    if (result != cgltf_result_success)
        throw_fmt<GLTFParseError>("Failed to parse gltf file {}, reason {}.", path, enum_string(result));

    const auto _owner = unique_data_ptr(gltf);

    result = cgltf_load_buffers(&options, gltf, path.c_str());
    if (result != cgltf_result_success)
        throw_fmt<GLTFParseError>("Failed to load gltf buffers of {}, reason {}.", path, enum_string(result));

    esr::ExternalScene scene = to_external_scene(*gltf);

    const ESRThroughportParams esr_params = {
        .generate_mips = params.generate_mips,
    };

    const ThroughportContext context = {
        .registry      = *dst_handle.registry(),
        .mesh_registry = mesh_registry,
        .async_cradle  = async_cradle,
    };

    co_await throughport_external_scene(MOVE(scene), dst_handle.entity(), esr_params, context);
}
#endif

} // namespace josh
