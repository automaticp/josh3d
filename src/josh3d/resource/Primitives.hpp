#pragma once
#include "AssetManager.hpp"
#include "Mesh.hpp"


namespace josh {


class Primitives {
public:
    Primitives(AssetManager& assman);

    RawTexture2D<GLConst> default_diffuse_texture()  const noexcept { return default_diffuse_texture_;  }
    RawTexture2D<GLConst> default_specular_texture() const noexcept { return default_specular_texture_; }
    RawTexture2D<GLConst> default_normal_texture()   const noexcept { return default_normal_texture_;   }
    RawCubemap<GLConst>   debug_skybox_cubemap()     const noexcept { return debug_skybox_cubemap_;     }

    const Mesh& plane_mesh()  const noexcept { return plane_mesh_;  }
    const Mesh& box_mesh()    const noexcept { return box_mesh_;    }
    const Mesh& sphere_mesh() const noexcept { return sphere_mesh_; }
    const Mesh& quad_mesh()   const noexcept { return quad_mesh_;   }

private:
    UniqueTexture2D default_diffuse_texture_;
    UniqueTexture2D default_specular_texture_;
    UniqueTexture2D default_normal_texture_;
    UniqueCubemap   debug_skybox_cubemap_;

    Mesh            plane_mesh_;
    Mesh            box_mesh_;
    Mesh            sphere_mesh_;
    Mesh            quad_mesh_;
};



} // namespace josh
