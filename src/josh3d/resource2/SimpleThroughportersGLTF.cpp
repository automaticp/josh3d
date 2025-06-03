#include "Common.hpp"
#include "RuntimeError.hpp"
#include "Scalars.hpp"
#include "SimpleThroughporters.hpp"
#include "VertexStatic.hpp"
#include "detail/CGLTF.hpp"
#include <cgltf.h>


namespace josh {

using namespace detail::cgltf;

auto throughport_scene_gltf(
    Path                  path,
    Handle                dst_handle,
    GLTFThroughportParams params,
    AsyncCradleRef        async_cradle,
    MeshRegistry&         mesh_registry)
        -> Job<>
{
    const cgltf_options options = {};
    cgltf_data*         gltf    = {};
    cgltf_result        result  = {};

    result = cgltf_parse_file(&options, path.c_str(), &gltf);
    if (result != cgltf_result_success)
        throw_fmt<GLTFParseError>("Failed ot parse gltf file {}, reason {}.", path, enum_string(result));

    const auto _owner = unique_data_ptr(gltf);

    result = cgltf_load_buffers(&options, gltf, path.c_str());
    if (result != cgltf_result_success)
        throw_fmt<GLTFParseError>("Failed to load gltf buffers of {}, reason {}.", path, enum_string(result));

    // NOTE: Will prefix gltf-specific data with _ to differentiate for now.
    const auto _meshes = make_span(gltf->meshes, gltf->meshes_count);

    for (const cgltf_mesh& _mesh : _meshes)
    {
        const auto _primitives = make_span(_mesh.primitives, _mesh.primitives_count);

        // TODO: This could be handled by a unitarization-like scheme.
        if (_primitives.size() > 1)
            throw GLTFParseError("Mult-primitive meshes not (yet) supported.");

        for (const cgltf_primitive& _primitive : _primitives)
        {
            if (_primitive.type != cgltf_primitive_type_triangles)
                throw GLTFParseError("Primitive types other than triangles are not supported.");
            if (_primitive.has_draco_mesh_compression)
                throw GLTFParseError("Draco mesh compression not supported.");

            // const Attributes attributes = parse_primitive_attributes(_primitive);

            // NOTE: We decide on whether the mesh is skinned based on the presence
            // of respective attributes, even if no skeleton is attached to it.
            // const bool is_skinned = (attributes.joint_ids and attributes.joint_weights);

            // if (is_skinned) validate_attributes_skinned(attributes);
            // else            validate_attributes_static (attributes);

        }
    }


}


} // namespace josh
