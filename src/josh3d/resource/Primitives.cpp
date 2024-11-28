#include "Primitives.hpp"
#include "AssetManager.hpp"
#include "Filesystem.hpp"
#include "GLAPIBinding.hpp"
#include "GLObjects.hpp"
#include "VertexPNUTB.hpp"


namespace josh {


namespace {


Mesh load_simple_mesh(AssetManager& asset_manager, const Path& path) {
    auto shared_mesh = asset_manager.load_model(path).get_result().meshes.at(0);
    make_available<Binding::ArrayBuffer>       (shared_mesh.vertices->id());
    make_available<Binding::ElementArrayBuffer>(shared_mesh.indices->id() );
    return Mesh::from_buffers<VertexPNUTB>(std::move(shared_mesh.vertices), std::move(shared_mesh.indices));
}


} // namespace



Primitives::Primitives(AssetManager& asset_manager)
    : plane_mesh_ { load_simple_mesh(asset_manager, "data/primitives/plane.obj")  }
    , box_mesh_   { load_simple_mesh(asset_manager, "data/primitives/box.obj")    }
    , sphere_mesh_{ load_simple_mesh(asset_manager, "data/primitives/sphere.obj") }
    , quad_mesh_  { load_simple_mesh(asset_manager, "data/primitives/quad.obj")   }
{}






} // namespace josh
