#include "Primitives.hpp"
#include "AssetManager.hpp"
#include "Filesystem.hpp"
#include "Future.hpp"
#include "GLAPIBinding.hpp"
#include "GLObjects.hpp"
#include "VertexPNUTB.hpp"


namespace josh {


namespace {


Mesh load_simple_mesh(AssetManager& assman, Path path) {
    auto shared_mesh = get_result(assman.load_model(AssetPath{ std::move(path), {} })).meshes.at(0);
    make_available<Binding::ArrayBuffer>       (shared_mesh.vertices->id());
    make_available<Binding::ElementArrayBuffer>(shared_mesh.indices->id() );
    return Mesh::from_buffers<VertexPNUTB>(std::move(shared_mesh.vertices), std::move(shared_mesh.indices));
}


} // namespace



Primitives::Primitives(AssetManager& assman)
    : plane_mesh_{
        load_simple_mesh(assman, "data/primitives/plane.obj")
    }
    , box_mesh_{
        load_simple_mesh(assman, "data/primitives/box.obj")
    }
    , sphere_mesh_{
        load_simple_mesh(assman, "data/primitives/sphere.obj")
    }
    , quad_mesh_{
        load_simple_mesh(assman, "data/primitives/quad.obj")
    }
{}






} // namespace josh
