#include "Primitives.hpp"
#include "AssetManager.hpp"
#include "Filesystem.hpp"
#include "GLAPIBinding.hpp"
#include <cassert>


namespace josh {


namespace {


Mesh load_simple_mesh(AssetManager& asset_manager, const Path& path) {
    auto job = asset_manager.load_model(path);
    asset_manager.wait_until_ready(job);
    assert(job.is_ready());
    auto shared_mesh = job.get_result().meshes.at(0);
    return visit([&]<typename T>(T& mesh_asset) -> Mesh {
        make_available<Binding::ArrayBuffer>       (mesh_asset.vertices->id());
        make_available<Binding::ElementArrayBuffer>(mesh_asset.indices->id() );
        return Mesh::from_buffers<typename T::vertex_type>(MOVE(mesh_asset.vertices), MOVE(mesh_asset.indices));
    }, shared_mesh);
}


} // namespace



Primitives::Primitives(AssetManager& asset_manager)
    : plane_mesh_ { load_simple_mesh(asset_manager, "data/primitives/plane.obj")  }
    , box_mesh_   { load_simple_mesh(asset_manager, "data/primitives/box.obj")    }
    , sphere_mesh_{ load_simple_mesh(asset_manager, "data/primitives/sphere.obj") }
    , quad_mesh_  { load_simple_mesh(asset_manager, "data/primitives/quad.obj")   }
{}






} // namespace josh
