#pragma once
#include "AssetLoader.hpp"
#include "Mesh.hpp"


namespace josh {


class Primitives {
public:
    Primitives(AssetLoader& asset_loader);

    const Mesh& plane_mesh()  const noexcept { return plane_mesh_;  }
    const Mesh& box_mesh()    const noexcept { return box_mesh_;    }
    const Mesh& sphere_mesh() const noexcept { return sphere_mesh_; }
    const Mesh& quad_mesh()   const noexcept { return quad_mesh_;   }

private:
    Mesh plane_mesh_;
    Mesh box_mesh_;
    Mesh sphere_mesh_;
    Mesh quad_mesh_;
};



} // namespace josh
