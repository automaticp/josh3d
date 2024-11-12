#include "Primitives.hpp"
#include "AssetLoader.hpp"
#include "Filesystem.hpp"
#include "Future.hpp"
#include "GLAPIBinding.hpp"
#include "GLObjects.hpp"
#include "VertexPNUTB.hpp"


namespace josh {


namespace {


Mesh load_simple_mesh(AssetLoader& asset_loader, Path path) {
    auto shared_mesh = get_result(asset_loader.load_model(AssetPath{ std::move(path), {} })).meshes.at(0);
    make_available<Binding::ArrayBuffer>       (shared_mesh.vertices->id());
    make_available<Binding::ElementArrayBuffer>(shared_mesh.indices->id() );
    return Mesh::from_buffers<VertexPNUTB>(std::move(shared_mesh.vertices), std::move(shared_mesh.indices));
}


} // namespace



Primitives::Primitives(AssetLoader& asset_loader)
    : plane_mesh_ { load_simple_mesh(asset_loader, "data/primitives/plane.obj")  }
    , box_mesh_   { load_simple_mesh(asset_loader, "data/primitives/box.obj")    }
    , sphere_mesh_{ load_simple_mesh(asset_loader, "data/primitives/sphere.obj") }
    , quad_mesh_  { load_simple_mesh(asset_loader, "data/primitives/quad.obj")   }
{}






} // namespace josh
