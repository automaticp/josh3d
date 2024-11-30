#include "Primitives.hpp"
#include "AssetManager.hpp"
#include "Filesystem.hpp"
#include "GLAPIBinding.hpp"
#include "GLObjects.hpp"
#include "VertexPNUTB.hpp"
#include <cassert>


namespace josh {


namespace {


Mesh load_simple_mesh(AssetManager& asset_manager, const Path& path) {
    auto job = asset_manager.load_model(path);
    asset_manager.wait_until_ready(job);
    assert(job.is_ready());
    auto shared_mesh = job.get_result().meshes.at(0);
    make_available<Binding::ArrayBuffer>       (shared_mesh.vertices->id());
    make_available<Binding::ElementArrayBuffer>(shared_mesh.indices->id() );
    return Mesh::from_buffers<VertexPNUTB>(MOVE(shared_mesh.vertices), MOVE(shared_mesh.indices));
}


} // namespace



Primitives::Primitives(AssetManager& asset_manager)
    : plane_mesh_ { load_simple_mesh(asset_manager, "data/primitives/plane.obj")  }
    , box_mesh_   { load_simple_mesh(asset_manager, "data/primitives/box.obj")    }
    , sphere_mesh_{ load_simple_mesh(asset_manager, "data/primitives/sphere.obj") }
    , quad_mesh_  { load_simple_mesh(asset_manager, "data/primitives/quad.obj")   }
{}






} // namespace josh
